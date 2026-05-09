


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

    CharacterPlayer = Cast<ACharacterPlayer>(GetOwningPlayerPawn());
}

void UStaminaWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UpdateHitIndicator(InDeltaTime);

    SetDashGauge();
    fDashGauge = CharacterPlayer->GetPlayerMovementComponent()->GetDashGauge();

    // DashGauge UI 업데이트 함수 호출
    if (CharacterPlayer)
    {
        UpdateDashUI(fDashGauge);
    }
}

#pragma region HitIndicator
//void UStaminaWidget::SetUpAimUIDelegateBinding(AProjectile* Projectile)
//{
//    Projectile->OnHeadShot.BindUObject(this, &UWeaponAimUIWidget::HeadShot);
//    Projectile->OnBodyShot.BindUObject(this, &UWeaponAimUIWidget::BodyShot);
//}

void UStaminaWidget::HeadShot()
{
    //UE_LOG(LogTemp, Error, TEXT("UWeaponAimUIWidget::HeadShot()!!!"));

    SetNormalOvelayInvisible();

    bIsIndicatingCriticalHit = true;
    CriticalOverlayFadeOutTimer = 0.f;
    CriticalOverlay->SetRenderOpacity(0.f);
    CriticalOverlay->SetVisibility(ESlateVisibility::Visible);
}

void UStaminaWidget::BodyShot()
{
    //UE_LOG(LogTemp, Error, TEXT("UWeaponAimUIWidget::BodyShot()!!!"));

    SetCriticalOvelayInvisible();

    bIsIndicatingNormalHit = true;
    NormalOverlayFadeOutTimer = 0.f;
    NormalOverlay->SetRenderOpacity(0.f);
    NormalOverlay->SetVisibility(ESlateVisibility::Visible);
}

void UStaminaWidget::FadeInCriticalOverlay(float DeltaTime)
{
    float InterpOpacity = FMath::FInterpTo(CurrentCriticalOverlayOpacity, TargetCriticalOverlayOpacity, DeltaTime, CriticalOverlayFadeInSpeed);
    CurrentCriticalOverlayOpacity = InterpOpacity;
    CriticalOverlay->SetRenderOpacity(CurrentCriticalOverlayOpacity);

    CriticalOverlayFadeOutTimer += DeltaTime;
}

void UStaminaWidget::FadeOutCriticalOverlay(float DeltaTime)
{
    float InterpOpacity = FMath::FInterpTo(CurrentCriticalOverlayOpacity, 0.f, DeltaTime, CriticalOverlayFadeOutSpeed);
    CurrentCriticalOverlayOpacity = InterpOpacity;
    CriticalOverlay->SetRenderOpacity(CurrentCriticalOverlayOpacity);

    if (CurrentCriticalOverlayOpacity < 0.1f)
    {
        //UE_LOG(LogTemp, Error, TEXT("CriticalOverlay Fade Out!!!"));

        bIsIndicatingCriticalHit = false;
        CurrentCriticalOverlayOpacity = 0.f;
        CriticalOverlay->SetRenderOpacity(0.f);
    }
}

void UStaminaWidget::FadeInNormalOverlay(float DeltaTime)
{
    float InterpOpacity = FMath::FInterpTo(CurrentNormalOverlayOpacity, TargetNormalOverlayOpacity, DeltaTime, NormalOverlayFadeInSpeed);
    CurrentNormalOverlayOpacity = InterpOpacity;
    NormalOverlay->SetRenderOpacity(CurrentNormalOverlayOpacity);

    NormalOverlayFadeOutTimer += DeltaTime;
}

void UStaminaWidget::FadeOutNormalOverlay(float DeltaTime)
{
    float InterpOpacity = FMath::FInterpTo(CurrentNormalOverlayOpacity, 0.f, DeltaTime, NormalOverlayFadeOutSpeed);
    CurrentNormalOverlayOpacity = InterpOpacity;
    NormalOverlay->SetRenderOpacity(CurrentNormalOverlayOpacity);

    if (CurrentNormalOverlayOpacity < 0.1f)
    {
        //UE_LOG(LogTemp, Error, TEXT("NormalOverlay Fade Out!!!"));

        bIsIndicatingNormalHit = false;
        CurrentNormalOverlayOpacity = 0.f;
        NormalOverlay->SetRenderOpacity(0.f);
    }
}

void UStaminaWidget::SetCriticalOvelayInvisible()
{
    bIsIndicatingCriticalHit = false;
    CurrentCriticalOverlayOpacity = 0.f;
    CriticalOverlay->SetRenderOpacity(0.f);
}

void UStaminaWidget::SetNormalOvelayInvisible()
{
    bIsIndicatingNormalHit = false;
    CurrentNormalOverlayOpacity = 0.f;
    NormalOverlay->SetRenderOpacity(0.f);
}

void UStaminaWidget::UpdateHitIndicator(float DeltaTime)
{
    if (bIsIndicatingCriticalHit)
    {
        if (CriticalOverlayFadeOutStartTime > CriticalOverlayFadeOutTimer)
        {
            FadeInCriticalOverlay(DeltaTime);
        }
        else
        {
            FadeOutCriticalOverlay(DeltaTime);
        }
    }

    if (bIsIndicatingNormalHit)
    {
        if (NormalOverlayFadeOutStartTime > NormalOverlayFadeOutTimer)
        {
            FadeInNormalOverlay(DeltaTime);
        }
        else
        {
            FadeOutNormalOverlay(DeltaTime);
        }
    }
}




#pragma endregion

#pragma region DashCounter
void UStaminaWidget::SetDashGauge()
{
    DashGauge->SetText(FText::AsNumber(fDashGauge));
}

void UStaminaWidget::UpdateDashUI(float InDashGauge)
{
    if (!LeftDashCounter || !RightDashCounter) return;

    if (InDashGauge <= 1.f)
    {
        RightDashCounter->SetPercent(InDashGauge);
        LeftDashCounter->SetPercent(0.f);
    }
    else
    {
        RightDashCounter->SetPercent(1.f);
        LeftDashCounter->SetPercent(InDashGauge - 1.f);
    }
}
#pragma endregion