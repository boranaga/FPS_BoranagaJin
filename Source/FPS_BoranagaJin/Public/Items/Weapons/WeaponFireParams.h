

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/Projectile/Projectile.h"
#include "ArmRecoilParams.h"
#include "WeaponRecoilParams.h"
#include "WeaponCamShakeBase.h"
#include "WeaponFireParams.generated.h"

USTRUCT(BlueprintType)
struct FWeaponFireParams
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;
	UPROPERTY(EditAnywhere)
	FArmRecoilParams Armrecoil;
	UPROPERTY(EditAnywhere)
	FWeaponRecoilParams Recoil;
	UPROPERTY(EditAnywhere)
	USoundBase* FireSound = nullptr;
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* MuzzleFireEffect = nullptr;
	UPROPERTY(EditAnywhere)
	TSubclassOf<UWeaponCamShakeBase> CamShake;
	UPROPERTY(EditAnywhere)
	int32 AmmoCost = 1;
	UPROPERTY(EditAnywhere)
	float MaxAngleOfMultiProjectileSpread = 3.f;
	UPROPERTY(EditAnywhere)
	int32 PelletsNum = 6;
	UPROPERTY(EditAnywhere)
	bool bAllowFireWithInsufficientAmmo = false;

	FWeaponFireParams() :
		ProjectileClass(nullptr),
		Armrecoil(),
		Recoil(),
		FireSound(nullptr),
		MuzzleFireEffect(nullptr),
		CamShake(),
		AmmoCost(1),
		MaxAngleOfMultiProjectileSpread(3.f),
		PelletsNum(6),
		bAllowFireWithInsufficientAmmo()
	{
	}

	FWeaponFireParams(
		TSubclassOf<class AProjectile> InProjectileClass,
		FArmRecoilParams InArmRecoilStruct,
		FWeaponRecoilParams InRecoil,
		USoundBase* InFireSound = nullptr,
		UNiagaraSystem* InMuzzleFireEffect = nullptr,
		TSubclassOf<UWeaponCamShakeBase> InCamShake = nullptr,
		int32 InAmmoCost = 0,
		float InMaxAngleOfMultiProjectileSpread = 0.f,
		int32 InPelletsNum = 0,
		bool InbAllowFireWithInsufficientAmmo = false
	) :
		ProjectileClass(InProjectileClass),
		Armrecoil(InArmRecoilStruct),
		Recoil(InRecoil),
		FireSound(InFireSound),
		MuzzleFireEffect(InMuzzleFireEffect),
		CamShake(InCamShake),
		AmmoCost(InAmmoCost),
		MaxAngleOfMultiProjectileSpread(InMaxAngleOfMultiProjectileSpread),
		PelletsNum(InPelletsNum),
		bAllowFireWithInsufficientAmmo(InbAllowFireWithInsufficientAmmo)
	{
	}
};

UCLASS()
class FPS_BORANAGAJIN_API AWeaponFireParamsStruct : public AActor
{
	GENERATED_BODY()
public:
	AWeaponFireParamsStruct();
};