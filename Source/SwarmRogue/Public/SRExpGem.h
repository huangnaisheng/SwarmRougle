#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SRExpGem.generated.h"

class UGameplayEffect;



UCLASS()
class SWARMROGUE_API ASRExpGem : public AActor
{
	GENERATED_BODY()

public:
	ASRExpGem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ==========================================
	// 对象池支持（P0）
	// ==========================================

	// 服务器唤醒：挪到出生点、清空目标、显示
	void OnAcquiredFromPool(const FVector& SpawnLocation);

	// 服务器回收：清空目标、隐藏、挪藏点
	void OnReleasedToPool();

protected:
	virtual void BeginPlay() override;

	AActor* GetClosestPlayer();

	// 经验值
	UPROPERTY(EditAnywhere, Category = "Stats")
	float ExpAmount = 100.f;

	// ==========================================
	// 【新增】提供给玩家的经验值增益效果 (Gameplay Effect)
	// ==========================================
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Drop")
	TSubclassOf<UGameplayEffect> ExpEffectClass;

	// ==========================================
	// 池化网络同步（P0）
	// ==========================================

	// 是否在池里"睡觉"（复制给客户端，客户端跟着隐藏/显示）
	UPROPERTY(ReplicatedUsing = OnRep_Pooled)
	bool bPooled = false;

	// 唤醒时的出生点。声明在 bPooled 之前，保证客户端触发 OnRep_Pooled 时位置已就绪
	UPROPERTY(Replicated)
	FVector PendingSpawnLocation = FVector::ZeroVector;

	UFUNCTION()
	void OnRep_Pooled();

public:
	// 每帧执行，处理磁吸逻辑
	virtual void Tick(float DeltaTime) override;

	// 被玩家“吸入”的范围
	float MagnetRange = 400.f;
	// 彻底捡起的范围
	float CollectRange = 50.f;
	// 飞行速度
	float FlySpeed = 600.f;

private:
	// 记录正在吸我的玩家
	UPROPERTY()
	AActor* TargetPlayer = nullptr;

	// 【P0 性能优化】下次允许查找目标的世界时间戳。
	// 没目标时不再每帧遍历玩家列表，降频到每 0.2 秒找一次
	float NextSearchTime = 0.f;
};
