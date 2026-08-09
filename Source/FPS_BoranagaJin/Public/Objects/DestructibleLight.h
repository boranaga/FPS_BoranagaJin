#pragma once

#include "CoreMinimal.h"
#include "Objects/DestructibleObject.h"
#include "DestructibleLight.generated.h"

class ULightComponent;
class UPointLightComponent;
class USpotLightComponent;
class URectLightComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EDestructibleLightType : uint8
{
	Point	UMETA(DisplayName = "Point Light"),
	Spot	UMETA(DisplayName = "Spot Light"),
	Rect	UMETA(DisplayName = "Rect Light")
};

UCLASS()
class FPS_BORANAGAJIN_API ADestructibleLight : public ADestructibleObject
{
	GENERATED_BODY()
public:
	ADestructibleLight();

	virtual float ReceiveDamage(const FDamageParams& DamageData) override;
protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BreakObject(const FVector& HitLocation, const FVector& HitDirection) override;
public:
	UFUNCTION(BlueprintCallable, Category = "Light")
	void TurnOnLight();
	UFUNCTION(BlueprintCallable, Category = "Light")
	void TurnOffLight();
	UFUNCTION(BlueprintCallable, Category = "Light")
	void ToggleLight();
	UFUNCTION(BlueprintCallable, Category = "Light")
	void SetLightEnabled(bool bEnabled);
	UFUNCTION(BlueprintCallable, Category = "Light")
	void SetLightIntensity(float NewIntensity);
	UFUNCTION(BlueprintCallable, Category = "Light")
	void SetLightColor(const FLinearColor& NewColor);
	UFUNCTION(BlueprintPure, Category = "Light")
	bool IsLightOn() const;

	UFUNCTION(BlueprintPure, Category = "Light")
	ULightComponent* GetActiveLightComponent() const;
protected:
	void UpdateLightSettings();
	void UpdateActiveLightType();
	void ApplyCommonLightSettings();
	void ApplyPointLightSettings();
	void ApplySpotLightSettings();
	void ApplyRectLightSettings();

