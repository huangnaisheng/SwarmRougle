#include "SRAssetManager.h"
#include "AbilitySystemGlobals.h" // GAS 全局静态类

USRAssetManager& USRAssetManager::Get()
{
    check(GEngine);

    // 尝试获取当前引擎正在使用的 AssetManager
    USRAssetManager* Singleton = Cast<USRAssetManager>(GEngine->AssetManager);

    if (Singleton)
    {
        return *Singleton;
    }

    UE_LOG(LogTemp, Fatal, TEXT("致命错误: 未在 DefaultEngine.ini 中配置 USRAssetManager!"));

    return *NewObject<USRAssetManager>(); // 仅为消除编译警告，实际运行不到这里
}

void USRAssetManager::StartInitialLoading()
{
    Super::StartInitialLoading();

    // =======================================================
    // 【联机 GAS 的命脉】
    // 这一步会初始化 GAS 的全局数据、结构体脚本（ScriptStructs）。
    // =======================================================
    UAbilitySystemGlobals::Get().InitGlobalData();

    //UE_LOG(LogTemp, Log, TEXT("USRAssetManager: 已经完成初始化"));

    // 未来的 DataRegistry (数据注册表) 或全局 GameplayTags 的加载逻辑，也会写在这里
}
