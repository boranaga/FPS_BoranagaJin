

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeaponName.h"
#include "Items/Weapons/WeaponAction.h"
#include "Items/Weapons/WeaponCamShakeBase.h"

#include "NiagaraSystem.h"
#include "ThrowableWeaponData.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FThrowableWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Name")
	EThrowableWeaponName WeaponName = EThrowableWeaponName::ThrowableWeaponName_Base;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primary Data Asset")
	FPrimaryAssetId ThrowableWeaponPDA;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	FName WeaponSocket = FName();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	bool bIsTwoHandedWeapon = true;

	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EWeaponAction LeftMouseAction = EWeaponAction::WeaponAction_SingleShot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EWeaponAction RightMouseAction = EWeaponAction::WeaponAction_Hold;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FVector ChargeEffectLocation = FVector();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FRotator ChargeEffectRotation = FRotator();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FVector ChargeEffenctScale = { 1.f, 1.f, 1.f };
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* WeaponImage_HUD = nullptr; // HUD에 표시할 총기 이미지

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* WeaponImage_Inventory = nullptr; // 인벤토리에 표시할 총기 이미지	

	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	float ReloadingTime = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	int32 MaxAmmo = 99;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	int32 AmmoConsumedPerShot = 1;

	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SingleShot")
	float SingleShotDelay = 1.f;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BurstShot")
	float BurstShotDelay = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BurstShot")
	float BurstShotFireRate = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BurstShot")
	int32 BurstShotCount = 3;

	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform RightHandSocketTransform_Idle = FTransform();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform RightHandSocketTransform_Crouch = FTransform();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform RightHandSocketTransform_Aim_Idle = FTransform();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform RightHandSocketTransform_Aim_Crouch = FTransform();

	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "CameraShake")
	TSubclassOf<UWeaponCamShakeBase> FiringCameraShakeClass;

	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "CameraShake")
	TSubclassOf<UWeaponCamShakeBase> ChargingCameraShakeClass;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	bool bAutoFireAtMaxChargeTime = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	float ChargeTimeThreshold = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	float MaxChargeTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	float ChargingAdditionalDamageBase = 100.f;

	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penetration")
	int32 MaxPenetrableObjectsNum = 4;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan")
	bool bIsHitScan = false;
};