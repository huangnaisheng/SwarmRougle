#include "SREnemyBase.h"
#include "SRExpGem.h"
#include "SRGameStateBase.h"
#include "SRSwarmSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

ASREnemyBase::ASREnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// 【P0 修正版】Actor Tick 必须保持每帧！
	// AddMovementInput 是"瞬时指令"：CMC 每帧模拟时会消费并清零输入，
	// 一旦断供，CMC 会按 BrakingDeceleration 刹车，怪物就会走走停停、越走越慢。
	// 所以喂输入必须每帧进行；真正低频化的是旋转与网格维护（在 Tick 内部按 0.1 秒分批）
	PrimaryActorTick.bCanEverTick = true;

	//  开启同步
	bReplicates = true;
	SetReplicateMovement(true); // 怪物位移必须同步

	// 让小怪自动转身面向移动方向
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	
	// 强制设为 Custom 预设，以便手动接管所有通道
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Custom"));

	// 将自己设置为Pawn
	GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);

	//  默认忽略一切无用通道（相机射线、可见性、其他无关投射物）
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	// 必须阻挡世界静态（WorldStatic），否则小怪会直接穿透地板掉进虚空
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	// 恢复对 Pawn 通道的响应
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
}

void ASREnemyBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASREnemyBase, Health);

	// 【P0 对象池】注意顺序：先复制出生点，再复制池化状态。
	// UE 按这里注册的顺序复制，客户端触发 OnRep_Pooled 时 PendingSpawnLocation 已经就绪
	DOREPLIFETIME(ASREnemyBase, PendingSpawnLocation);
	DOREPLIFETIME(ASREnemyBase, bPooled);
}

void ASREnemyBase::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;

	// 客户端不需要 Tick：怪物的移动全由服务器驱动、通过网络同步。
	// 直接关掉客户端的 Tick，省掉每只怪每 0.1 秒一次的空转调用
	SetActorTickEnabled(HasAuthority());

	// ==========================================
	// 启动寻路引擎与移动参数配置
	// ==========================================
	if (HasAuthority())
	{
		// 初始化怪物的移动速度
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = MovementSpeed;
		}

		// 【P0】注册进空间哈希网格（只发生在服务器）
		if (USRSwarmSubsystem* Swarm = USRSwarmSubsystem::Get(GetWorld()))
		{
			Swarm->RegisterEnemy(this, GetActorLocation());
		}

		StartNavigation();
	}
}

void ASREnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 【P0】无论以什么方式真正销毁（地图卸载、编辑器停止、程序退出），
	// 都要把自己从空间哈希网格注销，防止网格里残留失效指针
	if (USRSwarmSubsystem* Swarm = USRSwarmSubsystem::Get(GetWorld()))
	{
		Swarm->UnregisterEnemy(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ASREnemyBase::StartNavigation()
{
	if (!HasAuthority()) return;

	// 开启定时寻路任务（带随机错峰机制，防服务器瞬间卡顿）
	float InitialDelay = FMath::RandRange(0.0f, NavigationUpdateInterval);
	GetWorldTimerManager().SetTimer(
		NavUpdateTimerHandle,
		this,
		&ASREnemyBase::UpdateNavigation,
		NavigationUpdateInterval,
		true,
		InitialDelay
	);
}

void ASREnemyBase::StopNavigation()
{
	// 回收进池时必须停掉寻路定时器，否则"睡着的怪"还在后台空转寻路
	GetWorldTimerManager().ClearTimer(NavUpdateTimerHandle);
}

// ==========================================
// 对象池：唤醒 / 回收（P0）
// ==========================================

void ASREnemyBase::OnAcquiredFromPool(const FVector& SpawnLocation)
{
	if (!HasAuthority()) return;

	// 1. 重置战斗状态（相当于一次干净的"出生"）
	Health = MaxHealth;
	CurrentMoveDirection = FVector::ZeroVector;
	CurrentLookDirection = FVector::ZeroVector;
	LastAttackTime = 0.f;
	LowFreqAccumulator = 0.f;

	// 2. 挪到出生点并唤醒移动模拟。
	//    StopMovementImmediately 清掉上次残留的速度与输入，
	//    MOVE_Walking 把 CMC 从"休眠模式"唤醒
	PendingSpawnLocation = SpawnLocation;
	SetActorLocation(SpawnLocation, false);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// 3. 服务器侧显示 + 恢复 Tick
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	// 4. 通知客户端"我活了"（bPooled 的复制会触发客户端 OnRep_Pooled）
	bPooled = false;

	// 5. 重新入网格 + 重启寻路
	if (USRSwarmSubsystem* Swarm = USRSwarmSubsystem::Get(GetWorld()))
	{
		Swarm->RegisterEnemy(this, SpawnLocation);
	}
	StartNavigation();
}

void ASREnemyBase::OnReleasedToPool()
{
	if (!HasAuthority()) return;

	// 1. 停止一切逻辑：寻路、移动、旋转
	StopNavigation();
	GetCharacterMovement()->StopMovementImmediately();

	// 2. 【关键防坑】把 CMC 切到 MOVE_None（完全休眠）。
	//    藏点在 Z=-10000 的空中，如果留着 MOVE_Walking，CMC 每帧都会做
	//    "地面检测 -> 找不到地板 -> 进入坠落"的模拟，
	//    几百只睡着的怪叠在藏点互相挤，会变成一场物理风暴
	GetCharacterMovement()->SetMovementMode(MOVE_None);

	// 3. 标记回收（复制给客户端）+ 服务器侧隐藏 + 停 Tick
	bPooled = true;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	// 4. 退出空间哈希网格
	if (USRSwarmSubsystem* Swarm = USRSwarmSubsystem::Get(GetWorld()))
	{
		Swarm->UnregisterEnemy(this);
	}

	// 5. 挪到藏点（此时已经隐藏，客户端看不到这次瞬移，不会有撕裂）
	//    注意：UWorld::OriginLocation 是 FIntVector，必须显式转 FVector 才能做加法
	const FVector HideSpot(
		GetWorld()->OriginLocation.X,
		GetWorld()->OriginLocation.Y,
		GetWorld()->OriginLocation.Z - 10000.f
	);
	SetActorLocation(HideSpot, false);
}

void ASREnemyBase::OnRep_Pooled()
{
	// 客户端表现同步：服务器说"回收"就隐藏，说"唤醒"就显示
	if (bPooled)
	{
		SetActorHiddenInGame(true);
		SetActorTickEnabled(false);
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->SetMovementMode(MOVE_None);
		}
	}
	else
	{
		// 【防撕裂】唤醒：瞬间对齐服务器给的出生点（此刻已复制到位），
		// 并告诉 CMC"网络平滑已完成"，禁止它把这次传送当成大位移慢慢插值
		// （否则玩家会看到怪物从地图外的藏点"飞"进来）
		SetActorLocation(PendingSpawnLocation, false);
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->SetMovementMode(MOVE_Walking);
			MoveComp->bNetworkSmoothingComplete = true;
		}
		SetActorHiddenInGame(false);
		SetActorTickEnabled(false); // 客户端保持不 Tick（纯服务器驱动 + 网络同步）
	}
}

void ASREnemyBase::TakeDamageFromPlayer(float DamageAmount)
{
	if (!HasAuthority())
	{
		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("⚠️ 拦截：非服务器，无法扣血！"));
		return;
	}

	Health = FMath::Clamp(Health - DamageAmount, 0.f, MaxHealth);

	// 探照灯 2：看看血量扣了没
	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, FString::Printf(TEXT("🩸 怪物受到 %f 伤害，剩余血量: %f"), DamageAmount, Health));

	if (Health <= 0.f)
	{
		USRSwarmSubsystem* Swarm = USRSwarmSubsystem::Get(GetWorld());
		if (Swarm)
		{
			// 【P0 对象池】经验球也从池里拿，而不是 SpawnActor
			Swarm->AcquireGem(ExpGemClass, GetActorLocation());

			// 【P0 对象池】回收自己进池"睡觉"，而不是 Destroy
			Swarm->ReleaseEnemy(this);
		}
	}
}

AActor* ASREnemyBase::GetClosestPlayer()
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

