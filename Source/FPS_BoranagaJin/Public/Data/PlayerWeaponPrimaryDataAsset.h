

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "PlayerWeaponPrimaryDataAsset.generated.h"

/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API UPlayerWeaponPrimaryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSoftClassPtr<class AProjectile> LeftProjectileClass;
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSoftClassPtr<class AProjectile> RightProjectileClass;
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TSoftClassPtr<class AProjectile> SkillProjectileClass;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Effect")
	TSoftObjectPtr<UNiagaraSystem> FireEffect_L = nullptr;
	UPROPERTY(EditAnywhere, Category = "Effect")
	TSoftObjectPtr<UNiagaraSystem> FireEffect_R = nullptr;
	UPROPERTY(EditAnywhere, Category = "Effect")
	TSoftObjectPtr<UNiagaraSystem> FireEffect_Skill = nullptr;
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
	TSoftObjectPtr<USoundBase> FireSound_L = nullptr;
	UPROPERTY(EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundBase> FireSound_R = nullptr;
	UPROPERTY(EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundBase> FireSound_Skill = nullptr;
	UPROPERTY(EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundBase> ChargeSound = nullptr;
	UPROPERTY(EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundBase> TargetSearchLoopSound = nullptr;
	UPROPERTY(EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundBase> TargetLockedSound = nullptr;
	//-----------------------------------------------------------------

	FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("PlayerWeapon", GetFName()); }
};