#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "ThrowableWeaponPrimaryDataAsset.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API UThrowableWeaponPrimaryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSoftClassPtr<class AThrowableWeaponProjectile> ProjectileClass;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Effect")
	TSoftObjectPtr<UNiagaraSystem> FireEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "Effect")
	TSoftObjectPtr<UNiagaraSystem> ChargeEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "Effect")
	FVector ChargeEffectLocation = FVector();
	UPROPERTY(EditAnywhere, Category = "Effect")
	FRotator ChargeEffectRotation = FRotator();
	UPROPERTY(EditAnywhere, Category = "Effect")
	FVector ChargeEffenctScale = { 1.f, 1.f, 1.f };
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundBase> FireSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundBase> ChargeSound = nullptr;

	//-----------------------------------------------------------------

	FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("ThrowableWeapon", GetFName()); }
};