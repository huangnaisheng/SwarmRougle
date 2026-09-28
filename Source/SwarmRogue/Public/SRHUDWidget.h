// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h" // 必须包含以使用 FOnAttributeChangeData
#include "SRHUDWidget.generated.h"

class UAbilitySystemComponent;

UCLASS()
class SWARMROGUE_API USRHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 暴露给外部（如 Character 或 PlayerController）调用的初始化接口
	UFUNCTION(BlueprintCallable, Category = "GAS|UI")
	void InitWidget(UAbilitySystemComponent* ASC);

protected:
	// 交给蓝图去实现的纯表现层事件
	// 当血量发生变化（或者初始化）时触发
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|UI")
	void OnHealthUpdated(float CurrentHealth, float MaxHealth);

private:
	// 绑定的内部回调函数，签名必须符合 GAS 委托的要求
	void HealthChanged(const FOnAttributeChangeData& Data);
	void MaxHealthChanged(const FOnAttributeChangeData& Data);

	// 缓存 ASC 指针
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> OwnerASC;
};