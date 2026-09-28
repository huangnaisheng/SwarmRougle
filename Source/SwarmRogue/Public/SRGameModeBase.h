// SRGameModeBase.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SRGameModeBase.generated.h"

UCLASS()
class SWARMROGUE_API ASRGameModeBase : public AGameModeBase
{
    GENERATED_BODY()

public:
    // 处理角色肉体死亡 (可以由 Character 或 PlayerState 的 Health 属性变更触发)
    UFUNCTION(BlueprintCallable, Category = "Game Mode")
    virtual void HandlePlayerDeath(AController* PlayerController);

    // 重塑肉体
    UFUNCTION(BlueprintCallable, Category = "Game Mode")
    virtual void RespawnPlayer(AController* PlayerController);

    // 推进波次
    UFUNCTION(BlueprintCallable, Category = "Game Mode")
    virtual void StartNextWave();
};