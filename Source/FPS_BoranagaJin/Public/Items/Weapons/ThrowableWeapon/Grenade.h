#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeapon.h"
#include "Grenade.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API AGrenade : public AThrowableWeapon
{
	GENERATED_BODY()

public:
	AGrenade();

protected:
	virtual void ActivateThrowable() override;

	void Explode();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float ExplosionDamage = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float ExplosionRadius = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float DamageFalloff = 1.f;
};