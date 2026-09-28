// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DataTable.h"
#include "SRPlayerController.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct FSRUpgradeData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FName UpgradeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|UI")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|UI")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|UI")
	class UTexture2D* Icon; // ¼Ó¸ö class ·ÀÖ¹±¨´í

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Logic")
	TSubclassOf<UGameplayEffect> EffectClass;
};


/**
 * 
 */
UCLASS()
class SWARMROGUE_API ASRPlayerController : public APlayerController
{
	GENERATED_BODY()
public:

	UFUNCTION(Client, Reliable)
	void Client_ShowLevelUpUI(int32 NewLevel, const TArray<FName>& UpgradeIDs);

	
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void BP_ShowLevelUpUI(int32 NewLevel, const TArray<FName>& UpgradeIDs);

	UFUNCTION()
	void Server_GenerateUpgrades(int32 NewLevel);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Upgrade")
	void Server_ApplyUpgrade(FName SelectedUpgradeID);


protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	class UDataTable* UpgradeDataTable;
};
