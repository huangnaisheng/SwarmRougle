// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "SRAttributeSet.h"
#include "SRPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class SWARMROGUE_API ASRPlayerState : public APlayerState,public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
    ASRPlayerState();

    // 实现 IAbilitySystemInterface 接口
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintCallable, Category = "GAS")
    UAttributeSet* GetAttributeSet() const { return AttributeSet; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    TObjectPtr<UAttributeSet> AttributeSet;
};
