#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Items/Item.h"
#include "Items/Projectile/ProjectileSpreadConfig.h"
#include "Items/Weapons/WeaponStateType.h"

#include "BufferedFireRequest.h"
#include "WeaponAction.h"
#include "WeaponCamConfig.h"
#include "WeaponFireParams.h"
#include "WeaponInterface.h"
#include "WeaponName.h"
#include "WeaponGripMode.h"

#include "Items/QuickSlotType.h"

//#include "ArmRecoilParams.h"
//#include "WeaponRecoilParams.h"

#include "Weapon.generated.h"

class ACharacterPlayer;
class AProjectileShell;

class UAmmoCounterWidget;
class UInputAction;
class UNiagaraComponent;
class UNiagaraSystem;
class UWeaponAimUIWidget;
class UWeaponBaseState;
class UWeaponCamShakeBase;
class UWeaponChargingState;
class UWeaponFiringState;
class UWeaponFullAutoFiringState;
class UWeaponIdleState;
class UWeaponPumpActionReloadState;
class UWeaponReloadingState;
class UWeaponSwitchingState;
class UWeaponUnequippedState;
class UWeaponWaitingState;
class UWidgetComponent;

struct FBufferedFireRequest;
struct FInputBindingHandle;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponGripModeChanged, EWeaponGripMode, NewGripMode);

UCLASS()
class FPS_BORANAGAJIN_API AWeapon : public AItem, public IWeaponInterface
{
	GENERATED_BODY()

#pragma region Core
public:
	AWeapon();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** Ends gameplay for this actor. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#pragma endregion

#pragma region Initialization
public:
	virtual void InitItem(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor = nullptr) override;
	virtual void InitItemPost(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor = nullptr) override;

	void InitializeCamera(ACharacterPlayer* NewCharacter);
	virtual void InitializeUI();

	void LoadWeaponData();
	virtual void LoadWeaponData_Upgrade();
	virtual void LoadWeaponDataPost();
	void SetMeshVisibility(bool bflag);
#pragma endregion

#pragma region Data
public:
	UPROPERTY(EditAnywhere, Category = "Weapon|Data")
	TSoftObjectPtr<UDataTable> WeaponDataTable;

	UPROPERTY(EditAnywhere, Category = "Weapon|Data")
	FName WeaponRowName;

	TSharedPtr<FStreamableHandle> WeaponAssetsHandle;

protected:
	UPROPERTY()
	UDataTable* LoadedWeaponTable = nullptr;

	UPROPERTY(Transient)
	bool bWeaponAssetsReady = false;
#pragma endregion

#pragma region Components
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	USkeletalMeshComponent* WeaponMesh;

