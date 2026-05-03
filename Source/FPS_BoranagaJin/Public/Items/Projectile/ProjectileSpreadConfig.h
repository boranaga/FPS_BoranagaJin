

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileSpreadConfig.generated.h"

USTRUCT(Atomic, BlueprintType)
struct FProjectileSpreadConfig
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableProjectileSpread = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableAimUISpread = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxSpreadValue = 9.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpreadAmountBase = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpreadRangeMin = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpreadRangeMax = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpreadSpeed = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpreadRecoverSpeed = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpreadRecoveryStartTime = 0.2f;

	FProjectileSpreadConfig(
		bool InbEnableProjectileSpread = true,
		bool InbEnableAimUISpread = true,
		float InMaxSpreadValue = 9.f,
		float InSpreadAmountBase = 0.9f,
		float InSpreadRangeMin = 0.9f,
		float InSpreadRangeMax = 1.1f,
		float InSpreadSpeed = 18.f,
		float InSpreadRecoverSpeed = 20.f,
		float InSpreadRecoveryStartTime = 0.2f
	) :
		bEnableProjectileSpread(InbEnableProjectileSpread),
		bEnableAimUISpread(InbEnableAimUISpread),
		MaxSpreadValue(InMaxSpreadValue),
		SpreadAmountBase(InSpreadAmountBase),
		SpreadRangeMin(InSpreadRangeMin),
		SpreadRangeMax(InSpreadRangeMax),
		SpreadSpeed(InSpreadSpeed),
		SpreadRecoverSpeed(InSpreadRecoverSpeed),
		SpreadRecoveryStartTime(InSpreadRecoveryStartTime)
	{
	}
};

UCLASS()
class FPS_BORANAGAJIN_API AProjectileSpreadConfigStruct : public AActor
{
	GENERATED_BODY()
public:
	AProjectileSpreadConfigStruct();
};
