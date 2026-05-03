// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/Item.h"
#include "WeaponName.h"
#include "WeaponAction.h"
#include "WeaponFireParams.h"
#include "WeaponCamConfig.h"
#include "WeaponInterface.h"
#include "BufferedFireRequest.h"
#include "Items/Projectile/ProjectileSpreadConfig.h"
//#include "ArmRecoilParams.h"
//#include "WeaponRecoilParams.h"
#include "Weapon.generated.h"

class ACharacterPlayer;

class UWeaponBaseState;
class UWeaponIdleState;
class UWeaponFiringState;
class UWeaponFullAutoFiringState;
class UWeaponUnequippedState;
class UWeaponReloadingState;
class UWeaponPumpActionReloadState;
class UWeaponSwitchingState;

class UWeaponChargingState;
class UWeaponWaitingState;
class AProjectileShell;
class UWeaponCamShakeBase;

class UNiagaraSystem;
class UNiagaraComponent;
class UWidgetComponent;
class UAmmoCounterWidget;
class UWeaponAimUIWidget;

class UInputAction;
struct FInputBindingHandle;
struct FStreamableHandle;

struct FBufferedFireRequest;

UCLASS()
class FPS_BORANAGAJIN_API AWeapon : public AItem, public IWeaponInterface
{
	GENERATED_BODY()
public:	
	AWeapon();
protected:
	virtual void BeginPlay() override;
public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = Weapon)
	TSoftObjectPtr<UDataTable> WeaponDataTable;
	UPROPERTY(EditAnywhere, Category = Weapon)
	FName WeaponRowName;

	TSharedPtr<FStreamableHandle> WeaponAssetsHandle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponMesh")
	USkeletalMeshComponent* WeaponMesh;
	UFUNCTION()
	USkeletalMeshComponent* GetWeaponMesh() { return WeaponMesh; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EWeaponAction LeftMouseAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EWeaponAction RightMouseAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EWeaponAction SkillAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UStaticMeshComponent* MuzzlePoint;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftSingleShotAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RightSingleShotAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftBurstShotAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RightBurstShotAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftFullAutoShotAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RightFullAutoShotAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftHoldAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RightHoldAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftChargeAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RightChargeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SkillToggleAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RightZoomAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;

	TArray<FInputBindingHandle*> InputActionBindingHandles;

	virtual void InitWeapon(ACharacterPlayer* NewCharacter);
	void InitializeCamera(ACharacterPlayer* NewCharacter);
	virtual void InitializeUI();
protected:
	UPROPERTY() UDataTable* LoadedWeaponTable = nullptr;
	UPROPERTY(Transient) bool bWeaponAssetsReady = false;
public:
	void LoadWeaponData();
	virtual void LoadWeaponData_Upgrade();
	void SetMeshVisibility(bool bflag);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool AttachWeaponToPlayer(ACharacterPlayer* TargetCharacter);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DetachWeaponFromPlayer();

	virtual void FireSingleProjectile(FWeaponFireParams* FireData = nullptr, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f, bool bIsHoming = false, AActor* HomingTarget = nullptr);
	void FireMultiProjectile(FWeaponFireParams* FireData = nullptr, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f, int32 AdditionalPellet = 0, bool bIsHoming = false, AActor* HomingTarget = nullptr);

#pragma region Socket
protected:
	FName WeaponSocketName = FName("");

#pragma endregion

#pragma region HitScan
protected:
	bool bIsHitScan_L = false;
	bool bIsHitScan_R = false;

	void FireSingleHitScan(FWeaponFireParams* FireData = nullptr, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f);
#pragma endregion

#pragma region AutoAim
protected:
	bool bIsAirborneAutoAimActive = true;
	float AutoAimRadius = 100.f;

	void FireSingleAutoAim(FWeaponFireParams* FireData = nullptr, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f);
#pragma endregion

public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ZoomToggle();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ZoomIn();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ZoomOut();
protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#pragma region WeaponState
public:
	UPROPERTY(VisibleAnywhere)
	UWeaponBaseState* CurrentState = nullptr;
	UPROPERTY(VisibleAnywhere)
	UWeaponIdleState* IdleState = nullptr;
	UPROPERTY(VisibleAnywhere)
	UWeaponFiringState* FiringState = nullptr;
	UPROPERTY(VisibleAnywhere)
	UWeaponFullAutoFiringState* FullAutoFiringState = nullptr;
	UPROPERTY(VisibleAnywhere)
	UWeaponUnequippedState* UnequippedState = nullptr;
	UPROPERTY(VisibleAnywhere)
	UWeaponReloadingState* ReloadingState = nullptr;
	UPROPERTY(VisibleAnywhere)
	UWeaponPumpActionReloadState* PumpActionReloadingState = nullptr;
	UPROPERTY(VisibleAnywhere)
	UWeaponSwitchingState* SwitchingState = nullptr;
	UPROPERTY(VisibleAnywhere)
	UWeaponChargingState* ChargingState = nullptr;
	UPROPERTY(VisibleAnywhere)
	UWeaponWaitingState* WaitingState = nullptr;
public:
	UFUNCTION()
	UWeaponBaseState* GetCurrentState() const { return CurrentState; }
	void ChangeState(UWeaponBaseState* NewState);

protected:
	/** The Character holding this weapon*/
	UPROPERTY()
	ACharacterPlayer* Character = nullptr;

	UPROPERTY()
	APlayerController* CharacterController = nullptr;

#pragma region FireParams
protected:
	UPROPERTY()
	FWeaponFireParams FireData_L;
	UPROPERTY()
	FWeaponFireParams FireData_R;
	UPROPERTY()
	FWeaponFireParams FireData_Skill;
#pragma endregion

#pragma region Animation
protected:
	FTransform RightHandSocketTransform;
	FTransform RightHandSocketTransform_Crouch;
	FTransform SkillWeaponSocketTransform_Active;
	FTransform SkillWeaponSocketTransform_Inactive;
protected:
	void StartFireAnimation(UAnimMontage* CharacterFireAnimation, UAnimMontage* WeaponFireAnimation);
	void StartAnimation(UAnimMontage* CharacterAnimation, UAnimMontage* WeaponAnimation, float CharacterAnimPlayRate, float WeaponAnimPlayRate, FName StartSection = FName());
	void CancelAnimation(UAnimMontage* CharacterAnimation, UAnimMontage* WeaponAnimation);
public:
	FTransform GetRightHandSocketTransform() const { return RightHandSocketTransform; }
	FTransform GetRightHandSocketTransform_Crouch() const { return RightHandSocketTransform_Crouch; }
	FTransform GetSkillWeaponSocketTransform_Active() const { return SkillWeaponSocketTransform_Active; }
	FTransform GetSkillWeaponSocketTransform_Inactive() const { return SkillWeaponSocketTransform_Inactive; }
#pragma endregion

#pragma region Animation/Character
protected:
	UPROPERTY()
	UAnimInstance* CharacterAnimInstance = nullptr;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Character")
	UAnimMontage* AM_Fire_Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Character")
	UAnimMontage* AM_Reload_Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Character")
	UAnimMontage* AM_Equip_Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Character")
	UAnimMontage* AM_Unequip_Character = nullptr;
#pragma endregion

#pragma region Animation/Weapon
protected: // TODO: 처리 고려
	UPROPERTY()
	UAnimInstance* WeaponAnimInstance = nullptr;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Weapon")
	UAnimMontage* AM_Fire_Weapon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Weapon")
	UAnimMontage* AM_Reload_Weapon = nullptr;
#pragma endregion

#pragma region Sound
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* ChargeSound = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	UAudioComponent* WeaponAudioComponent = nullptr;

	void PlayWeaponSound(USoundBase* weaponsound = nullptr);
	void StopWeaponSound();
#pragma endregion

#pragma region Niagara
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	UNiagaraSystem* ChargeEffect = nullptr;

	FVector ChargeEffectLocation;
	FRotator ChargeEffectRotation;
	FVector ChargeEffenctScale;

	UPROPERTY()
	UNiagaraComponent* ChargeEffectComponent = nullptr;
public:
	void SpawnMuzzleFireEffect(UNiagaraSystem* FireEffect = nullptr, FVector SpawnLocation = FVector(), FRotator SpawnRotation = FRotator());
	void SpawnChargeEffect(FVector SpawnLocation, FRotator SpawnRotation, FVector EffectScale);
	void DestroyChargeEffect();
#pragma endregion

#pragma region ProjectileShell
protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectileShell> ProjectileShellClass;
	UPROPERTY(EditAnywhere)
	int32 MaxProjectileShellNum = 5;

	UPROPERTY()
	TArray<AProjectileShell*> ProjectileShells; //TODO: queue로 대체 , 근데 그냥 array가 나을듯

	UPROPERTY(EditAnywhere)
	FVector DefaultEjectImpulseVec;
	UPROPERTY(EditAnywhere)
	float DefaultEjectImpulse = 0.f;

	int32 CurrProjectileShellIdx = 0;

	void InitProjectileShells();
	virtual void EjectProjectileShell() override;


#pragma endregion

#pragma region Aim
protected:
	FTransform AimSocketRelativeTransform;

	bool bIsZoomIn = false;

	FVector RightHandToAimSocketOffset;


	FTransform RightHandOffset;
protected:
	float LineTraceMaxDistance = 10000.f;

	UPROPERTY(EditAnywhere)
	float SphereTraceRadius = 20.f;

	FVector ScreenCenterWorldPosition;
	FVector ScreenCenterWorldDirection;

	FVector TargetLocationOfProjectile;

public:
	FVector CalculateScreenCenterWorldPositionAndDirection(FVector& OutWorldPosition, FVector& OutWorldDirection) const;

	void SetAimSocketTransform();
	void SetAimSocketRelativeTransform();
	FTransform GetAimSocketRelativeTransform();
#pragma endregion

#pragma region Equip/Unequip
protected:
	UPROPERTY(EditAnywhere)
	float WeaponSwitchingRate = 1.f;
	FTimerHandle SwitchingTimer;
public:
	void SwitchWeapon(ACharacterPlayer* TargetCharacter, bool bEquip);
	void EndWeaponSwitch(ACharacterPlayer* TargetCharacter, bool bEquip);
	void EquipWeapon(ACharacterPlayer* TargetCharacter, bool bActivateDirectly = false);
	void UnequipWeapon(ACharacterPlayer* TargetCharacter);
	virtual void SetInputActionBinding();
	void ResetInputActionBinding();
#pragma endregion

#pragma region Reload
protected:
	UPROPERTY(EditAnywhere)
	bool bCanAutoReload = true;
	bool bActivePumpActionReload = false;
	bool bFireInputDuringReload = false;
	TOptional<FBufferedFireRequest> BufferedFireRequest;

	UPROPERTY(EditAnywhere)
	float ReloadingTime = 2.5f;
	float PumpReloadingTime_Start = 0.6f;
	float PumpReloadingTime_Loop = 0.4;
	float PumpReloadingTime_End = 0.6;
	float PumpReloadingTime_StartAndEnd = 1.f;
	float PumpReloadingTime_LoopToFire = 0.2f;

	float CurrPumpActionReloadSectionTime = 1.f;
	float CurrPumpActionReloadTime = 1.f;

	UPROPERTY(EditAnywhere)
	int32 MaxTotalAmmo = 200.f;

	UPROPERTY(EditAnywhere)
	int32 TotalAmmo = 100.f;

	UPROPERTY(EditAnywhere)
	int32 MaxAmmoPerMag = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 LeftAmmoInCurrentMag = 0;

	FTimerHandle ReloadingTimer;
protected:
	void HandleReload();
	void HandlePumpActionReload();
	void CancelReload();
public:
	void StartReload();
	void StartPumpActionReload(bool bStartFromMiddle = false);
protected:
	void StopReload();
	void StopPumpActionReload();
	void InterruptReloadAndFire();

	void ConsumeAmmo(int32 AmmoCost = 1, bool AllowFireWithInsufficientAmmo = false);
	void ReloadAmmo(bool bPumpAction = false);
	bool HasAmmoInCurrentMag();
	bool HasAmmoInCurrentMag(int32 AmmoCost);
public:
	bool AddAmmo(int32 NumAmmo);
	void AutoReload();
	virtual void ReloadingEnd() override;

	int32 GetLeftAmmoInCurrentMag() { return LeftAmmoInCurrentMag; }
	int32 GetMaxAmmoPerMag() { return MaxAmmoPerMag; }
#pragma endregion

#pragma region UI
protected:
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "AimUIWidget")
	TSubclassOf<UWeaponAimUIWidget> AimUIWidgetClass;
	UPROPERTY()
	UWeaponAimUIWidget* AimUIWidget;
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "AmmoCounterWidget")
	TSubclassOf<UAmmoCounterWidget> AmmoCounterWidgetClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AmmoCounterWidget")
	UAmmoCounterWidget* AmmoCounterWidget;
