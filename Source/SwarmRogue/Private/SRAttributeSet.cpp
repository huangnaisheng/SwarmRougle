// Fill out your copyright notice in the Description page of Project Settings.


#include "SRAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "SRPlayerController.h"

USRAttributeSet::USRAttributeSet()
{
	// 给个初始默认值，防止空指针
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMoveSpeed(600.f);
	InitAttackRangeMultiplier(1.f);
	InitCurrentExp(0.f);
	InitPlayerLevel(1.f);
	InitMaxExp(100.f);
	
}

void USRAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 注册属性同步，REPNOTIFY_Always 确保即使数值没变也会通知客户端
	DOREPLIFETIME_CONDITION_NOTIFY(USRAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USRAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USRAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USRAttributeSet, AttackRangeMultiplier, COND_None, REPNOTIFY_Always);

	DOREPLIFETIME_CONDITION_NOTIFY(USRAttributeSet, CurrentExp, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USRAttributeSet, MaxExp, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USRAttributeSet, PlayerLevel, COND_None, REPNOTIFY_Always);
}

void USRAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USRAttributeSet, Health, OldHealth);
}

void USRAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USRAttributeSet, MaxHealth, OldMaxHealth);
}

void USRAttributeSet::OnRep_AttackRangeMultiplier(const FGameplayAttributeData& OldAttackRangeMultiplier)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USRAttributeSet, AttackRangeMultiplier, OldAttackRangeMultiplier);
}

void USRAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USRAttributeSet, MoveSpeed, OldMoveSpeed);
}

void USRAttributeSet::OnRep_CurrentExp(const FGameplayAttributeData& OldCurrentExp)
{
}

void USRAttributeSet::OnRep_MaxExp(const FGameplayAttributeData& OldMaxExp)
{
}

void USRAttributeSet::OnRep_PlayerLevel(const FGameplayAttributeData& OldPlayerLevel)
{
}

void USRAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 比如：血量永远不能小于 0，也不能大于 MaxHealth
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void USRAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// 再次 Clamp 实际值
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

		// 如果血量归零，在这里调用你的 HandlePlayerDeath 逻辑！
		if (GetHealth() <= 0.f)
		{
			// 可以在这里通过 Data.Target 拿到对应的 Actor 并广播死亡
			UE_LOG(LogTemp, Warning, TEXT("角色生命值归零！"));
		}
	}

	if (Data.EvaluatedData.Attribute == GetMoveSpeedAttribute())
	{
		// 获取受影响的 Actor (AvatarActor)
		if (ACharacter* OwningCharacter = Cast<ACharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor()))
		{
			OwningCharacter->GetCharacterMovement()->MaxWalkSpeed = GetMoveSpeed();
		}
	}

	// ==========================================
	// 拦截并处理经验值的变动
	// ==========================================
	if (Data.EvaluatedData.Attribute == GetCurrentExpAttribute())
	{
		// 只有服务器能处理升级逻辑
		if (GetOwningAbilitySystemComponent()->GetOwnerRole() == ROLE_Authority)
		{
			float TempCurrentExp = GetCurrentExp();
			float TempMaxExp = GetMaxExp();


			//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("经验注入成功！当前经验: %f / %f"), TempCurrentExp, TempMaxExp));

			// 检查是否满足升级条件 (使用 while 是为了防止一口气吃太多经验连升两级)
			while (TempCurrentExp >= TempMaxExp)
			{
				// 1. 扣除升级所需经验，保留溢出的经验
				TempCurrentExp -= TempMaxExp;

				// 2. 等级 + 1
				SetPlayerLevel(GetPlayerLevel() + 1.0f);

				// 3. 提升下一级的经验上限 (比如每级所需经验是上一级的 1.2 倍)
				TempMaxExp = TempMaxExp * 1.2f;
				SetMaxExp(TempMaxExp);


				AActor* OwnerActor = GetOwningAbilitySystemComponent()->GetAvatarActor();
				if (APawn* OwnerPawn = Cast<APawn>(OwnerActor))
				{

					if (ASRPlayerController* PC = Cast<ASRPlayerController>(OwnerPawn->GetController()))
					{

						PC->Server_GenerateUpgrades(FMath::TruncToInt(GetPlayerLevel()));
					}
				}
				// 通过 GameplayEvent，或者直接在 Character 里拉一个 Delegate
				UE_LOG(LogTemp, Warning, TEXT("玩家升到了 %f 级！新上限：%f"), GetPlayerLevel(), GetMaxExp());
			}
			// 将结算后的最终经验值写回属性
			SetCurrentExp(TempCurrentExp);
		}
	}
}