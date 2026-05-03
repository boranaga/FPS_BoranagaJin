

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/Weapons/WeaponAction.h"
#include "BufferedFireRequest.generated.h"

/**
 *
 */
USTRUCT(Atomic, BlueprintType)
struct FBufferedFireRequest
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	EWeaponAction ActionName;
	UPROPERTY(EditAnywhere)
	bool bIsLeftInput = false;
	UPROPERTY(EditAnywhere)
	bool bSingleProjectile = false;
	UPROPERTY(EditAnywhere)
	int32 NumPenetrable = 0;


	FBufferedFireRequest(
		EWeaponAction InActionName = EWeaponAction::WeaponAction_SingleShot,
		bool InbIsLeftInput = true,
		bool InbSingleProjectile = true,
		int32 InNumPenetrable = 0
	) :
		ActionName(InActionName),
		bIsLeftInput(InbIsLeftInput),
		bSingleProjectile(InbSingleProjectile),
		NumPenetrable(InNumPenetrable)
	{
	}
};

UCLASS()
class FPS_BORANAGAJIN_API ABufferedFireRequestStruct : public AActor
{
	GENERATED_BODY()
public:
	ABufferedFireRequestStruct();
};
