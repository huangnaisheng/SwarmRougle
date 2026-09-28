// Fill out your copyright notice in the Description page of Project Settings.

#include "SRCharacter.h"
#include "SRGameStateBase.h"
#include "SRPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h" // 用于加载 Mapping Context
#include "GameplayAbilitySpec.h" // 如果后续需要按 Spec 触发

// Sets default values
ASRCharacter::ASRCharacter()
{
    bIsASCInputBound = false;
    // 1. 彻底切断角色与控制器的旋转绑定 (让 Controller 只管逻辑，不管视觉)
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 2. 配置割草游戏的专属移动逻辑：角色会自动转向移动的方向
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp)
    {
        MoveComp->bOrientRotationToMovement = true; // 开启自动转向
        MoveComp->RotationRate = FRotator(0.0f, 800.0f, 0.0f); // 转向速度调快，手感更干脆
        MoveComp->bConstrainToPlane = true; // 俯视角通常固定在Z轴平面
        MoveComp->bSnapToPlaneAtStart = true;
    }

    // 3. 悬臂 (Spring Arm)：上帝视角的支架
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->SetUsingAbsoluteRotation(true); // 绝对旋转！角色怎么转，悬臂都不跟着转
    CameraBoom->TargetArmLength = 1200.f; // 镜头拉远
    CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); // 俯视 60 度角
    CameraBoom->bDoCollisionTest = false; // 俯视角不需要镜头碰撞防穿模，关掉省性能

    // 4. 摄像机
    TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    TopDownCameraComponent->bUsePawnControlRotation = false; // 相机自身不旋转
}

UAbilitySystemComponent* ASRCharacter::GetAbilitySystemComponent() const
{
    ASRPlayerState* PS = GetPlayerState<ASRPlayerState>();
    return PS ? PS->GetAbilitySystemComponent() : nullptr;
}


void ASRCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // 【Server 端初始化】
    ASRPlayerState* PS = GetPlayerState<ASRPlayerState>();
    if (PS)
    {
        // InitAbilityActorInfo(OwnerActor, AvatarActor)
        UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
        if (ASC)
        {
            ASC->InitAbilityActorInfo(PS, this);

            GiveDefaultAbilities();

            BindASCInput();
        }
    }

    // ==========================================
    // 集中式分发：向服务器汇报自己的存在
    // ==========================================
    if (HasAuthority())
    {
        if (ASRGameStateBase* GS = GetWorld()->GetGameState<ASRGameStateBase>())
        {
            GS->RegisterPlayer(this);
        }
    }


}

void ASRCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    // 【Client 端初始化】
    // 联机环境下，客户端的 PlayerState 同步会有延迟。
    // 必须在 OnRep_PlayerState 触发时，才说明数据到了，才能初始化 ASC。
    ASRPlayerState* PS = GetPlayerState<ASRPlayerState>();
    if (PS)
    {
        PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);

        // 可选：客户端在这里绑定 Enhanced Input 到 GAS
        BindASCInput(); 
    }
}


void ASRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // 1. 获取 Enhanced Input 组件
    UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EnhancedInputComp) return;

    if (MoveAction)
    {
        EnhancedInputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASRCharacter::Move);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("警告：MoveAction 为空，请检查 BP_SRCharacter 的蓝图配置！"));
    }

    // 2. 注册 Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("警告：DefaultMappingContext 为空！"));
            }
        }
    }
    BindASCInput();
}
void ASRCharacter::BindASCInput()
{
    // 1. 检查是否已经绑过
    if (bIsASCInputBound) return;

    // 2. 检查 ASC 
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("⚠️ 拦截：ASC 或 PlayerState 还没准备好"));
        return;
    }

    // 3. 检查 InputComponent (按键系统)
    if (!InputComponent)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("拦截：InputComponent 还没准备好"));
        return;
    }

    // 4. 检查是否为增强输入
    UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInputComp)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("致命拦截：输入组件不是 EnhancedInputComponent！"));
        return;
    }

    // 5. 检查控制器 (UI 必须依附于玩家控制器)
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, TEXT("拦截：PlayerController 尚未附身"));
        return;
    }

    // 6. 检查是否本地控制 (联机时，别人电脑上的你的分身不需要创建 UI)
    if (!IsLocallyControlled())
    {
        // 这个不需要报红，因为如果是别人的分身，拦截是正常的
        return;
    }

    if (HUDWidgetClass)
    {
        if (HUDWidgetInstance == nullptr)
        {
            // 传入刚校验过的 PC
            HUDWidgetInstance = CreateWidget<USRHUDWidget>(PC, HUDWidgetClass);

            if (HUDWidgetInstance)
            {
                HUDWidgetInstance->AddToViewport();
                HUDWidgetInstance->InitWidget(ASC);
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("UI 成功创建并挂载！血条已就绪！"));
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT(" 致命错误：CreateWidget 失败！请检查蓝图类！"));
            }
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT(" 致命错误：HUDWidgetClass 为空！蓝图配置丢失！"));
    }

    // 只有所有校验全过，才标记为完成
    bIsASCInputBound = true;
}

void ASRCharacter::GiveDefaultAbilities()
{
    // 只有服务器有权力赋予技能
    if (!HasAuthority()) return;

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (!ASC)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("❌ 死因2：ASC 为空！(PlayerState 可能未就绪)"));
        return;
    }

    if (DefaultAbilities.IsEmpty())
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("❌ 死因3：技能数组为空！请打开 BP_SRCharacter 重新填入 GA_Melee！"));
        return;
    }

    for (TSubclassOf<UGameplayAbility> AbilityClass : DefaultAbilities)
    {
        if (AbilityClass)
        {
            // FGameplayAbilitySpec 是技能的实例化包装，1 代表技能等级
            FGameplayAbilitySpecHandle SpecHandle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));

            bool bSuccess = ASC->TryActivateAbility(SpecHandle);

            if (bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("✅ 成功：自动攻击技能 [%s] 点火成功！"), *AbilityClass->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ 失败：技能 [%s] 已发放到身上，但无法激活！"), *AbilityClass->GetName());
            }
        }
    }
}

//void ASRCharacter::OnAttackInputPressed()
//{
//    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
//    if (ASC)
//    {
//        // 通过 GameplayTag 来触发技能，而不是硬写类名
//        if (AttackAbilityTag.IsValid())
//        {
//            ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AttackAbilityTag));
//        }
//        else
//        {
//            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("警告：未配置攻击技能的 Tag！"));
//        }
//    }
//}

// 真正的移动逻辑：固定世界坐标系的 WASD (上北下南左西右东)
void ASRCharacter::Move(const FInputActionValue& Value)
{
    // 获取二维输入向量 (WASD 或者摇杆)
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // 直接使用绝对的世界坐标系：X轴正方向为上(W)，Y轴正方向为右(D)
        const FVector ForwardDirection = FVector(1.0f, 0.0f, 0.0f);
        const FVector RightDirection = FVector(0.0f, 1.0f, 0.0f);

        // 注入移动输入
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void ASRCharacter::Destroyed()
{
    // 集中式分发：销毁前注销自己
    if (HasAuthority())
    {
        if (ASRGameStateBase* GS = GetWorld()->GetGameState<ASRGameStateBase>())
        {
            GS->UnregisterPlayer(this);
        }
    }

    Super::Destroyed();
}