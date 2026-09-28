#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SRSwarmSubsystem.generated.h"

class ASREnemyBase;
class ASRExpGem;

/**
 * 蜂群子系统（Swarm Subsystem）—— P0 性能优化的地基
 *
 * 它是 UWorldSubsystem：每张地图（每个 UWorld）自动存在一个实例，
 * 随地图加载而创建、随地图卸载而销毁，用 Get(World) 即可拿到。
 *
 * 职责 1：空间哈希网格
 *   把地图切成 100cm 的格子，敌人注册/注销/跨格更新。
 *   蜂群排斥查邻居时只翻 3x3 个格子，替代"每只怪每 0.2 秒一次物理场景查询"。
 *
 * 职责 2：对象池
 *   敌人和经验球死亡时"回收休眠"而不是 Destroy，下次生成直接"唤醒"。
 *   池里用弱指针存放，不干扰 GC，也不会出现悬垂指针。
 *
 * 网络说明：服务器和客户端各有一份实例，但我们只在服务器
 * （HasAuthority）填数据；客户端那份的网格和池永远是空的，天然安全。
 */
UCLASS()
class SWARMROGUE_API USRSwarmSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 快捷获取单例：USRSwarmSubsystem::Get(GetWorld())
	static USRSwarmSubsystem* Get(const UWorld* World);

	// 世界销毁时的清理（弱指针会自动失效，这里只是清空容器）
	virtual void Deinitialize() override;

	// ==================== 空间哈希网格 ====================

	// 敌人出生/唤醒时注册进网格
	void RegisterEnemy(ASREnemyBase* Enemy, const FVector& Location);

	// 敌人回收/真正销毁时从网格移除
	void UnregisterEnemy(ASREnemyBase* Enemy);

	// 敌人每次移动后调用：如果跨了格子才真正搬运，否则零成本
	void UpdateEnemyCell(ASREnemyBase* Enemy, const FVector& NewLocation);

	// 查询圆心 Radius 半径内的所有敌人（只翻 3x3 个格子）
	// OutNeighbors 由调用方提供容器（内部 Reset），方便复用成员数组避免堆分配
	void QueryNeighbors(const FVector& Center, float Radius, TArray<ASREnemyBase*>& OutNeighbors, AActor* IgnoreActor = nullptr);

	// ==================== 对象池 ====================

	// 唤醒一只敌人：优先从池里拿，池空才 SpawnActor（只发生在预热期）
	ASREnemyBase* AcquireEnemy(TSubclassOf<ASREnemyBase> EnemyClass, const FVector& Location);

	// 回收一只敌人进池
	void ReleaseEnemy(ASREnemyBase* Enemy);

	// 唤醒一颗经验球
	ASRExpGem* AcquireGem(TSubclassOf<ASRExpGem> GemClass, const FVector& Location);

	// 回收一颗经验球
	void ReleaseGem(ASRExpGem* Gem);

private:
	// 格子的世界尺寸（厘米）。
	// 敌人的排斥半径是 80，所以 100 的格子 + 周围 8 格足以覆盖整个查询圆
	static constexpr float GridCellSize = 100.f;

	// 世界坐标 -> 格子坐标（XY 平面就够，割草游戏约束在地面）
	FIntVector2 GetCellCoord(const FVector& Location) const;

	// 空间哈希网格：格子坐标 -> 格子里的敌人
	TMap<FIntVector2, TArray<TWeakObjectPtr<ASREnemyBase>>> Grid;

	// 按蓝图子类分开的对象池：
	// 不同蓝图的怪/球配置不同，不能混用（比如精英怪的属性不能拿去当小怪刷）
	TMap<TSubclassOf<ASREnemyBase>, TArray<TWeakObjectPtr<ASREnemyBase>>> EnemyPool;
	TMap<TSubclassOf<ASRExpGem>, TArray<TWeakObjectPtr<ASRExpGem>>> GemPool;
};
