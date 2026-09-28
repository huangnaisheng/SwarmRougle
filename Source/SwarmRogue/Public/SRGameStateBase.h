// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SRGameStateBase.generated.h"


// 声明多播委托，彻底解耦 UI
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveChangedSignature, int32, NewWave);

/**
 * 
 */
UCLASS()
class SWARMROGUE_API ASRGameStateBase : public AGameStateBase
{
    GENERATED_BODY()

public:
    ASRGameStateBase();

    // 注册网络同步属性必须重写此函数
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 暴露给服务器调用的接口
    void SetCurrentWave(int32 NewWave);

    // 给 UI 监听的委托
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnWaveChangedSignature OnWaveChangedDelegate;

    // 核心网络同步变量，使用 RepNotify
    UPROPERTY(ReplicatedUsing = OnRep_CurrentWave, Transient, BlueprintReadOnly, Category = "Game State")
    int32 CurrentWave;

    // 供玩家出生时调用
    void RegisterPlayer(APawn* Player);

    // 供玩家死亡/离线时调用              
    void UnregisterPlayer(APawn* Player);

    // 供小怪寻路时读取
    const TArray<APawn*>& GetActivePlayers() const { return ActivePlayers; }

protected:

    // 活跃玩家列表（仅服务器维护即可，客户端小怪不需要自己寻路）
    // 使用 UPROPERTY 防止被垃圾回收 (GC) 意外清理
    UPROPERTY(Transient)
    TArray<TObjectPtr<APawn>> ActivePlayers;

    UFUNCTION()
    void OnRep_CurrentWave(int32 OldWave);
};