public:
	void ActivateAimUIWidget(bool bflag);
	void ActivateAmmoCounterWidget(bool bflag);
	UFUNCTION(BlueprintCallable)
	UAmmoCounterWidget* GetAmmoCounterWidget() const { return AmmoCounterWidget; }
	UFUNCTION(BlueprintCallable)
	UWeaponAimUIWidget* GetAimUIWidget() const { return AimUIWidget; }
protected:
	void SetUpAimUIDelegateBinding(AProjectile* Projectile);
#pragma endregion

#pragma region WeaponType
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponName WeaponName = EWeaponName::WeaponName_Rifle;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//EWeaponType WeaponType;
public:
	EWeaponName GetWeaponName() const { return WeaponName; }
#pragma endregion

#pragma region FireMode
protected:
	void HandleSingleFire(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0);
	void HandleBurstFire(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0);
	void HandleFullAutoFire();
#pragma endregion

#pragma region FireMode/SingleShot
protected:
	FTimerHandle SingleShotTimer;

	UPROPERTY(EditAnywhere)
	float SingleShotDelay = 1.f;
public:
	void StartSingleShot(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f, int32 AdditionalPellet = 0);
	void StopSingleShot();
#pragma endregion

#pragma region FireMode/BurstShot
protected:
	FTimerHandle BurstShotTimer;

	UPROPERTY(EditAnywhere, Category = "BurstShot")
	float BurstShotDelay = 1.f;

	UPROPERTY(EditAnywhere)
	float BurstShotFireRate = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BurstShotCount = 3;

	int32 BurstShotFired = 0;

