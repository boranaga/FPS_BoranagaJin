


#include "UI/AmmoCounterWidget.h"
#include "Components/TextBlock.h"

void UAmmoCounterWidget::UpdateAmmoCount(int32 NewAmmoCount)
{
	AmmoCount->SetText(FText::FromString(FString::Printf(TEXT("%d"), NewAmmoCount)));
}

void UAmmoCounterWidget::UpdateTotalAmmo(int32 NewTotalAmmo)
{
	TotalAmmo->SetText(FText::FromString(FString::Printf(TEXT("%d"), NewTotalAmmo)));
}

