// Fill out your copyright notice in the Description page of Project Settings.


#include "SRPlayerController.h"
#include "Engine/DataTable.h"
#include "AbilitySystemComponent.h" 
#include "AbilitySystemBlueprintLibrary.h"

void ASRPlayerController::Client_ShowLevelUpUI_Implementation(int32 NewLevel, const TArray<FName>& UpgradeIDs)
{
	BP_ShowLevelUpUI(NewLevel, UpgradeIDs);
}


void ASRPlayerController::Server_GenerateUpgrades(int32 NewLevel)
{
	if (!HasAuthority()) return; // 铁律：只有服务器有权发牌
	if (!UpgradeDataTable) return;

	// 1. 拿到数据表里所有的行名字 (行名字就是我们在表里填的 UPG_Range, UPG_Speed 等)
	TArray<FName> AllRowNames = UpgradeDataTable->GetRowNames();
	TArray<FName> SelectedIDs;

	// 2. 联机防爆头：如果表里连 3 行数据都没有，有多少发多少，防止死循环
	int32 ChoicesCount = FMath::Min(3, AllRowNames.Num());

	// 3. 随机摇号算法 (Fisher-Yates 随机置换微缩版)
	while (SelectedIDs.Num() < ChoicesCount)
	{
		int32 RandomIndex = FMath::RandRange(0, AllRowNames.Num() - 1);
		FName PickedID = AllRowNames[RandomIndex];

		// 确保不重复
		if (!SelectedIDs.Contains(PickedID))
		{
			SelectedIDs.Add(PickedID);
		}
	}

	//  摇号完毕！直接拨通长途电话，把选中的 3 个 ID 空投给客户端
	Client_ShowLevelUpUI(NewLevel, SelectedIDs);
}

void ASRPlayerController::Server_ApplyUpgrade_Implementation(FName SelectedUpgradeID)
{
	if (!UpgradeDataTable) return;

	FSRUpgradeData* UpgradeData = UpgradeDataTable->FindRow<FSRUpgradeData>(SelectedUpgradeID, TEXT("UpgradeContext"));

	if (UpgradeData && UpgradeData->EffectClass)
	{
	
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn()))
		{
			
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddInstigator(GetPawn(), GetPawn());

			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UpgradeData->EffectClass, 1.0f, Context);

			
			if (SpecHandle.IsValid())
			{
				
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}