	/*
 * 고장 상태 진입 여부를 검사합니다.
 */
	void EvaluateMalfunctionState();
	/*
	 * 고장으로 인한 깜빡임을 시작합니다.
	 */
	void StartMalfunctionFlicker();
	/*
	 * 깜빡임을 정지하고 타이머를 제거합니다.
	 */
	void StopMalfunctionFlicker();
	/*
	 * 현재 조명 상태를 반전하고 다음 랜덤 타이머를 설정합니다.
	 */
	void UpdateMalfunctionFlicker();
	/*
	 * 다음 깜빡임 실행 시간을 랜덤하게 설정합니다.
	 */
	void ScheduleNextFlicker();
protected:
	/*
	 * 조명의 위치와 회전을 별도로 조정하기 위한 루트입니다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Component")
	TObjectPtr<USceneComponent> LightRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Component")
	TObjectPtr<UPointLightComponent> PointLightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Component")
	TObjectPtr<USpotLightComponent> SpotLightComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Component")
	TObjectPtr<URectLightComponent> RectLightComponent;

	// 공통 설정

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Settings")
	EDestructibleLightType LightType = EDestructibleLightType::Point;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Light|Settings")
	bool bStartsEnabled = true;

	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadOnly, Category = "Light|Settings")
	bool bLightEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Settings",meta = (ClampMin = "0.0"))
	float LightIntensity = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Settings")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Settings", meta = (ClampMin = "0.0", Units = "cm"))
	float AttenuationRadius = 1000.f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Settings"
	)
	bool bCastShadows = true;

	// Point Light 설정

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Point",
		meta = (
			EditCondition =
			"LightType == EDestructibleLightType::Point",
			EditConditionHides,
			ClampMin = "0.0"
			)
	)
	float PointSourceRadius = 0.f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Point",
		meta = (
			EditCondition =
			"LightType == EDestructibleLightType::Point",
			EditConditionHides,
			ClampMin = "0.0"
			)
	)
	float PointSoftSourceRadius = 0.f;

	// Spot Light 설정

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Spot",
		meta = (
			EditCondition =
			"LightType == EDestructibleLightType::Spot",
			EditConditionHides,
			ClampMin = "0.0",
			ClampMax = "80.0",
			Units = "deg"
			)
	)
	float InnerConeAngle = 20.f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Spot",
		meta = (
			EditCondition =
			"LightType == EDestructibleLightType::Spot",
			EditConditionHides,
			ClampMin = "0.0",
			ClampMax = "80.0",
			Units = "deg"
			)
	)
	float OuterConeAngle = 45.f;

	// Rect Light 설정

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Rect",
		meta = (
			EditCondition =
			"LightType == EDestructibleLightType::Rect",
			EditConditionHides,
			ClampMin = "0.0",
			Units = "cm"
			)
	)
	float RectSourceWidth = 100.f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Rect",
		meta = (
			EditCondition =
			"LightType == EDestructibleLightType::Rect",
			EditConditionHides,
			ClampMin = "0.0",
			Units = "cm"
			)
	)
	float RectSourceHeight = 100.f;

protected:
	// 기존 조명 변수 생략

	/*
	 * 내구도가 이 비율 이하가 되면 고장 납니다.
	 *
	 * 0.3이면 최대 내구도의 30% 이하에서 고장 납니다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Malfunction",
		meta = (
			ClampMin = "0.0",
			ClampMax = "1.0"
			)
	)
	float MalfunctionHealthRatio = 0.3f;

	/*
	 * 내구도 저하에 따른 깜빡임 기능 사용 여부입니다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Malfunction"
	)
	bool bEnableMalfunctionFlicker = true;

	/*
	 * 두 번의 깜빡임 사이 최소 시간입니다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Malfunction",
		meta = (
			EditCondition = "bEnableMalfunctionFlicker",
			ClampMin = "0.01",
			Units = "s"
			)
	)
	float MinFlickerInterval = 0.05f;

	/*
	 * 두 번의 깜빡임 사이 최대 시간입니다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Malfunction",
		meta = (
			EditCondition = "bEnableMalfunctionFlicker",
			ClampMin = "0.01",
			Units = "s"
			)
	)
	float MaxFlickerInterval = 0.5f;

	/*
	 * 한 번 깜빡일 때 조명이 꺼질 확률입니다.
	 *
	 * 0.7이면 실행될 때마다 70% 확률로 꺼집니다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Malfunction",
		meta = (
			EditCondition = "bEnableMalfunctionFlicker",
			ClampMin = "0.0",
			ClampMax = "1.0"
			)
	)
	float FlickerOffProbability = 0.6f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Malfunction",
		meta = (
			EditCondition = "bEnableMalfunctionFlicker",
			ClampMin = "0.01",
			Units = "s"
			)
	)
	float MinFlickerOnDuration = 0.05f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Malfunction",
		meta = (
			EditCondition = "bEnableMalfunctionFlicker",
			ClampMin = "0.01",
			Units = "s"
			)
	)
	float MaxFlickerOnDuration = 0.4f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Malfunction",
		meta = (
			EditCondition = "bEnableMalfunctionFlicker",
			ClampMin = "0.01",
			Units = "s"
			)
	)
	float MinFlickerOffDuration = 0.02f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category = "Light|Malfunction",
		meta = (
			EditCondition = "bEnableMalfunctionFlicker",
			ClampMin = "0.01",
			Units = "s"
			)
	)
	float MaxFlickerOffDuration = 0.2f;


	UPROPERTY(
		VisibleInstanceOnly,
		SaveGame,
		BlueprintReadOnly,
		Category = "Light|Malfunction"
	)
	bool bIsMalfunctioning = false;

private:
	FTimerHandle MalfunctionFlickerTimerHandle;
};