#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeaponProjectile.h"
#include "Items/Weapons/WeaponCamShakeBase.h"
#include "ThrowableWeaponFireParams.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FThrowableWeaponFireParams
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AThrowableWeaponProjectile> ProjectileClass;
	UPROPERTY(EditAnywhere)
	USoundBase* FireSound = nullptr;
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* MuzzleFireEffect = nullptr;
	UPROPERTY(EditAnywhere)
	TSubclassOf<UWeaponCamShakeBase> CamShake;
	UPROPERTY(EditAnywhere)
	int32 AmmoCost = 1;
};
