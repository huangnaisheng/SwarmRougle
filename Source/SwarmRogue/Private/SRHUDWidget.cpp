#include "SRHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "SRAttributeSet.h" // 引入你的属性集

void USRHUDWidget::InitWidget(UAbilitySystemComponent* ASC)
{
	if (!ASC) return;

	OwnerASC = ASC;

	// 1. 绑定 GAS 属性变更委托
	// 只要服务端的数值变了同步过来，或者本地预测修改了数值，这里就会立刻触发
	OwnerASC->GetGameplayAttributeValueChangeDelegate(USRAttributeSet::GetHealthAttribute()).AddUObject(this, &USRHUDWidget::HealthChanged);
	OwnerASC->GetGameplayAttributeValueChangeDelegate(USRAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &USRHUDWidget::MaxHealthChanged);

	// 2. 首次初始化 UI 表现
	// 委托只有在数值变化时才触发，所以刚进游戏时我们需要手动推一次当前值给 UI
	bool bFoundHealth = false;
	float CurrentHealth = OwnerASC->GetGameplayAttributeValue(USRAttributeSet::GetHealthAttribute(), bFoundHealth);

	bool bFoundMaxHealth = false;
	float CurrentMaxHealth = OwnerASC->GetGameplayAttributeValue(USRAttributeSet::GetMaxHealthAttribute(), bFoundMaxHealth);

	if (bFoundHealth && bFoundMaxHealth)
	{
		OnHealthUpdated(CurrentHealth, CurrentMaxHealth);
	}
}

void USRHUDWidget::HealthChanged(const FOnAttributeChangeData& Data)
{
	// 当当前血量改变时，获取最大血量，并触发蓝图事件
	if (OwnerASC)
	{
		bool bFound = false;
		float MaxHealth = OwnerASC->GetGameplayAttributeValue(USRAttributeSet::GetMaxHealthAttribute(), bFound);
		OnHealthUpdated(Data.NewValue, MaxHealth);
	}
}

void USRHUDWidget::MaxHealthChanged(const FOnAttributeChangeData& Data)
{
	// 当最大血量改变时，获取当前血量，并触发蓝图事件
	if (OwnerASC)
	{
		bool bFound = false;
		float CurrentHealth = OwnerASC->GetGameplayAttributeValue(USRAttributeSet::GetHealthAttribute(), bFound);
		OnHealthUpdated(CurrentHealth, Data.NewValue); // Data.NewValue 就是新的最大血量
	}
}