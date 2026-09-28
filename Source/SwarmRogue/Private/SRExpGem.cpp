#include "SRExpGem.h"
#include "SRGameStateBase.h"
#include "SRSwarmSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ASRExpGem::ASRExpGem()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true); // 让经验球的位移在所有客户端同步
}

void ASRExpGem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 顺序很重要：先复制出生点，再复制池化状态（客户端 OnRep_Pooled 时位置已就绪）
	DOREPLIFETIME(ASRExpGem, PendingSpawnLocation);
	DOREPLIFETIME(ASRExpGem, bPooled);
}

void ASRExpGem::BeginPlay()
{
	Super::BeginPlay();

	// 客户端不需要 Tick：磁吸移动全由服务器驱动、位置通过网络同步
	SetActorTickEnabled(HasAuthority());
}

// ==========================================
// 对象池：唤醒 / 回收（P0）
// ==========================================

void ASRExpGem::OnAcquiredFromPool(const FVector& SpawnLocation)
{
	if (!HasAuthority()) return;

	// 1. 清空上次的拾取目标，允许重新寻找
	TargetPlayer = nullptr;
	NextSearchTime = 0.f;

	// 2. 挪到出生点（掉落位置）
	PendingSpawnLocation = SpawnLocation;
	SetActorLocation(SpawnLocation, false);

	// 3. 服务器侧显示 + 恢复 Tick
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	// 4. 通知客户端"我活了"
	bPooled = false;
}

void ASRExpGem::OnReleasedToPool()
{
	if (!HasAuthority()) return;

	// 1. 清空目标，防止"睡着"后还惦记着玩家
	TargetPlayer = nullptr;

	// 2. 标记回收 + 隐藏 + 停 Tick
	bPooled = true;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	// 3. 挪到藏点（已隐藏，客户端看不到这次瞬移）
	//    注意：UWorld::OriginLocation 是 FIntVector，必须显式转 FVector 才能做加法
	const FVector HideSpot(
		GetWorld()->OriginLocation.X,
		GetWorld()->OriginLocation.Y,
		GetWorld()->OriginLocation.Z - 10000.f
	);
	SetActorLocation(HideSpot, false);
}

void ASRExpGem::OnRep_Pooled()
{
	// 客户端表现同步
	if (bPooled)
	{
		SetActorHiddenInGame(true);
		SetActorTickEnabled(false);
	}
	else
	{
		// 经验球是普通 Actor（没有 CharacterMovement 的平滑插值），
		// 直接对齐服务器位置即可，不会有"飞过来"的插值撕裂
		SetActorLocation(PendingSpawnLocation, false);
		SetActorHiddenInGame(false);
		SetActorTickEnabled(false); // 客户端保持不 Tick
	}
}

AActor* ASRExpGem::GetClosestPlayer()
{
	ASRGameStateBase* GS = GetWorld()->GetGameState<ASRGameStateBase>();
	if (!GS) return nullptr;

	// 直接拿到花名册
	const TArray<APawn*>& Players = GS->GetActivePlayers();
	if (Players.IsEmpty()) return nullptr;

	AActor* BestTarget = nullptr;
	float ClosestDistanceSq = MAX_FLT;

	for (APawn* Player : Players)
	{
		// 严谨的防崩校验：万一数组里的玩家刚刚被强杀，这里过滤掉无效指针
		if (!IsValid(Player)) continue;

		float DistanceSq = FVector::DistSquared(Player->GetActorLocation(), GetActorLocation());
		if (DistanceSq < ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			BestTarget = Player;
		}
	}

	return BestTarget;
}

void ASRExpGem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 拾取逻辑必须只在服务器运行
	if (!HasAuthority()) return;

	// 1. 寻找最近的玩家（如果还没有目标）
	if (!TargetPlayer)
	{
		// 【P0 性能优化】没目标时降频查找：每 0.2 秒找一次，
		// 而不是每帧遍历玩家列表（几百颗球时每帧遍历是白烧 CPU）
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime < NextSearchTime)
		{
			return;
		}
		NextSearchTime = CurrentTime + 0.2f;

		AActor* PlayerPawn = GetClosestPlayer(); 
		if (PlayerPawn)
		{
			float Dist = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
			if (Dist < MagnetRange)
			{
				TargetPlayer = PlayerPawn;
			}
		}
	}
	else
	{
		// 2. 磁吸飞行逻辑
		FVector Direction = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		AddActorWorldOffset(Direction * FlySpeed * DeltaTime);

		// 3. 彻底拾取判定
		float Dist = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
		if (Dist < CollectRange)
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPlayer);
			if (TargetASC && ExpEffectClass) 
			{
				FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
				Context.AddSourceObject(this);

				FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(ExpEffectClass, 1.0f, Context);
				if (SpecHandle.IsValid())
				{
					// 将经验球的数值 (ExpAmount) 动态注入到 GE 里 
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("SetByCaller.Exp")), ExpAmount);
					TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}

			// 【P0 对象池】回收进池"睡觉"，而不是 Destroy
			if (USRSwarmSubsystem* Swarm = USRSwarmSubsystem::Get(GetWorld()))
			{
				Swarm->ReleaseGem(this);
			}
		}
	}
}
