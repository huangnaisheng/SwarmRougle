// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "SRAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 *
 */
UCLASS()
class SWARMROGUE_API USRAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	USRAttributeSet();

	// ==========================================
	// 核心属性声明（必须带 ReplicatedUsing）
	// ==========================================

	// 生命值
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(USRAttributeSet, Health)

		// 最大生命值
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(USRAttributeSet, MaxHealth)


	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackRangeMultiplier)
	FGameplayAttributeData AttackRangeMultiplier;
	ATTRIBUTE_ACCESSORS(USRAttributeSet, AttackRangeMultiplier)

		// 移动速度（对于肉鸽游戏至关重要）
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(USRAttributeSet, MoveSpeed)

	
	// Exp
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentExp, Category = "Attributes")
	FGameplayAttributeData CurrentExp;
	ATTRIBUTE_ACCESSORS(USRAttributeSet, CurrentExp)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxExp, Category = "Attributes")
	FGameplayAttributeData MaxExp;
	ATTRIBUTE_ACCESSORS(USRAttributeSet, MaxExp)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PlayerLevel, Category = "Attributes")
	FGameplayAttributeData PlayerLevel;
	ATTRIBUTE_ACCESSORS(USRAttributeSet, PlayerLevel)

	// ==========================================
	// 网络同步必须重写的函数
	// ==========================================
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 当属性在服务器改变时，通知客户端的函数
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	void OnRep_AttackRangeMultiplier(const FGameplayAttributeData& OldAttackRangeMultiplier);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);

	UFUNCTION()
	virtual void OnRep_CurrentExp(const FGameplayAttributeData& OldCurrentExp);

	UFUNCTION()
	virtual void OnRep_MaxExp(const FGameplayAttributeData& OldMaxExp);

	UFUNCTION()
	virtual void OnRep_PlayerLevel(const FGameplayAttributeData& OldPlayerLevel);


	// 在属性真正修改前触发（用于限制数值，比如血量不能超过最大值）
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// 在 GameplayEffect 执行后触发（处理伤害、死亡逻辑的最佳地点）
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
};

