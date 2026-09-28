#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SREnemyBase.generated.h"


class UGameplayEffect;
class ASRExpGem;

UCLASS()
class SWARMROGUE_API ASREnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	ASREnemyBase();

	// ==========================================
	// 基础属性 (联机同步)
	// ==========================================
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 【P0 修正】喂移动输入必须每帧（CMC 输入是瞬时指令，断供会刹车减速）；
	// 旋转与空间网格维护在 Tick 内部低频化（每 0.1 秒一次）
	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// 对象池支持（P0）
	// ==========================================

	// 服务器唤醒：重置战斗状态、挪到出生点、显示、重新入网、重启寻路
	void OnAcquiredFromPool(const FVector& SpawnLocation);

	// 服务器回收：停止一切、隐藏、退出网格、挪到藏点"睡觉"
	void OnReleasedToPool();

	// 供 SRSwarmSubsystem 记录/读取这只怪当前所在的网格坐标
	void SetSwarmGridCell(const FIntVector2& InCell) { SwarmGridCell = InCell; }
	const FIntVector2& GetSwarmGridCell() const { return SwarmGridCell; }

protected:
	virtual void BeginPlay() override;

	// 真正被销毁（地图卸载/编辑器结束）时，必须把自己从空间网格注销
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ==========================================
	// 爆经验系统
	// ==========================================
	// 经验球的蓝图类引用
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop")
	TSubclassOf<ASRExpGem> ExpGemClass;

	// ==========================================
	// 扣血系统
	// ==========================================
	// 怪物当前生命值 (同步)
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Attributes")
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes")
	float MaxHealth = 50.f;

	// 碰到玩家时扣多少血
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float CollisionDamage = 10.f;

	// 这里预留一个 GE 类，当碰到玩家时，我们会实例化这个 GE 丢给玩家
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UFUNCTION()
	void OnRep_Health();

	// 处理受击逻辑 (由玩家的技能调用)
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void TakeDamageFromPlayer(float DamageAmount);

	// ==========================================
	// 移动和寻路系统
	// ==========================================
	// 寻路更新频率（秒），0.1s ~ 0.2s 即可，不需要每帧寻路
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float NavigationUpdateInterval = 0.2f;

	// 寻路定时器句柄
	FTimerHandle NavUpdateTimerHandle;

	// 启动/停止寻路定时器（BeginPlay 与池化复活共用）
	void StartNavigation();
	void StopNavigation();

	// 真正的寻路执行函数
	void UpdateNavigation();

	// 辅助函数：获取最近的玩家
	AActor* GetClosestPlayer();

	// 移动速度缓存（方便后期根据波次难度动态修改）
	UPROPERTY(EditAnywhere, Category = "AI")
	float MovementSpeed = 300.f;

	//缓存当前的移动方向
	UPROPERTY()
	FVector CurrentMoveDirection = FVector::ZeroVector;

	// 缓存当前的朝向（面向玩家的方向）
	UPROPERTY()
	FVector CurrentLookDirection = FVector::ZeroVector;


	// ==========================================
	// 跨界伤害投递 (GAS)
	// ==========================================
 
	// 攻击距离 (距离平方，省去开根号)
	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRange = 90.f;

	// 记录上一次咬人的时间戳
	float LastAttackTime = 0.f;

	// 伤害频率：每隔多少秒啃一次玩家
	UPROPERTY(EditAnywhere, Category = "Combat")
	float DamageInterval = 1.0f;

	// 真正执行伤害投递的私有函数
	void ApplyDamageToTarget(AActor* Target);

	// ==========================================
	// 群落排斥 (Swarm Separation)
	// ==========================================

	// 排斥半径：多近开始排斥？
	UPROPERTY(EditAnywhere, Category = "AI")
	float SeparationRadius = 80.f;

	// 排斥权重：推力有多大？
	UPROPERTY(EditAnywhere, Category = "AI")
	float SeparationWeight = 1.5f;

	// 计算周围怪物的排斥向量
	FVector CalculateSeparationVector();

	// ==========================================
	// 池化网络同步（P0）
	// ==========================================

	// 是否在池里"睡觉"。用 ReplicatedUsing 同步：
	// 客户端收到变化后通过 OnRep_Pooled 跟着隐藏/显示，保证两端表现一致
	UPROPERTY(ReplicatedUsing = OnRep_Pooled)
	bool bPooled = false;

	// 唤醒时的出生点。故意声明在 bPooled 之前：
	// UE 按声明顺序复制属性，客户端触发 OnRep_Pooled 时这个位置一定已经就绪，
	// 客户端才能"瞬间对齐"，而不是看到角色从藏点飞过来的撕裂
	UPROPERTY(Replicated)
	FVector PendingSpawnLocation = FVector::ZeroVector;

	UFUNCTION()
	void OnRep_Pooled();

	// 自己当前所在的网格坐标（INT32_MIN 表示"不在网格"）
	FIntVector2 SwarmGridCell = FIntVector2(INT32_MIN, INT32_MIN);

	// 排斥查询的复用容器：避免每次查询都重新分配内存
	TArray<ASREnemyBase*> NeighborCache;

	// 【P0】低频工作累计器：旋转与网格维护每 0.1 秒才做一次
	float LowFreqAccumulator = 0.f;
};
