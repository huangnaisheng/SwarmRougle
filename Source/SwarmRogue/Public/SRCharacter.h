
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "SRPlayerState.h"
#include "InputActionValue.h" // 增加 Enhanced Input 的值解析
#include "GameplayTagContainer.h"
#include "SRHUDWidget.h"
#include "SRCharacter.generated.h"

class UInputAction;
class UAbilitySystemComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;


UCLASS()
class SWARMROGUE_API ASRCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ASRCharacter();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void Destroyed() override;

    bool bIsASCInputBound;

    // 尝试绑定输入，只有当 ASC 和 InputComponent 都准备好时才真正执行
    void BindASCInput();

    // ==========================================
    // GAS 技能装配系统
    // ==========================================

    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Abilities")
    TArray<TSubclassOf<class UGameplayAbility>> DefaultAbilities;

    // 赋予初始技能的内部函数
    void GiveDefaultAbilities();


    // ==========================================
    // 攻击按键的 GAS 映射
    // ==========================================

    // 用一个 Tag 来代表“攻击指令”，而不是硬编码技能类
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Input")
    FGameplayTag AttackAbilityTag;


    // ==========================================
    // Enhanced Input 测试
    // ==========================================
    UPROPERTY(EditDefaultsOnly, Category = "GAS Input")
    UInputAction* AttackInputAction;

    // 真正的按键触发逻辑
 /*   void OnAttackInputPressed();*/

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> TopDownCameraComponent;

    UPROPERTY(EditDefaultsOnly, Category = "GAS Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "GAS Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<USRHUDWidget> HUDWidgetClass;

    // 保存创建出来的 UI 实例
    UPROPERTY()
    TObjectPtr<USRHUDWidget> HUDWidgetInstance;

    // 处理移动逻辑
    void Move(const FInputActionValue& Value);
};