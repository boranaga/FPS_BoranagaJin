

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TargetMarkerWidget.generated.h"

class UTextBlock;
class UImage;

UENUM(BlueprintType)
enum class ETargetLockOnState : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	TargetingReady = 1 UMETA(DisplayName = "TargetingReady"),
	TargetingSuccess = 2 UMETA(DisplayName = "TargetingSuccess")
};

UCLASS()
class FPS_BORANAGAJIN_API UTargetMarkerWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	UPROPERTY(meta = (BindWidget))
	UImage* MarkerImage = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BoolTest = nullptr;

	ETargetLockOnState CurrentLockOnState = ETargetLockOnState::None;

	float CurrentTimerDelay = 0.f;

	float TargetingReadyDuration = 0.f;
	float TargetingSuccessDuration = 0.f;
	UPROPERTY(Transient)
	float AnimationElapsedTime = 0.0f; // 애니메이션 경과 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float AnimationDuration = 0.f; // 애니메이션 총 지속 시간


	// 에디터에서 설정 가능하도록 UPROPERTY 매크로를 추가
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UISetting")
	FLinearColor DamageColor = FColor(255, 136, 0).ReinterpretAsLinear(); // #FF8800
public:
	bool bIsLockedOn = false; // 락온 완료 상태를 나타내는 플래그

	void StartLockOnProcess(float InReadyDuration, float InSuccessDruation); // 0.3초 타이머를 시작하는 함수
	void UpdateLockOnTimer(float RealTimeDeltaSeconds); // 매 프레임 타이머를 업데이트하는 함수

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
