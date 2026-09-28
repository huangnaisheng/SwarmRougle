#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "SRAssetManager.generated.h"

/**
 * 自定义资产管理器
 * 职责：在游戏最早期启动全局系统（如 GAS），并负责肉鸽游戏中遗物、技能的异步加载
 */
UCLASS()
class SWARMROGUE_API USRAssetManager : public UAssetManager
{
    GENERATED_BODY()

public:
    // 提供一个获取单例的静态快捷方法，方便在任何地方调用
    static USRAssetManager& Get();

protected:
    // 这是引擎生命周期中最关键的钩子之一，在游戏启动初始化时触发
    virtual void StartInitialLoading() override;
};