protected:
	void StartBurstFire(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f);
	void StopBurstFire();
	void EndBurstShot();
#pragma endregion

#pragma region FireMode/FullAuto
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|FireMode")
	float FullAutoShotFireRate = 0.1f;

	FTimerHandle FullAutoShotTimer;

protected:
	void StartFullAutoShot(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0);
	void UpdateFullAutoShot(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0);
	void StopFullAutoShot();
#pragma endregion

#pragma region FireMode/Charging
protected:
	bool bAutoFireAtMaxChargeTime = true;
	float ChargeTimeThreshold = 0.5f;
	float MaxChargeTime = 3.f;
	float ChargingAdditionalDamageBase = 100.f;

	float ChargingAdditionalRecoilAmountPitchBase = 4.f;
	float ChargingAdditionalRecoilAmountYawBase = 1.f;
	float ChargingAdditionalProjectileRadiusBase = 20.f;

	int32 ChargingAdditionalPelletMaxNum = 0;

	float ElapsedChargeTime = 0.f;
	FTimerHandle ChargingTimer;
protected:
	void StartCharge();
	void UpdateCharge();
	void StopCharge();
#pragma endregion

#pragma region Penetration
protected:
	int32 MaxPenetrableObjectsNum_Left = 4;
	int32 MaxPenetrableObjectsNum_Right = 4;
