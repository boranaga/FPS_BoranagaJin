

#pragma once

#include "CoreMinimal.h"
#include "BaseUIWidget.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeaponFireMode.h"
#include "WeaponAimUIWidget.generated.h"

class ACharacterPlayer;
class AProjectile;

class UImage;
class UOverlay;
class UProgressBar;
class UTextBlock;

UCLASS()
class FPS_BORANAGAJIN_API UWeaponAimUIWidget : public UBaseUIWidget
{
	GENERATED_BODY()
public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual EUIType GetUIType() const { return EUIType::WeaponAim; }
public:
	UPROPERTY(meta = (BindWidget))
	UImage* Dot = nullptr;

	UPROPERTY(meta = (BindWidget))
	UImage* OutCircle = nullptr;

	UPROPERTY(meta = (BindWidget))
	UOverlay* NormalOverlay = nullptr;

	UPROPERTY(meta = (BindWidget))
	UOverlay* CriticalOverlay = nullptr;

	UPROPERTY(meta = (BindWidget))
	UImage* TopCrosshair = nullptr;

	UPROPERTY(meta = (BindWidget))
	UImage* BottomCrosshair = nullptr;

	UPROPERTY(meta = (BindWidget))
	UImage* LeftCrosshair = nullptr;

	UPROPERTY(meta = (BindWidget))
	UImage* RightCrosshair = nullptr;

#pragma region ThrowableFireMode
public:
	void SetThrowableFireModeUIEnabled(bool bEnable);
	void SetThrowableFireMode(EThrowableWeaponFireMode NewFireMode);

protected:
	void ApplyThrowableFireModeVisual();

	// Optional bindings: existing firearm Aim Widget Blueprints do not need these widgets.
	UPROPERTY(meta = (BindWidgetOptional))
	UOverlay* ThrowableFireModeOverlay = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* ThrowableUpArc = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* ThrowableDownArc = nullptr;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|ThrowableFireMode")
	//FVector2D DefaultThrowableActiveArcScale = FVector2D(1.15f, 1.15f);

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|ThrowableFireMode")
	//FVector2D DefaultThrowableInactiveArcScale = FVector2D(1.15f, 1.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|ThrowableFireMode")
	FVector2D ThrowableActiveArcScale = FVector2D(50.f, 17.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|ThrowableFireMode")
	FVector2D ThrowableInactiveArcScale = FVector2D(50.f * 0.80f, 17 * 0.80f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|ThrowableFireMode", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ThrowableActiveArcOpacity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|ThrowableFireMode", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ThrowableInactiveArcOpacity = 0.55f;

	bool bThrowableFireModeUIEnabled = false;
	EThrowableWeaponFireMode ThrowableFireMode = EThrowableWeaponFireMode::Upward;
#pragma endregion

#pragma region Spread
public:
	void ResetAimUISize();
	void ApplyAimUISpread(float SpreadValue = 0.f);
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D DefaultOutCircleSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OutCircleSpreadSizeOffset = 10.f;
#pragma endregion

#pragma region HitIndicator
protected:
	bool bIsIndicatingNormalHit = false;
	bool bIsIndicatingCriticalHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetNormalOverlayOpacity = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetCriticalOverlayOpacity = 1.f;

	float CurrentNormalOverlayOpacity = 0.f;
	float CurrentCriticalOverlayOpacity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NormalOverlayFadeInSpeed = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalOverlayFadeInSpeed = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NormalOverlayFadeOutSpeed = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalOverlayFadeOutSpeed = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NormalOverlayFadeOutStartTime = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalOverlayFadeOutStartTime = 0.2f;

	float NormalOverlayFadeOutTimer = 0.f;
	float CriticalOverlayFadeOutTimer = 0.f;

public:
	void SetUpAimUIDelegateBinding(AProjectile* Projectile);
protected:
	void HeadShot();
	void BodyShot();

	void FadeInCriticalOverlay(float DeltaTime);
	void FadeOutCriticalOverlay(float DeltaTime);

	void FadeInNormalOverlay(float DeltaTime);
	void FadeOutNormalOverlay(float DeltaTime);

	void SetCriticalOvelayInvisible();
	void SetNormalOvelayInvisible();

	void UpdateHitIndicator(float DeltaTime);

#pragma endregion
};