	UFUNCTION()
	USkeletalMeshComponent* GetWeaponMesh() { return WeaponMesh; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Gameplay")
	UAnimMontage* FireAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Components")
	UStaticMeshComponent* MuzzlePoint;
#pragma endregion

#pragma region Actions
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Action")
	EWeaponAction LeftMouseAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Action")
	EWeaponAction RightMouseAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Action")
	EWeaponAction SkillAction;
#pragma endregion

#pragma region Input
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftSingleShotAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* RightSingleShotAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftBurstShotAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* RightBurstShotAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftFullAutoShotAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* RightFullAutoShotAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftHoldAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* RightHoldAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftChargeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* RightChargeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* SkillToggleAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* RightZoomAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;

	TArray<FInputBindingHandle*> InputActionBindingHandles;

protected:
	UPROPERTY()
	APlayerController* CharacterController = nullptr;
#pragma endregion

#pragma region Attachment
public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool AttachItemToPlayer(ACharacterPlayer* TargetCharacter) override;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DetachWeaponFromPlayer();
#pragma endregion

#pragma region Firing
public:
	virtual void FireSingleProjectile(FWeaponFireParams* FireData = nullptr, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f, bool bIsHoming = false, AActor* HomingTarget = nullptr);
	void FireMultiProjectile(FWeaponFireParams* FireData = nullptr, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f, int32 AdditionalPellet = 0, bool bIsHoming = false, AActor* HomingTarget = nullptr);
#pragma endregion

#pragma region Zoom
public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ZoomToggle();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ZoomIn();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ZoomOut();
#pragma endregion


#pragma region Save And Load
	//TODO: Save가 필요한 모든 변수에
	// UPROPERTY(VisibleAnywhere, SaveGame, Category = "Weapon|Save")
	// TODO: 기본적으로 DT를 통해 Load되는 변수들에 대해서는 어떻게 처리해야할지 고민 필요
	// 나중에 Restore 이후에 InitItem에서 DT Load 먼저 진행한 이후에 LoadItemSaveData 진행해야 하나?
public:
	virtual void OnItemStateRestored() override;
#pragma endregion

#pragma region State
public:
	void SetState(EWeaponStateType NewState);
	EWeaponStateType GetCurrentState() const { return CurrentState; }
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|State")
	EWeaponStateType CurrentState = EWeaponStateType::WeaponStateType_None;

protected:
	void EnterState(EWeaponStateType NewState);
	void ExitState(EWeaponStateType OldState);
	void UpdateState(float DeltaTime);

#pragma endregion

#pragma region Socket And Grip Mode
public:
	/** True: rifle/shotgun/launcher (two hands required). False: pistol/flare gun. */
	UFUNCTION(BlueprintPure, Category = "Weapon|Grip")
	bool IsTwoHandedWeapon() const { return bIsTwoHandedWeapon; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Grip")
	EWeaponGripMode GetGripMode() const { return CurrentGripMode; }

	/** Two-handed firearms can never enter OneHanded mode. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Grip")
	bool SetGripMode(EWeaponGripMode NewGripMode);

	/** Exposed for the first-person AnimBP/IK layer. */
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Grip")
	FOnWeaponGripModeChanged OnWeaponGripModeChanged;

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Grip", meta = (DisplayName = "On Grip Mode Changed"))
	void BP_OnGripModeChanged(EWeaponGripMode NewGripMode);

protected:
	void ConfigureWeaponHandedness();
	void RefreshGripAttachment();

	/** Loaded by the existing WeaponDataTable; finalized by weapon kind. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Grip")
	bool bIsTwoHandedWeapon = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Weapon|Grip")
	EWeaponGripMode CurrentGripMode = EWeaponGripMode::TwoHanded;

	/** Existing two-hand socket from DT; default is Gun. */
	FName WeaponSocketName = NAME_None;

	/** Must be a RIGHT-hand socket/bone on the first-person arms mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Grip")
	FName OneHandedWeaponSocketName = FName(TEXT("hand_r"));

	/** Multipliers are applied ONLY while a one-handed firearm is held in one hand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Grip|Accuracy", meta = (ClampMin = "1.0"))
	float OneHandedRecoilMultiplier = 1.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Grip|Accuracy", meta = (ClampMin = "1.0"))
	float OneHandedSpreadMultiplier = 1.65f;

	/** First shot also incurs inaccuracy, even when accumulated spread is zero. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Grip|Accuracy", meta = (ClampMin = "0.0"))
	float OneHandedBaseSpreadAngle = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Grip|Accuracy", meta = (ClampMin = "1.0"))
	float OneHandedCameraShakeMultiplier = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Grip|Accuracy", meta = (ClampMin = "1.0"))
	float OneHandedArmRecoilMultiplier = 1.4f;

	/** Persistent storage; never send a pointer to a temporary ArmRecoil struct. */
	UPROPERTY(Transient)
	FArmRecoilParams AdjustedArmRecoilForGrip;

	bool IsOneHandedGripActive() const;
	float GetGripSpreadAngle(float BaseAngle) const;
	void ApplyGripAdjustedFireCameraShake(TSubclassOf<UWeaponCamShakeBase> CamShakeClass);
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

#pragma region Animation - Character
protected:
	UPROPERTY()
	UAnimInstance* CharacterAnimInstance = nullptr;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation|Character")
	UAnimMontage* AM_Fire_Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation|Character")
	UAnimMontage* AM_Reload_Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation|Character")
	UAnimMontage* AM_Equip_Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation|Character")
	UAnimMontage* AM_Unequip_Character = nullptr;
#pragma endregion

#pragma region Animation - Weapon
protected: // TODO: 처리 고려
	UPROPERTY()
	UAnimInstance* WeaponAnimInstance = nullptr;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation|Weapon")
	UAnimMontage* AM_Fire_Weapon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation|Weapon")
	UAnimMontage* AM_Reload_Weapon = nullptr;
#pragma endregion

#pragma region Sound
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Audio")
	USoundBase* ChargeSound = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Audio")
	UAudioComponent* WeaponAudioComponent = nullptr;

	void PlayWeaponSound(USoundBase* weaponsound = nullptr);
	void StopWeaponSound();

	float ReloadingSoundLoudness = 0.5;
	float ReloadingSoundRange = 800;

	float FiringSoundLoudness = 2.0f;
	float FiringSoundRange = 3000.f;

	void ReportReloadingSoundToAI();
	void ReportFiringSoundToAI();
#pragma endregion

#pragma region Niagara
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Effects")
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

#pragma region Projectile Shell
	//TODO: ObjectPoolSubsystem에 편입
protected:
	UPROPERTY(EditAnywhere, Category = "Weapon|Projectile|Shell")
	TSubclassOf<AProjectileShell> ProjectileShellClass;
	UPROPERTY(EditAnywhere, Category = "Weapon|Projectile|Shell")
	int32 MaxProjectileShellNum = 5;

	UPROPERTY()
	TArray<AProjectileShell*> ProjectileShells; //TODO: queue로 대체 , 근데 그냥 array가 나을듯

	UPROPERTY(EditAnywhere, Category = "Weapon|Projectile|Shell")
	FVector DefaultEjectImpulseVec;
	UPROPERTY(EditAnywhere, Category = "Weapon|Projectile|Shell")
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

	UPROPERTY(EditAnywhere, Category = "Weapon|Aim")
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

#pragma region Equip / Unequip
protected:
	UPROPERTY(EditAnywhere, Category = "Weapon|Equip")
	float WeaponSwitchingRate = 1.f;
	FTimerHandle SwitchingTimer;
public:
	//void EndWeaponSwitch(ACharacterPlayer* TargetCharacter, bool bEquip);
	virtual void Unequip(ACharacterPlayer* TargetCharacter) override;
	virtual void Equip(ACharacterPlayer* TargetCharacter) override;

	//void EquipWeapon(ACharacterPlayer* TargetCharacter, bool bActivateDirectly = false);

	void UnequipWeapon_Legacy(ACharacterPlayer* TargetCharacter);

	void OnUnequipEnded(ACharacterPlayer* TargetCharacter);
	void OnEquipEnded(ACharacterPlayer* TargetCharacter);

	virtual void SetInputActionBinding();
	void ResetInputActionBinding();
#pragma endregion

#pragma region Reload
protected:
	UPROPERTY(EditAnywhere, Category = "Weapon|Reload")
	bool bCanAutoReload = true;
	bool bActivePumpActionReload = false;
	bool bFireInputDuringReload = false;
	TOptional<FBufferedFireRequest> BufferedFireRequest;

	UPROPERTY(EditAnywhere, Category = "Weapon|Reload")
	float ReloadingTime = 2.5f;
	float PumpReloadingTime_Start = 0.6f;
	float PumpReloadingTime_Loop = 0.4;
	float PumpReloadingTime_End = 0.6;
	float PumpReloadingTime_StartAndEnd = 1.f;
	float PumpReloadingTime_LoopToFire = 0.2f;

	float CurrPumpActionReloadSectionTime = 1.f;
	float CurrPumpActionReloadTime = 1.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Reload")
	int32 MaxTotalAmmo = 200;
	UPROPERTY(EditAnywhere, Category = "Weapon|Reload")
	int32 MaxAmmoPerMag = 20;
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "Weapon|Save")
	int32 TotalAmmo = 100;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Weapon|Save")
	int32 LeftAmmoInCurrMag = 0;

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

	int32 GetLeftAmmoInCurrentMag() { return LeftAmmoInCurrMag; }
	int32 GetMaxAmmoPerMag() { return MaxAmmoPerMag; }
#pragma endregion

#pragma region UI
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|UI|Aim")
	TSubclassOf<UWeaponAimUIWidget> AimUIWidgetClass;
	UPROPERTY()
	UWeaponAimUIWidget* AimUIWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|UI|Ammo")
	TSubclassOf<UAmmoCounterWidget> AmmoCounterWidgetClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|UI|Ammo")
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Type")
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

#pragma region Fire Mode - Single Shot
protected:
	FTimerHandle SingleShotTimer;

	UPROPERTY(EditAnywhere, Category = "Weapon|FireMode|Single")
	float SingleShotDelay = 1.f;
public:
	void StartSingleShot(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f, int32 AdditionalPellet = 0);
	void StopSingleShot();
#pragma endregion

#pragma region Fire Mode - Burst Shot
protected:
	FTimerHandle BurstShotTimer;

	UPROPERTY(EditAnywhere, Category = "Weapon|FireMode|Burst")
	float BurstShotDelay = 1.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|FireMode|Burst")
	float BurstShotFireRate = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|FireMode|Burst")
	int32 BurstShotCount = 3;

	int32 BurstShotFired = 0;

protected:
	void StartBurstFire(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0, float AdditionalDamage = 0.f, float AdditionalRecoilAmountPitch = 0.f, float AdditionalRecoilAmountYaw = 0.f, float AdditionalProjectileRadius = 0.f);
	void StopBurstFire();
	void EndBurstShot();
#pragma endregion

#pragma region Fire Mode - Full Auto
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|FireMode")
	float FullAutoShotFireRate = 0.1f;

	FTimerHandle FullAutoShotTimer;

protected:
	void StartFullAutoShot(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0);
	void UpdateFullAutoShot(bool bIsLeftInput = true, bool bSingleProjectile = true, int32 NumPenetrable = 0);
	void StopFullAutoShot();
#pragma endregion

#pragma region Fire Mode - Charging
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

#pragma region Recoil - Aim
protected:
	bool bIsRecoiling = false;
	FWeaponRecoilParams* CurrentRecoil;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil")
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

#pragma region Recoil - Arm Animation
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil|Arm")
	FArmRecoilParams ArmRecoil_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil|Arm")
	FArmRecoilParams ArmRecoil_R;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil|Arm")
	FArmRecoilParams ArmRecoil_Hand;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil|Arm")
	FArmRecoilParams ArmRecoil_UpperArm;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil|Arm")
	FArmRecoilParams ArmRecoil_LowerArm;
public:
	void AddArmRecoil(FArmRecoilParams* armrecoil = nullptr);
	FArmRecoilParams* GetArmRecoilInfo();
	FArmRecoilParams* GetArmRecoilInfo_Hand();
	FArmRecoilParams* GetArmRecoilInfo_UpperArm();
	FArmRecoilParams* GetArmRecoilInfo_LowerArm();
#pragma endregion

#pragma region Recoil - Skill Weapon
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Recoil|Arm")
	FArmRecoilParams ArmRecoil_SkillWeapon;
	void AddSkillWeaponRecoil(FArmRecoilParams* armrecoil = nullptr);
#pragma endregion

#pragma region Overheat
protected:
	UPROPERTY(EditAnywhere, Category = "Weapon|Overheat")
	bool bIsOverheatMode = false;
	UPROPERTY(EditAnywhere, Category = "Weapon|Overheat")
	float OverheatBaseIncrement = 0.5f; //TODO: 총기 능력 사용 시간만큼 더해지도록 해야함. 그리고 우클릭 좌클릭 다르게 들어가도록
	UPROPERTY(EditAnywhere, Category = "Weapon|Overheat")
	float MaxOverheatValue = 5.f;
	UPROPERTY(EditAnywhere, Category = "Weapon|Overheat")
	float OverheatSpeed = 10.f;
	UPROPERTY(EditAnywhere, Category = "Weapon|Overheat")
	float OverheatRecoverSpeed = 10.f; //TODO: Reload Time을 사용할 수도 있어서 조금 지켜봐야함

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Overheat")
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
	//TODO: ObjectPoolingSubsystem에 편입
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

#pragma region Projectile - Single Projectile Spread
protected:
	bool bIsSpreading = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Projectile|Spread")
	FProjectileSpreadConfig DefaultSpread;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Projectile|Spread")
	FProjectileSpreadConfig ZoomSpread;

	float TotalTargetSpreadValue = 0.f;
	float CurrentSpreadVaule = 0.f;
	float SpreadRecoverTimer = 0.f;
	void AddSpreadValue(FProjectileSpreadConfig* SpreadValue = nullptr);
	void ApplySpread(float DeltaTime, FProjectileSpreadConfig* SpreadValue = nullptr);
	void RecoverSpread(float DeltaTime, FProjectileSpreadConfig* SpreadValue = nullptr);
	void UpdateSpread(float DeltaTime);
	FVector GetRandomSpreadVector(FVector BaseDir);
#pragma endregion

#pragma region Projectile - Multi Projectile Spread
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Projectile|Multi")
	bool bEnableMultiProjectile_L;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Projectile|Multi")
	bool bEnableMultiProjectile_R;
#pragma endregion

#pragma region Camera
protected:
	bool bIsUsingPlayerCamFov = false;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Camera")
	FWeaponCamConfig CamSetting_Default = FWeaponCamConfig(
		90.f,
		{ 0.f, 0.f, 0.f },
		{ 0.f, 0.f, 70.f },
		1.f, 1.f, 15.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Camera")
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Camera|Shake")
	TSubclassOf<UWeaponCamShakeBase> ZoomCameraShakeClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Camera|Shake")
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

protected:
	//TODO: 수정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuickSlotType")
	EQuickSlotType QuickSlotType = EQuickSlotType::PrimaryWeapon;

public:
	EQuickSlotType GetQuickSlotType() const { return QuickSlotType; }
};
