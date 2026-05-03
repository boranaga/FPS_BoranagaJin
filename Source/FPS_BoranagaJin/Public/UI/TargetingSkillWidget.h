

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetingSkillWidget.generated.h"

class UImage;
class UACPlayerHUDManager; //TODO: 구현
class ACharacterPlayer;
class UHitScreenWidget;
class UTextBlock;

UCLASS()
class FPS_BORANAGAJIN_API UTargetingSkillWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* CurrentSkillCoolDown = nullptr;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* MaxSkillCoolDown = nullptr;

	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* ElapsedTimeAfterTargetingStarted = nullptr;
	UPROPERTY(EditAnyWhere, BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* MaxTargetingTime = nullptr;

	void InitializeUI(float NewCurrCoolDown, float NewMaxSkillCoolDown, float NewCurrTargetingTime, float NewMaxTargetingTime);
	void SetTargetingSkillCoolDown(float NewCurrCoolDown);
	void SetDetectionTimeUIVisible(bool bVisible);
	void SetTargetingTimeUI(float NewCurrTargetingTime);

#pragma region HpBar
private:
	// HP 단계별 이미지 배열
	UPROPERTY(meta = (BindWidget))
	UImage* CoolTimeBar = nullptr;

	float CachedMaxCoolDown;

	//APawnPlayer* SuraPawnPlayer;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UTexture2D*> HPBarTextures; // 총 6개 (0~5단계)

	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime);

	UFUNCTION()
	void ApplyCooltimeBarImage(float CurrentRatio);
#pragma endregion
};