void ASREnemyBase::ApplyDamageToTarget(AActor* Target)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (TargetASC && DamageEffectClass)
	{
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		Context.AddSourceObject(this);

		// 创建 Spec。注意：这里可以根据小怪的当前强度动态缩放效果等级
		FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
		if (SpecHandle.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void ASREnemyBase::UpdateNavigation()
{
	if (!HasAuthority()) return;

	AActor* ClosestPlayer = GetClosestPlayer();

	if (ClosestPlayer)
	{

		// 1. 获取距离的平方
		float DistanceSq = FVector::DistSquared(ClosestPlayer->GetActorLocation(), GetActorLocation());
		float AttackRangeSq = AttackRange * AttackRange;

		FVector PureTargetVector = (ClosestPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();

		// 2. 状态分化：攻击 or 追击
		if (DistanceSq <= AttackRangeSq)
		{
			float CurrentTime = GetWorld()->GetTimeSeconds();
			if (CurrentTime - LastAttackTime >= DamageInterval)
			{
				ApplyDamageToTarget(ClosestPlayer);
				LastAttackTime = CurrentTime;
			}

			// 攻击时：腿停下，但眼睛继续死死盯着玩家
			CurrentMoveDirection = FVector::ZeroVector;
			CurrentLookDirection = PureTargetVector;
		}
		else
		{
			// 追击状态
			FVector SeparationVector = CalculateSeparationVector();

			// 腿的控制权：目标 + 排斥力
			CurrentMoveDirection = (PureTargetVector + (SeparationVector * SeparationWeight)).GetSafeNormal();

			// 眼睛的控制权：只看玩家，绝不看排斥方向！
			CurrentLookDirection = PureTargetVector;
		}
	}
}

FVector ASREnemyBase::CalculateSeparationVector()
{
	FVector OffSet(0.f);

	// 【P0 性能优化】用子系统的空间哈希网格查邻居。
	// 原来这里是 OverlapMultiByObjectType 物理场景查询：
	// 1000 只怪 = 每秒 5000 次物理查询。现在变成 5000 次哈希查表，
	// 只遍历 3x3 个格子，便宜 1~2 个数量级
	USRSwarmSubsystem* Swarm = USRSwarmSubsystem::Get(GetWorld());
	if (!Swarm) return OffSet;

	NeighborCache.Reset();
	Swarm->QueryNeighbors(GetActorLocation(), SeparationRadius, NeighborCache, this);

	for (ASREnemyBase* Neighbor : NeighborCache)
	{
		if (!Neighbor) continue;

		FVector Diff = GetActorLocation() - Neighbor->GetActorLocation();
		float Distance = Diff.Size();
		if (Distance > 0.1f)
		{
			// 正常排斥：距离越近，推力越大
			OffSet += (Diff.GetSafeNormal() / (Distance / SeparationRadius));
		}
		else
		{
			// 奇点排斥：如果两只怪完全叠在一起，给它们一个随机的量子推力炸开
			OffSet += FVector(FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f), 0.f).GetSafeNormal() * 5.0f;
		}
	}
	return OffSet;
}


void ASREnemyBase::OnRep_Health()
{
	// 客户端表现：血量归零立刻隐藏。
	// 服务器稍后才会真正把怪回收进池，客户端先隐藏，
	// 视觉上就不会出现"尸体站桩等回收"的延迟
	if (Health <= 0.f)
	{
		SetActorHiddenInGame(true);
	}
}

void ASREnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 只有服务器需要驱动小怪移动，客户端靠网络同步位置
	if (!HasAuthority())
	{
		return;
	}

	// ==========================================
	// 1. 管腿：每帧持续喂输入（必须每帧！）
	//    CMC 的输入是"瞬时"的：每帧模拟时消费并清零，
	//    一旦断供，CMC 会按 BrakingDeceleration 刹车减速，
	//    怪物就会走走停停、越走越慢（P0 首版踩过的坑）
	// ==========================================
	if (!CurrentMoveDirection.IsNearlyZero())
	{
		AddMovementInput(CurrentMoveDirection, 1.0f);
	}

	// ==========================================
	// 2. 低频工作：每 0.1 秒才做一次
	//    旋转与网格维护不需要每帧更新，这才是真正能省下的开销
	// ==========================================
	LowFreqAccumulator += DeltaTime;
	if (LowFreqAccumulator < 0.1f)
	{
		return;
	}
	LowFreqAccumulator = 0.f;

	// 2a. 维护自己在空间网格里的归属（跨格子才搬运，否则零成本）
	if (USRSwarmSubsystem* Swarm = USRSwarmSubsystem::Get(GetWorld()))
	{
		Swarm->UpdateEnemyCell(this, GetActorLocation());
	}

	// 2b. 管眼睛：只要有视线目标，就强行转过去
	if (!CurrentLookDirection.IsNearlyZero())
	{
		// 这里使用 LookDirection 而不是 MoveDirection！
		FRotator TargetRotation = CurrentLookDirection.Rotation();

		TargetRotation.Pitch = 0.f;
		TargetRotation.Roll = 0.f;

		// 低频分支固定间隔 0.1 秒，RInterpTo 的 DeltaTime 传固定值更稳定
		FRotator SmoothRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, 0.1f, 15.f); // 15.f 让转身更敏锐一点
		SetActorRotation(SmoothRotation);
	}
}
