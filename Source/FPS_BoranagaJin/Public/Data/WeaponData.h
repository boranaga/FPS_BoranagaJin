

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/WeaponName.h"
#include "Items/Weapons/WeaponAction.h"
#include "Items/Weapons/WeaponRecoilParams.h"
#include "Items/Weapons/ArmRecoilParams.h"
#include "Items/Weapons/WeaponCamShakeBase.h"
#include "Items/Projectile/ProjectileSpreadConfig.h"
#include "NiagaraSystem.h"
#include "WeaponData.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Name")
	EWeaponName WeaponName = EWeaponName::WeaponName_Rifle;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primary Data Asset")
	FPrimaryAssetId WeaponPDA;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	FName WeaponSocket = FName();
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	bool bIsSkillWeapon = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	bool bAllowNormalFireForSkillWeapon = false;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EWeaponAction LeftMouseAction = EWeaponAction::WeaponAction_SingleShot;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EWeaponAction RightMouseAction = EWeaponAction::WeaponAction_SingleShot;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EWeaponAction SkillAction = EWeaponAction::WeaponAction_None;
	//-----------------------------------------------------------------
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	//TSoftObjectPtr<UNiagaraSystem> FireEffect_L = nullptr;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	//TSoftObjectPtr<UNiagaraSystem> FireEffect_R = nullptr;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	//TSoftObjectPtr<UNiagaraSystem> FireEffect_Skill = nullptr;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	//TSoftObjectPtr<UNiagaraSystem> ChargeEffect = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FVector ChargeEffectLocation = FVector();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FRotator ChargeEffectRotation = FRotator();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FVector ChargeEffenctScale = { 1.f, 1.f, 1.f };
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK")
	FTransform RightHandOffset = FTransform();
	//-----------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* WeaponImage_HUD = nullptr; // HUD에 표시할 총기 이미지

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* WeaponImage_Inventory = nullptr; // 인벤토리에 표시할 총기 이미지	

	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	float ReloadingTime = 2.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	float PumpReloadingTime_Start = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	float PumpReloadingTime_Loop = 0.4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	float PumpReloadingTime_End = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	float PumpReloadingTime_StartAndEnd = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	float PumpReloadingTime_LoopToFire = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	int32 MaxAmmoPerMag = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	int32 MaxTotalAmmo = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	int32 AmmoConsumedPerShot_Left = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	int32 AmmoConsumedPerShot_Right = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	int32 AmmoCost_Skill = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	bool bAllowFireWithInsufficientAmmo_L = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	bool bAllowFireWithInsufficientAmmo_R = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	bool bAllowFireWithInsufficientAmmo_Skill = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reload")
	bool bActivePumpActionReload = false;
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FullAutoShot")
	float FullAutoShotFireRate = 1.f;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectileSpread")
	FProjectileSpreadConfig DefaultSpread;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectileSpread")
	FProjectileSpreadConfig ZoomSpread;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiProjectile")
	bool bEnableMultiProjectile_Left = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiProjectile")
	bool bEnableMultiProjectile_Right = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiProjectile")
	float MaxAngleOfMultiProjectileSpread_L = 3.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiProjectile")
	float MaxAngleOfMultiProjectileSpread_R = 3.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiProjectile")
	int32 PelletsNum_L = 6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiProjectile")
	int32 PelletsNum_R = 6;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	FWeaponRecoilParams DefaultRecoil; //TODO: 제거
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	FWeaponRecoilParams DefaultRecoil_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	FWeaponRecoilParams DefaultRecoil_R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	FWeaponRecoilParams DefaultRecoil_Skill;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	FWeaponRecoilParams ZoomRecoil;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_Skill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_Hand;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_UpperArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_LowerArm;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform RightHandSocketTransform = FTransform();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform RightHandSocketTransform_Crouch = FTransform();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform RightHandSocketTransform_Targeting = FTransform(FRotator(22, -8, -89), FVector(-17, 10, 120), FVector(1, 1, 1));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform RightHandSocketTransform_Targeting_Crouch = FTransform(FRotator(80, -5, -97), FVector(-14, 8, 130), FVector(1, 1, 1));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform SkillWeaponSocketTransform_Active = FTransform(FRotator(0, 0, 0), FVector(26, 8, 160), FVector(1, 1, 1));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FTransform SkillWeaponSocketTransform_Inactive = FTransform(FRotator(-80, 0, 0), FVector(30, -8, 160), FVector(1, 1, 1));
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "CameraShake")
	TSubclassOf<UWeaponCamShakeBase> DefaultCameraShakeClass_L;
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "CameraShake")
	TSubclassOf<UWeaponCamShakeBase> DefaultCameraShakeClass_R;
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "CameraShake")
	TSubclassOf<UWeaponCamShakeBase> DefaultCameraShakeClass_Skill;
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "CameraShake")
	TSubclassOf<UWeaponCamShakeBase> ZoomCameraShakeClass;
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "CameraShake")
	TSubclassOf<UWeaponCamShakeBase> ChargingCameraShakeClass;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float MissileLaunchDelay = 0.2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	int32 MaxTargetNum = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float MaxTargetDetectionRadius = 3000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float MaxTargetDetectionAngle = 80.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float MaxTargetDetectionTime = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float TimeToReachMaxTargetDetectionRange = 2.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float TargetingGlobalTimeScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float TargetingGlobalTimeDilationSpeed_In = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float TargetingGlobalTimeDilationSpeed_Out = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float TargetingSkillCoolDown = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float MaxTargetingTime = 10.f;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	bool bAutoFireAtMaxChargeTime = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	float ChargeTimeThreshold = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	float MaxChargeTime = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	float ChargingAdditionalDamageBase = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	float ChargingAdditionalRecoilAmountPitchBase = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	float ChargingAdditionalRecoilAmountYawBase = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	float ChargingAdditionalProjectileRadiusBase = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charging")
	int32 ChargingAdditionalPelletMaxNum = 0;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penetration")
	int32 MaxPenetrableObjectsNum_Left = 4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penetration")
	int32 MaxPenetrableObjectsNum_Right = 4;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan")
	bool bIsHitScan_Left = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitScan")
	bool bIsHitScan_Right = false;
	//-----------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoAim")
	bool bIsAirborneAutoAimActive = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AutoAim")
	float AutoAimRadius = 100.f;
};

