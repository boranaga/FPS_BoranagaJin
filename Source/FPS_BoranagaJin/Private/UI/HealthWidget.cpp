#include "UI/HealthWidget.h"

#include "Components/ProgressBar.h"

void UHealthWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    SetUIType(EUIType::Health);
}

void UHealthWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UHealthWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UHealthWidget::InitHealthBar(float maxhealth)
{
    MaxHealth = maxhealth;
    HealthBar->SetPercent(1.f);
}

void UHealthWidget::SetHealthBarPercent(float const maxhealth, float const currhealth)
{
    MaxHealth = maxhealth;
    CurrHealth = currhealth;
    HealthBar->SetPercent(CurrHealth / MaxHealth);
}

