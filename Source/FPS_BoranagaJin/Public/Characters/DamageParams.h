

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EDamageType.h"
#include "DamageParams.generated.h"

USTRUCT(Atomic, BlueprintType)
struct FDamageParams
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDamageType DamageType = EDamageType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanForceDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ImpulseDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ImpulseMagnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ImpactPoint = FVector::ZeroVector;
};

UCLASS()
class FPS_BORANAGAJIN_API ADamageParamsStruct : public AActor
{
	GENERATED_BODY()
public:
	ADamageParamsStruct();
};