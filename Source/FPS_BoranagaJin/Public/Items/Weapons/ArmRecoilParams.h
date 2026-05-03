

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArmRecoilParams.generated.h"

USTRUCT(Atomic, BlueprintType)
struct FArmRecoilParams
{
	GENERATED_USTRUCT_BODY()
public:

	UPROPERTY(EditAnywhere)
	FRotator Recoil_Rot;

	UPROPERTY(EditAnywhere)
	FRotator RecoilRangeMin_Rot;

	UPROPERTY(EditAnywhere)
	FRotator RecoilRangeMax_Rot;

	UPROPERTY(EditAnywhere)
	FVector Recoil_Vec;

	UPROPERTY(EditAnywhere)
	FVector RecoilRangeMin_Vec;

	UPROPERTY(EditAnywhere)
	FVector RecoilRangeMax_Vec;

	//----------------

	//UPROPERTY(EditAnywhere)
	//FRotator RecoilAmount;

	//UPROPERTY(EditAnywhere)
	//FRotator RecoilRangeMin;

	//UPROPERTY(EditAnywhere)
	//FRotator RecoilRangeMax;

	UPROPERTY(EditAnywhere)
	float RecoilSpeed = 4.f;
	UPROPERTY(EditAnywhere)
	float RecoilRecoverSpeed = 3.5f;

	//FArmRecoilStruct(
	//	FRotator InRecoilAmount = {1.f, 1.f, 1.f},
	//	FRotator InRecoilRangeMin = { 1.f, 1.f, 1.f },
	//	FRotator InRecoilRangeMax = { 1.f, 1.f, 1.f },
	//	float InRecoilSpeed = 4.f,
	//	float InRecoilRecoverSpeed = 3.5f
	//	):
	//	RecoilAmount(InRecoilAmount),
	//	RecoilRangeMin(InRecoilRangeMin),
	//	RecoilRangeMax(InRecoilRangeMax),
	//	RecoilSpeed(InRecoilSpeed),
	//	RecoilRecoverSpeed(InRecoilRecoverSpeed)
	//{
	//}

	FArmRecoilParams(
		FRotator InRecoilAmount_Rot = FRotator(10.f, -20.f, 10.f),
		FRotator InRecoilRangeMin_Rot = FRotator(-1.f, -0.1f, -1.f),
		FRotator InRecoilRangeMax_Rot = FRotator(1.f, 1.1f, 1.f),

		FVector InRecoilAmount_Vec = FVector(17.f, 3.f, 3.f),
		FVector InRecoilRangeMin_Vec = FVector(1.f, 1.f, -1.f),
		FVector InRecoilRangeMax_Vec = FVector(1.1f, 1.1f, 1.1f),

		float InRecoilSpeed = 10.f,
		float InRecoilRecoverSpeed = 40.f
	) :
		Recoil_Rot(InRecoilAmount_Rot),
		RecoilRangeMin_Rot(InRecoilRangeMin_Rot),
		RecoilRangeMax_Rot(InRecoilRangeMax_Rot),

		Recoil_Vec(InRecoilAmount_Vec),
		RecoilRangeMin_Vec(InRecoilRangeMin_Vec),
		RecoilRangeMax_Vec(InRecoilRangeMax_Vec),

		RecoilSpeed(InRecoilSpeed),
		RecoilRecoverSpeed(InRecoilRecoverSpeed)
	{
	}
};

UCLASS()
class FPS_BORANAGAJIN_API AArmRecoilParamsStruct : public AActor
{
	GENERATED_BODY()
public:
	AArmRecoilParamsStruct();
};