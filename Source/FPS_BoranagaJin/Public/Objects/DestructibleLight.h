#pragma once

#include "CoreMinimal.h"
#include "Objects/DestructibleObject.h"
#include "DestructibleLight.generated.h"

class ULightComponent;
class UPointLightComponent;
class USpotLightComponent;
class URectLightComponent;
class USceneComponent;
class UMeshComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EDestructibleLightType : uint8
{
	Point	UMETA(DisplayName = "Point Light"),
	Spot	UMETA(DisplayName = "Spot Light"),
	Rect	UMETA(DisplayName = "Rect Light"),
	None UMETA(DisplayName = "Material Only")
};

USTRUCT(BlueprintType)
struct FLightEmissiveMaterial
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Material")
	int32 MaterialIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Material", meta = (ClampMin = "0.0"))
	float EmissiveIntensity = 20.f;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial = nullptr;
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

#pragma region LightControl
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
#pragma endregion

#pragma region ExternalPower
public:
	UFUNCTION(BlueprintCallable, Category = "Light|Power")
	void SetExternalPowerAvailable(bool bAvailable);
	UFUNCTION(BlueprintPure, Category = "Light|Power")
	bool HasExternalPower() const { return bHasExternalPower; }
#pragma endregion

#pragma region LightSettings
protected:
	void UpdateLightSettings();
	void UpdateActiveLightType();
	void ApplyCommonLightSettings();
	void ApplyPointLightSettings();
	void ApplySpotLightSettings();
	void ApplyRectLightSettings();
#pragma endregion

#pragma region Malfunction
protected:
	void EvaluateMalfunctionState();
	void StartMalfunctionFlicker();
	void StopMalfunctionFlicker();
	void UpdateMalfunctionFlicker();
	void ScheduleNextFlicker();
#pragma endregion

#pragma region Output
protected:
	/**
	 * 실제 조명 출력을 제어합니다.
	 *
	 * LightComponent + Emissive Material을 동시에 처리합니다.
	 */
	void SetLightOutputEnabled(bool bEnabled);
	void SetLightComponentOutputEnabled(bool bEnabled);
	void SetEmissiveOutputEnabled(bool bEnabled);
	bool CanProduceLight() const;
#pragma endregion


#pragma region Material
protected:
	void InitializeEmissiveMaterials();
	void UpdateEmissiveMaterials();
	UMeshComponent* GetEmissiveTargetMesh() const;
#pragma endregion

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Component")
	TObjectPtr<USceneComponent> LightRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Component")
	TObjectPtr<UPointLightComponent> PointLightComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Component")
	TObjectPtr<USpotLightComponent> SpotLightComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light|Component")
	TObjectPtr<URectLightComponent> RectLightComponent;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Settings")
	EDestructibleLightType LightType = EDestructibleLightType::Point;
	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Light|Settings")
	bool bStartsEnabled = true;
	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadOnly, Category = "Light|Settings")
	bool bLightEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Settings", meta = (ClampMin = "0.0"))
	float LightIntensity = 5000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Settings")
	FLinearColor LightColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Settings", meta = (ClampMin = "0.0", Units = "cm"))
	float AttenuationRadius = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Settings")
	bool bCastShadows = true;

protected:
	// <Point Light>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point", meta = (EditCondition = "LightType == EDestructibleLightType::Point", EditConditionHides, ClampMin = "0.0"))
	float PointSourceRadius = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Point", meta = (EditCondition = "LightType == EDestructibleLightType::Point", EditConditionHides, ClampMin = "0.0"))
	float PointSoftSourceRadius = 0.f;

protected:
	// <Spot Light>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Spot", meta = (EditCondition = "LightType == EDestructibleLightType::Spot", EditConditionHides, ClampMin = "0.0", ClampMax = "80.0"))
	float InnerConeAngle = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Spot", meta = (EditCondition = "LightType == EDestructibleLightType::Spot", EditConditionHides, ClampMin = "0.0", ClampMax = "80.0"))
	float OuterConeAngle = 45.f;

protected:
	// <Rect Light>
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Rect", meta = (EditCondition = "LightType == EDestructibleLightType::Rect", EditConditionHides, ClampMin = "0.0"))
	float RectSourceWidth = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Rect", meta = (EditCondition = "LightType == EDestructibleLightType::Rect", EditConditionHides, ClampMin = "0.0"))
	float RectSourceHeight = 100.f;

#pragma region MaterialSettings
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Material")
	bool bUseEmissiveMaterial = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Material")
	FName EmissiveIntensityParameterName = TEXT("LightIntensity");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Material")
	TArray<FLightEmissiveMaterial> EmissiveMaterials;
#pragma endregion


#pragma region Power
protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Light|Power")
	bool bHasExternalPower = true;
#pragma endregion


protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Malfunction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MalfunctionHealthRatio = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Malfunction")
	bool bEnableMalfunctionFlicker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Malfunction", meta = (ClampMin = "0.01", Units = "s"))
	float MinFlickerInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light|Malfunction", meta = (ClampMin = "0.01", Units = "s"))
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


	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Light|Malfunction")
	bool bIsMalfunctioning = false;

private:
	FTimerHandle MalfunctionFlickerTimerHandle;
	bool bFlickerOutputEnabled = true;
};