


#include "UI/AmmoCounterWidget.h"

#include "Components/TextBlock.h"

void UAmmoCounterWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetUIType(EUIType::UIType_AmmoCounter);
}

void UAmmoCounterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	//SetUIType(EUIType::UIType_AmmoCounter);
}

void UAmmoCounterWidget::UpdateAmmoCount(int32 NewAmmoCount)
{
	AmmoCount->SetText(FText::FromString(FString::Printf(TEXT("%d"), NewAmmoCount)));
}

void UAmmoCounterWidget::UpdateTotalAmmo(int32 NewTotalAmmo)
{
	TotalAmmo->SetText(FText::FromString(FString::Printf(TEXT("%d"), NewTotalAmmo)));
}

