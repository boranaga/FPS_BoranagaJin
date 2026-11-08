


#include "UI/WeaponAimUIWidget.h"

// #include "InterchangeResult.h"
#include "Items/Projectile/Projectile.h"
#include "Characters/Player/CharacterPlayer.h"
#include "Characters/Player/PlayerMovementComponent.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "Components//TextBlock.h"


void UWeaponAimUIWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    SetUIType(EUIType::WeaponAim);
}

void UWeaponAimUIWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Throwable fire-mode UI is disabled by default, so existing firearm aim widgets are unaffected.
    SetThrowableFireModeUIEnabled(bThrowableFireModeUIEnabled);

    //UE_LOG(LogTemp, Error, TEXT("UWeaponAimUIWidget::NativeConstruct()!!!"));
}

void UWeaponAimUIWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UpdateHitIndicator(InDeltaTime);
}

#pragma region ThrowableFireMode
void UWeaponAimUIWidget::SetThrowableFireModeUIEnabled(bool bEnable)
{
    bThrowableFireModeUIEnabled = bEnable;

    const ESlateVisibility NewVisibility =
        bThrowableFireModeUIEnabled
        ? ESlateVisibility::HitTestInvisible
        : ESlateVisibility::Hidden;

    if (ThrowableFireModeOverlay)
    {
        ThrowableFireModeOverlay->SetVisibility(NewVisibility);
    }

    if (ThrowableUpArc)
    {
        ThrowableUpArc->SetVisibility(NewVisibility);
    }

    if (ThrowableDownArc)
    {
        ThrowableDownArc->SetVisibility(NewVisibility);
    }

    if (bThrowableFireModeUIEnabled)
    {
        ApplyThrowableFireModeVisual();
    }
}

void UWeaponAimUIWidget::SetThrowableFireMode(EThrowableWeaponFireMode NewFireMode)
{
    ThrowableFireMode = NewFireMode;

    if (!bThrowableFireModeUIEnabled)
    {
        return;
    }

    ApplyThrowableFireModeVisual();
}

void UWeaponAimUIWidget::ApplyThrowableFireModeVisual()
{
    if (!bThrowableFireModeUIEnabled)
    {
        return;
    }

    auto ApplyArcState = [this](UImage* ArcImage, bool bActive)
        {
            if (!ArcImage)
            {
                return;
            }

            ArcImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
            ArcImage->SetRenderScale(bActive ? ThrowableActiveArcScale : ThrowableInactiveArcScale);
            //FVector2D NewScale_Active = FVector2D(ArcImage->GetRenderTransform().Scale.X * ThrowableActiveArcScale.X, ArcImage->GetRenderTransform().Scale.Y * ThrowableActiveArcScale.Y);
            //FVector2D NewScale_InActive = FVector2D(ArcImage->GetRenderTransform().Scale.X * ThrowableInactiveArcScale.X, ArcImage->GetRenderTransform().Scale.Y * ThrowableInactiveArcScale.Y);
            //FVector2D NewScale_Active = FVector2D(ArcImage->GetRenderTransform().Scale.X * ThrowableActiveArcScale.X, ArcImage->GetRenderTransform().Scale.Y * ThrowableActiveArcScale.Y);
            //FVector2D NewScale_InActive = FVector2D(ArcImage->GetRenderTransform().Scale.X * ThrowableInactiveArcScale.X, ArcImage->GetRenderTransform().Scale.Y * ThrowableInactiveArcScale.Y);
            //ArcImage->SetRenderScale(bActive ? NewScale_Active : NewScale_InActive);
            ArcImage->SetRenderOpacity(bActive ? ThrowableActiveArcOpacity : ThrowableInactiveArcOpacity);
        };

    const bool bUpwardActive = ThrowableFireMode == EThrowableWeaponFireMode::Upward;
    const bool bDownwardActive = ThrowableFireMode == EThrowableWeaponFireMode::Downward;

    ApplyArcState(ThrowableUpArc, bUpwardActive);
    ApplyArcState(ThrowableDownArc, bDownwardActive);
}
#pragma endregion