#pragma endregion

#pragma region Recoil/Aim
protected:
	bool bIsRecoiling = false;
	FWeaponRecoilParams* CurrentRecoil;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWeaponRecoilParams ZoomRecoil;

	FTimerHandle RecoilRecoverTimer;

	float TotalTargetRecoilValuePitch = 0.f;
	float TotalTargetRecoilValueYaw = 0.f;

	float CulmulatedRecoilValuePitch = 0.f;
	float CulmulatedRecoilValueYaw = 0.f;

	float RecoveredRecoilValuePitch = 0.f;
	float RecoveredRecoilValueYaw = 0.f;
public:
	void AddRecoilValue(FWeaponRecoilParams* RecoilStruct = nullptr, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f);
	void ApplyRecoil(float DeltaTime, FWeaponRecoilParams* RecoilStruct = nullptr);
	void RecoverRecoil(float DeltaTime, FWeaponRecoilParams* RecoilStruct = nullptr);
	void UpdateRecoil(float DeltaTime);
#pragma endregion

#pragma region Recoil/ArmAnimation
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_R;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_Hand;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_UpperArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_LowerArm;
public:
	void AddArmRecoil(FArmRecoilParams* armrecoil = nullptr);
	FArmRecoilParams* GetArmRecoilInfo();
	FArmRecoilParams* GetArmRecoilInfo_Hand();
	FArmRecoilParams* GetArmRecoilInfo_UpperArm();
	FArmRecoilParams* GetArmRecoilInfo_LowerArm();
#pragma endregion

#pragma region Recoil/SkilWeapon
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ArmRecoil")
	FArmRecoilParams ArmRecoil_SkillWeapon;
	void AddSkillWeaponRecoil(FArmRecoilParams* armrecoil = nullptr);
