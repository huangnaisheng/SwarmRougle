#include "SRSwarmSubsystem.h"

#include "SREnemyBase.h"
#include "SRExpGem.h"
#include "Engine/World.h"

USRSwarmSubsystem* USRSwarmSubsystem::Get(const UWorld* World)
{
	return World ? World->GetSubsystem<USRSwarmSubsystem>() : nullptr;
}

void USRSwarmSubsystem::Deinitialize()
{
	// 弱指针不需要手动清理，它们会在 Actor 销毁后自动失效；
	// 这里清空容器即可，防止世界切换时残留脏数据
	Grid.Empty();
	EnemyPool.Empty();
	GemPool.Empty();

	Super::Deinitialize();
}

FIntVector2 USRSwarmSubsystem::GetCellCoord(const FVector& Location) const
{
	// 用 FloorToInt 向下取整：负数坐标也会正确归到负号格子，不会出现 (0,0) 附近错位
	return FIntVector2(
		FMath::FloorToInt(Location.X / GridCellSize),
		FMath::FloorToInt(Location.Y / GridCellSize)
	);
}

// ==================== 空间哈希网格 ====================

void USRSwarmSubsystem::RegisterEnemy(ASREnemyBase* Enemy, const FVector& Location)
{
	if (!Enemy) return;

	const FIntVector2 Cell = GetCellCoord(Location);
	Grid.FindOrAdd(Cell).AddUnique(Enemy); // AddUnique 防御重复注册（新生成的怪会注册两次，见讲解）

	// 让敌人记住自己所在的格子，后续跨格判断全靠它
	Enemy->SetSwarmGridCell(Cell);
}

void USRSwarmSubsystem::UnregisterEnemy(ASREnemyBase* Enemy)
{
	if (!Enemy) return;

	const FIntVector2 Cell = Enemy->GetSwarmGridCell();
	if (TArray<TWeakObjectPtr<ASREnemyBase>>* Bucket = Grid.Find(Cell))
	{
		Bucket->RemoveAll([Enemy](const TWeakObjectPtr<ASREnemyBase>& WeakEnemy)
		{
			return WeakEnemy.Get() == Enemy;
		});

		// 格子空了就删掉条目，防止 TMap 里积累大量空桶
		if (Bucket->IsEmpty())
		{
			Grid.Remove(Cell);
		}
	}

	// 标记"不在网格"，INT32_MIN 这个坐标不可能被正常敌人使用
	Enemy->SetSwarmGridCell(FIntVector2(INT32_MIN, INT32_MIN));
}

void USRSwarmSubsystem::UpdateEnemyCell(ASREnemyBase* Enemy, const FVector& NewLocation)
{
	if (!Enemy) return;

	const FIntVector2 NewCell = GetCellCoord(NewLocation);

	// 没跨格子：这是最常见的情况，一次比较就结束，零成本
	if (Enemy->GetSwarmGridCell() == NewCell)
	{
		return;
	}

	// 跨格子了：先退出旧格，再进新格
	UnregisterEnemy(Enemy);
	RegisterEnemy(Enemy, NewLocation);
}

void USRSwarmSubsystem::QueryNeighbors(const FVector& Center, float Radius, TArray<ASREnemyBase*>& OutNeighbors, AActor* IgnoreActor)
{
	OutNeighbors.Reset();

	const FIntVector2 CenterCell = GetCellCoord(Center);
	const float RadiusSq = Radius * Radius;

	// 因为格子边长(100) > 查询半径(80)，遍历 3x3 = 9 个格子必能覆盖整个查询圆
	for (int32 X = -1; X <= 1; ++X)
	{
		for (int32 Y = -1; Y <= 1; ++Y)
		{
			const FIntVector2 Cell(CenterCell.X + X, CenterCell.Y + Y);

			if (const TArray<TWeakObjectPtr<ASREnemyBase>>* Bucket = Grid.Find(Cell))
			{
				for (const TWeakObjectPtr<ASREnemyBase>& WeakEnemy : *Bucket)
				{
					ASREnemyBase* Enemy = WeakEnemy.Get();
					if (!Enemy || Enemy == IgnoreActor)
					{
						continue; // 弱指针已失效，或查到了自己
					}

					// 格子是粗略筛选，这里再用距离平方做精确筛选（不必要的不开根号）
					if (FVector::DistSquared(Enemy->GetActorLocation(), Center) <= RadiusSq)
					{
						OutNeighbors.Add(Enemy);
					}
				}
			}
		}
	}
}

// ==================== 对象池 ====================

ASREnemyBase* USRSwarmSubsystem::AcquireEnemy(TSubclassOf<ASREnemyBase> EnemyClass, const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!World || !EnemyClass)
	{
		return nullptr;
	}

	ASREnemyBase* Enemy = nullptr;

	// 1. 先翻同类的池子，从尾部弹一个活的出来
	if (TArray<TWeakObjectPtr<ASREnemyBase>>* Pool = EnemyPool.Find(EnemyClass))
	{
		while (Pool->Num() > 0)
		{
			Enemy = Pool->Pop().Get();
			if (Enemy)
			{
				break; // 拿到活的了
			}
			// 弹到失效的弱指针就继续弹下一个（正常情况下很少发生）
		}
	}

	// 2. 池里没有，才真正 SpawnActor（预热期；运行稳定后几乎不再发生）
	if (!Enemy)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Enemy = World->SpawnActor<ASREnemyBase>(EnemyClass, Location, FRotator::ZeroRotator, SpawnParams);
	}

	// 3. 唤醒它：重置状态、显示、同步客户端、重新入网（都由 Actor 自己负责）
	if (Enemy)
	{
		Enemy->OnAcquiredFromPool(Location);
	}

	return Enemy;
}

void USRSwarmSubsystem::ReleaseEnemy(ASREnemyBase* Enemy)
{
	if (!Enemy) return;

	// 先让 Actor 自己完成"睡眠仪式"（停移动、隐藏、出网格、挪藏点），再放入池
	Enemy->OnReleasedToPool();

	EnemyPool.FindOrAdd(TSubclassOf<ASREnemyBase>(Enemy->GetClass())).Add(Enemy);
}

ASRExpGem* USRSwarmSubsystem::AcquireGem(TSubclassOf<ASRExpGem> GemClass, const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!World || !GemClass)
	{
		return nullptr;
	}

	ASRExpGem* Gem = nullptr;

	if (TArray<TWeakObjectPtr<ASRExpGem>>* Pool = GemPool.Find(GemClass))
	{
		while (Pool->Num() > 0)
		{
			Gem = Pool->Pop().Get();
			if (Gem)
			{
				break;
			}
		}
	}

	if (!Gem)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Gem = World->SpawnActor<ASRExpGem>(GemClass, Location, FRotator::ZeroRotator, SpawnParams);
	}

	if (Gem)
	{
		Gem->OnAcquiredFromPool(Location);
	}

	return Gem;
}

void USRSwarmSubsystem::ReleaseGem(ASRExpGem* Gem)
{
	if (!Gem) return;

	Gem->OnReleasedToPool();

	GemPool.FindOrAdd(TSubclassOf<ASRExpGem>(Gem->GetClass())).Add(Gem);
}
