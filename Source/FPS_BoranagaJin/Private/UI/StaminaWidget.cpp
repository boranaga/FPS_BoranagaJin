


#include "UI/StaminaWidget.h"

// #include "InterchangeResult.h"
#include "Items/Projectile/Projectile.h"
#include "Characters/Player/CharacterPlayer.h"
#include "Characters/Player/PlayerMovementComponent.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "Components//TextBlock.h"


void UStaminaWidget::NativeConstruct()
{
    Super::NativeConstruct();

    //UE_LOG(LogTemp, Error, TEXT("UWeaponAimUIWidget::NativeConstruct()!!!"));

    //CharacterPlayer = Cast<ACharacterPlayer>(GetOwningPlayerPawn());
}

void UStaminaWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UStaminaWidget::InitStaminaBar(float maxstamina)
{
    MaxStamina = maxstamina;
    StaminaBar->SetPercent(1.f);

    UE_LOG(LogTemp, Error, TEXT("UStaminaWidget::InitStaminaBar(float maxstamina)!!!"));
}
void UStaminaWidget::SetStaminaBarPercent(float const Value)
{
    CurrStamina = Value;
    StaminaBar->SetPercent(CurrStamina/MaxStamina);
}

void UStaminaWidget::HideInGame(bool bHidden)
{
    if (bHidden)
        SetVisibility(ESlateVisibility::Hidden);
    else
        SetVisibility(ESlateVisibility::Visible);
}

void UStaminaWidget::PlayFadeAnimation()
{
    PlayAnimation(FadeInOutAnimation);
}