#pragma endregion

#pragma region Overheat
protected:
	UPROPERTY(EditAnywhere)
	bool bIsOverheatMode = false;
	UPROPERTY(EditAnywhere)
	float OverheatBaseIncrement = 0.5f; //TODO: 총기 능력 사용 시간만큼 더해지도록 해야함. 그리고 우클릭 좌클릭 다르게 들어가도록
	UPROPERTY(EditAnywhere)
	float MaxOverheatValue = 5.f;
	UPROPERTY(EditAnywhere)
	float OverheatSpeed = 10.f;
	UPROPERTY(EditAnywhere)
	float OverheatRecoverSpeed = 10.f; //TODO: Reload Time을 사용할 수도 있어서 조금 지켜봐야함

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OverheatRecoveryStartTime = 0.2f;

	bool bIsOverheating = false;

	float TotalTargetOverheatValue = 0.f;
	float CurrentOverheatVaule = 0.f;
	float OverheatRecoverTimer = 0.f;
protected:
	void AddOverheatValue();
	void ApplyOverheat(float DeltaTime);
	void RecoverOverheat(float DeltaTime);
	void UpdateOverheat(float DeltaTime);
#pragma endregion

#pragma region Projectile
protected:
	int32 NumProjectile_L = 10;
	int32 NumProjectile_R = 5;

	//----------------------------
	TMap<TSubclassOf<AProjectile>, TArray<AProjectile*>> ProjectilePool;
	UPROPERTY()
	TSet<AProjectile*> ActiveProjectileSet;

public:
	void InitProjectiles(TSubclassOf<AProjectile> ProjectileClass, int32 NumObject);
	AProjectile* GetProjectileFromPool(TSubclassOf<AProjectile> ProjectileClass);
	void ReturnProjectile(AProjectile* Projectile);
#pragma endregion

#pragma region Projectile/SingleProjectileSpread
protected:
	bool bIsSpreading = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectileSpread")
	FProjectileSpreadConfig DefaultSpread;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectileSpread")
	FProjectileSpreadConfig ZoomSpread;

	float TotalTargetSpreadValue = 0.f;
	float CurrentSpreadVaule = 0.f;
	float SpreadRecoverTimer = 0.f;
protected:
	void AddSpreadValue(FProjectileSpreadConfig* SpreadValue = nullptr);
	void ApplySpread(float DeltaTime, FProjectileSpreadConfig* SpreadValue = nullptr);
	void RecoverSpread(float DeltaTime, FProjectileSpreadConfig* SpreadValue = nullptr);
	void UpdateSpread(float DeltaTime);
	FVector GetRandomSpreadVector(FVector BaseDir);
#pragma endregion

#pragma region Projectile/MultiProjectileSpread
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiProjectile")
	bool bEnableMultiProjectile_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MultiProjectile")
	bool bEnableMultiProjectile_R;
#pragma endregion

#pragma region Camera
protected:
	bool bIsUsingPlayerCamFov = false;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWeaponCamConfig CamSetting_Default = FWeaponCamConfig(
		90.f,
		{ 0.f, 0.f, 0.f },
		{ 0.f, 0.f, 70.f },
		1.f, 1.f, 15.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWeaponCamConfig CamSetting_ZoomIn = FWeaponCamConfig(
		90.f,
		{ 0.f, 0.f, 0.f },
		{ 50.f, 0.f, 70.f },
		1.f, 1.f, 15.f);
	FTimerHandle CamSettingTimer;
public:
	void StartCameraSettingChange(FWeaponCamConfig* CamSetting);
	void UpdateCameraSetting(float DeltaTime, FWeaponCamConfig* CamSetting);
	void StopCameraSettingChange();
	void ForceStopCamModification();
	bool IsModifyingPlayerCamFov() const { return bIsUsingPlayerCamFov; }
#pragma endregion

#pragma region CameraShake
protected:
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "CameraShake")
	TSubclassOf<UWeaponCamShakeBase> ZoomCameraShakeClass;
	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "CameraShake")
	TSubclassOf<UWeaponCamShakeBase> ChargingCameraShakeClass;
public:
	void ApplyCameraShake(TSubclassOf<UWeaponCamShakeBase> CamShakeClass = nullptr, float Scale = 1.f);
#pragma endregion

#pragma region Skill
protected:
	bool bIsSkillWeapon = false;
public:
	bool IsSkillWeapon() const { return bIsSkillWeapon; }
#pragma endregion
};
