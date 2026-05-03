

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponAimUIWidget.generated.h"

class ACharacterPlayer;
class AProjectile;

class UImage;
class UOverlay;
class UProgressBar;
class UTextBlock;

UCLASS()
class FPS_BORANAGAJIN_API UWeaponAimUIWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);
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



#pragma region Spread
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

#pragma region DashCounter
protected:
	// WeaponAimUIWidget.h
	FTimerHandle LeftDashTimerHandle;
	FTimerHandle RightDashTimerHandle;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DashGauge = nullptr;

	float fDashGauge = 0.f;

	// Dash Counter
	UPROPERTY(meta = (BindWidget))
	UProgressBar* LeftDashCounter = nullptr;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* RightDashCounter = nullptr;

	UPROPERTY()
	ACharacterPlayer* CharacterPlayer = nullptr;

public:
	void SetDashGauge();
	void UpdateDashUI(float InDashGauge);


#pragma endregion 
};