// Fill out your copyright notice in the Description page of Project Settings.


#include "SRPlayerState.h"

ASRPlayerState::ASRPlayerState()
{
    // 创建 ASC 并强制设置网络同步
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);

    // 【关键】同屏怪海联机，玩家必须是 Mixed！
    // Server同步GE给Owner客户端，只同步表现Cue给其他客户端
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    //创建核心属性集 (会自动注册到 ASC)
    AttributeSet = CreateDefaultSubobject<USRAttributeSet>(TEXT("AttributeSet"));

    // 极度重要的网络频率优化！
    // 默认 PlayerState 的 NetUpdateFrequency 只有 1Hz (每秒1次)，
    // 这会导致客户端的 GAS 预测和属性同步看起来非常卡顿。必须提速！
    NetUpdateFrequency = 100.0f;
}



UAbilitySystemComponent* ASRPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}
