

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StaminaWidget.generated.h"

class ACharacterPlayer;

class UImage;
class UOverlay;
class UProgressBar;
class UTextBlock;

UCLASS()
class FPS_BORANAGAJIN_API UStaminaWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);
public:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* StaminaBar = nullptr;
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* FadeInOutAnimation = nullptr;
protected:
	float MaxStamina = 100.f;
	float CurrStamina = 100.f;
public:
	void InitStaminaBar(float maxstamina = 100.f);
	void SetStaminaBarPercent(float const Value);
	void HideInGame(bool bHidden);
	void PlayFadeAnimation();
#pragma endregion
};