#pragma region Spread
void UWeaponAimUIWidget::ResetAimUISize()
{
    if (OutCircle)
    {
        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(OutCircle->Slot);
        if (CanvasSlot)
        {
            CanvasSlot->SetSize(DefaultOutCircleSize);
        }
    }
}
void UWeaponAimUIWidget::ApplyAimUISpread(float SpreadValue)
{
    if (OutCircle)
    {
        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(OutCircle->Slot);
        if (CanvasSlot)
        {
            FVector2D NewSize = { DefaultOutCircleSize.X + SpreadValue * OutCircleSpreadSizeOffset, DefaultOutCircleSize.Y + SpreadValue * OutCircleSpreadSizeOffset };
            CanvasSlot->SetSize(NewSize);
        }
    }
}
#pragma endregion

#pragma region HitIndicator
void UWeaponAimUIWidget::SetUpAimUIDelegateBinding(AProjectile* Projectile)
{
    Projectile->OnHeadShot.BindUObject(this, &UWeaponAimUIWidget::HeadShot);
    Projectile->OnBodyShot.BindUObject(this, &UWeaponAimUIWidget::BodyShot);
}

void UWeaponAimUIWidget::HeadShot()
{
    //UE_LOG(LogTemp, Error, TEXT("UWeaponAimUIWidget::HeadShot()!!!"));

    SetNormalOvelayInvisible();

    bIsIndicatingCriticalHit = true;
    CriticalOverlayFadeOutTimer = 0.f;
    CriticalOverlay->SetRenderOpacity(0.f);
    CriticalOverlay->SetVisibility(ESlateVisibility::Visible);
}

void UWeaponAimUIWidget::BodyShot()
{
    //UE_LOG(LogTemp, Error, TEXT("UWeaponAimUIWidget::BodyShot()!!!"));

    SetCriticalOvelayInvisible();

    bIsIndicatingNormalHit = true;
    NormalOverlayFadeOutTimer = 0.f;
    NormalOverlay->SetRenderOpacity(0.f);
    NormalOverlay->SetVisibility(ESlateVisibility::Visible);
}

void UWeaponAimUIWidget::FadeInCriticalOverlay(float DeltaTime)
{
    float InterpOpacity = FMath::FInterpTo(CurrentCriticalOverlayOpacity, TargetCriticalOverlayOpacity, DeltaTime, CriticalOverlayFadeInSpeed);
    CurrentCriticalOverlayOpacity = InterpOpacity;
    CriticalOverlay->SetRenderOpacity(CurrentCriticalOverlayOpacity);

    CriticalOverlayFadeOutTimer += DeltaTime;
}

void UWeaponAimUIWidget::FadeOutCriticalOverlay(float DeltaTime)
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

void UWeaponAimUIWidget::FadeInNormalOverlay(float DeltaTime)
{
    float InterpOpacity = FMath::FInterpTo(CurrentNormalOverlayOpacity, TargetNormalOverlayOpacity, DeltaTime, NormalOverlayFadeInSpeed);
    CurrentNormalOverlayOpacity = InterpOpacity;
    NormalOverlay->SetRenderOpacity(CurrentNormalOverlayOpacity);

    NormalOverlayFadeOutTimer += DeltaTime;
}

void UWeaponAimUIWidget::FadeOutNormalOverlay(float DeltaTime)
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

void UWeaponAimUIWidget::SetCriticalOvelayInvisible()
{
    bIsIndicatingCriticalHit = false;
    CurrentCriticalOverlayOpacity = 0.f;
    CriticalOverlay->SetRenderOpacity(0.f);
}

void UWeaponAimUIWidget::SetNormalOvelayInvisible()
{
    bIsIndicatingNormalHit = false;
    CurrentNormalOverlayOpacity = 0.f;
    NormalOverlay->SetRenderOpacity(0.f);
}

void UWeaponAimUIWidget::UpdateHitIndicator(float DeltaTime)
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