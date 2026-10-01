#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeaponName.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeaponStateType.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeaponFireParams.h"
#include "Items/Weapons/WeaponAction.h"
#include "Items/Weapons/WeaponInterface.h"
#include "ThrowableWeapon.generated.h"

class ACharacterPlayer;
class APlayerController;
class AItemPickUp;
class AThrowableWeaponProjectile;
class UAnimInstance;
class UAnimMontage;
class UInputAction;
class UInputMappingContext;
class UWeaponAimUIWidget;
class UAmmoCounterWidget;
class UWeaponCamShakeBase;
struct FInputBindingHandle;
struct FStreamableHandle;

UCLASS()
class FPS_BORANAGAJIN_API AThrowableWeapon : public AItem, public IWeaponInterface
{
	GENERATED_BODY()
public:
	AThrowableWeapon();
protected:
	virtual void BeginPlay() override;
public:
	// Initialization and data loading
	virtual void InitItem(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor = nullptr) override;
	virtual void InitItemPost(ACharacterPlayer* NewCharacter, AItemPickUp* PickUpActor = nullptr) override;
	void InitCam(ACharacterPlayer* TargetCharacter);
	void InitUI();
	void LoadData();
	void LoadDataPost();
	virtual void OnItemStateRestored() override;

	// Equipment
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool AttachItemToPlayer(ACharacterPlayer* TargetCharacter) override;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DetachItemFromPlayer();

	virtual void Equip(ACharacterPlayer* TargetCharacter) override;
	virtual void Unequip(ACharacterPlayer* TargetCharacter) override;
	void OnEquipEnded(ACharacterPlayer* TargetCharacter);
	void OnUnequipEnded(ACharacterPlayer* TargetCharacter);
	void DetachWeaponFromPlayer();

	// Throwable state
	void SetState(EThrowableWeaponStateType NewState);
	EThrowableWeaponStateType GetCurrentState() const { return CurrentState; }
	bool IsIdle() const { return CurrentState == EThrowableWeaponStateType::Idle; }
	bool IsAiming() const { return CurrentState == EThrowableWeaponStateType::Aiming; }
	bool IsThrown() const { return CurrentState == EThrowableWeaponStateType::Switching; }

	// Aim and throw
	void StartAim();
	void StopAim();
	void Throw(const FVector& ThrowDirection);

	// Input
	virtual void SetInputActionBinding();
	void ResetInputActionBinding();

	// Ammo and reload
	void StartReload();
	bool AddAmmo(int32 NumAmmo);
	void AutoReload();
	virtual void ReloadingEnd() override;
	int32 GetLeftAmmo() const { return LeftAmmo; }
	int32 GetMaxAmmo() const { return MaxAmmo; }

	// Projectile pool
	void InitProjectiles(TSubclassOf<AThrowableWeaponProjectile> ProjectileClass, int32 NumObject);
	//AThrowableWeaponProjectile* GetProjectileFromPool(TSubclassOf<AThrowableWeaponProjectile> ProjectileClass);
	void ReturnProjectile(AThrowableWeaponProjectile* Projectile);

	// UI and camera
	void ActivateAimUIWidget(bool bFlag);
	void ActivateAmmoCounterWidget(bool bFlag);
	void ApplyCameraShake(TSubclassOf<UWeaponCamShakeBase> CamShakeClass = nullptr, float Scale = 1.f);

	// Getters
	EThrowableWeaponName GetWeaponName() const { return ThrowableWeaponName; }
	FTransform GetRightHandSocketTransform() const { return RightHandSocketTransform_Default; }
	FTransform GetRightHandSocketTransform_Crouch() const { return RightHandSocketTransform_Crouch; }
	FTransform GetRightHandSocketTransform_Aim() const { return RightHandSocketTransform_Aim_Default; }
	FTransform GetRightHandSocketTransform_Aim_Crouch() const { return RightHandSocketTransform_Aim_Crouch; }
	FTransform GetMuzzlePointTransform() const;

protected:
	// State lifecycle
	void EnterState(EThrowableWeaponStateType NewState);
	void ExitState(EThrowableWeaponStateType OldState);
	void UpdateState(float DeltaTime);
	void UpdateIdle(float DeltaTime);
	void UpdateAiming(float DeltaTime);
	void UpdateThrown(float DeltaTime);
	void UpdateSwitching(float DeltaTime);
	virtual void OnEnterIdle();
	virtual void OnEnterAiming();
	virtual void OnEnterThrown();

	// Throwable lifecycle
	virtual void StartFuse();

	UFUNCTION()
	virtual void OnFuseExpired();

	virtual void ActivateThrowable();
	virtual void ResetThrowable();

	// Fire handling
	void FireSingleProjectile(FThrowableWeaponFireParams* FireData = nullptr, int32 NumPenetrable = 0, float AdditionalDamage = 0.f);
	void FireSingleHitScan(FThrowableWeaponFireParams* FireData = nullptr, int32 NumPenetrable = 0, float AdditionalDamage = 0.f);
	void HandleSingleFire(int32 NumPenetrable = 0);
	void HandleBurstFire(int32 NumPenetrable = 0);
	void StartSingleShot(int32 NumPenetrable = 0, float AdditionalDamage = 0.f);
	void StopSingleShot();
	void StartBurstFire(int32 NumPenetrable = 0, float AdditionalDamage = 0.f);
	void StopBurstFire();
	void EndBurstShot();

	// Charge handling
	void StartCharge();
	void UpdateCharge();
	void StopCharge();

	// Reload handling
	void HandleReload();
	void CancelReload();
	void StopReload();
	void InterruptReloadAndFire();
	void ConsumeAmmo(int32 AmmoCost = 1);
	void ReloadAmmo();
	bool HasAmmo();
	bool HasAmmo(int32 AmmoCost);

	// Animation
	void StartFireAnimation(UAnimMontage* CharacterFireAnimation);
	void StartAnimation(UAnimMontage* CharacterAnimation, float CharacterAnimPlayRate, FName StartSection = FName());
	void CancelAnimation(UAnimMontage* CharacterAnimation);

	// Effect
	void SpawnChargeEffect(FVector SpawnLocation, FRotator SpawnRotation, FVector EffectScale);
	void DestroyChargeEffect();

	// Sound
	void ReportFiringSoundToAI();
	void ReportReloadingSoundToAI();
protected:
	// Throwable settings
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable") TObjectPtr<UStaticMeshComponent> ItemMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Throw") float ThrowStrength = 1500.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Throw") float ThrowUpwardBias = 0.15f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Fuse") float FuseTime = 3.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Socket") FName HandSocketName = TEXT("ThrowableSocket");
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|State") EThrowableWeaponStateType CurrentState = EThrowableWeaponStateType::Idle;
	FTimerHandle FuseTimerHandle;

	// Weapon data
	UPROPERTY(EditAnywhere, Category = "Weapon") TSoftObjectPtr<UDataTable> WeaponDataTable;
	UPROPERTY(EditAnywhere, Category = "Weapon") FName WeaponRowName;
	UPROPERTY() UDataTable* LoadedWeaponTable = nullptr;
	TSharedPtr<FStreamableHandle> WeaponAssetsHandle;
	UPROPERTY(Transient) bool bWeaponAssetsReady = false;
	UPROPERTY() FThrowableWeaponFireParams FireData;
	bool bIsHitScan = false;

	// Weapon identity and actions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throwable") EThrowableWeaponName ThrowableWeaponName = EThrowableWeaponName::ThrowableWeaponName_Base;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action") EWeaponAction LeftMouseAction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action") EWeaponAction RightMouseAction;

	// Controller
	UPROPERTY()
	APlayerController* CharacterController = nullptr;

	// Input assets
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true")) UInputMappingContext* InputMappingContext = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true")) UInputAction* LeftSingleShotAction = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true")) UInputAction* RightSingleShotAction = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true")) UInputAction* LeftBurstShotAction = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true")) UInputAction* RightBurstShotAction = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true")) UInputAction* LeftHoldAction = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true")) UInputAction* RightHoldAction = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true")) UInputAction* LeftChargeAction = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true")) UInputAction* RightChargeAction = nullptr;
	TArray<FInputBindingHandle*> InputActionBindingHandles;

	// Socket transforms
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	bool bIsTwoHandedWeapon = false;
	FName WeaponSocketName = FName("");
	FTransform RightHandSocketTransform_Default;
	FTransform RightHandSocketTransform_Crouch;
	FTransform RightHandSocketTransform_Aim_Default;
	FTransform RightHandSocketTransform_Aim_Crouch;

	// ProjectileStartPoint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UStaticMeshComponent* MuzzlePoint;

	// Animation
	UPROPERTY() UAnimInstance* CharacterAnimInstance = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Character") UAnimMontage* AM_Fire_Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Character") UAnimMontage* AM_Reload_Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Character") UAnimMontage* AM_Equip_Character = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Character") UAnimMontage* AM_Unequip_Character = nullptr;
	UPROPERTY(EditAnywhere, Category = "Animation") float WeaponSwitchingRate = 1.f;
	FTimerHandle SwitchingTimer;

	// Ammo and reload
	UPROPERTY(EditAnywhere, Category = "Weapon|Ammo") bool bCanAutoReload = true;
	UPROPERTY(EditAnywhere, Category = "Weapon|Ammo") float ReloadingTime = 2.5f;
	UPROPERTY(EditAnywhere, Category = "Weapon|Ammo") int32 MaxAmmo = 200;
	UPROPERTY(VisibleAnywhere, SaveGame, Category = "Weapon|Save") int32 TotalAmmo = 100;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Weapon|Save") int32 LeftAmmo = 0;
	FTimerHandle ReloadingTimer;

	// Single shot
	UPROPERTY(EditAnywhere, Category = "SingleShot") float SingleShotDelay = 1.f;
	FTimerHandle SingleShotTimer;

	// Burst shot
	UPROPERTY(EditAnywhere, Category = "BurstShot") float BurstShotDelay = 1.f;
	UPROPERTY(EditAnywhere, Category = "BurstShot") float BurstShotFireRate = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BurstShot") int32 BurstShotCount = 3;
	int32 BurstShotFired = 0;
	FTimerHandle BurstShotTimer;

	// Charge shot
	bool bAutoFireAtMaxChargeTime = true;
	float ChargeTimeThreshold = 0.5f;
	float MaxChargeTime = 3.f;
	float ChargingAdditionalDamageBase = 100.f;
	//float ChargingAdditionalRecoilAmountPitchBase = 4.f;
	//float ChargingAdditionalRecoilAmountYawBase = 1.f;
	//float ChargingAdditionalProjectileRadiusBase = 20.f;
	//int32 ChargingAdditionalPelletMaxNum = 0;
	float ElapsedChargeTime = 0.f;
	FTimerHandle ChargingTimer;

	// Penetration
	int32 MaxPenetrableObjectsNum = 4;

	// UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Aim") TSubclassOf<UWeaponAimUIWidget> AimUIWidgetClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|AmmoCounter") TSubclassOf<UAmmoCounterWidget> AmmoCounterWidgetClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Aim") UWeaponAimUIWidget* AimUIWidget;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|AmmoCounter") UAmmoCounterWidget* AmmoCounterWidget;

	// Camera shake
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake") TSubclassOf<UWeaponCamShakeBase> ChargingCameraShakeClass;

	// Effect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	UNiagaraSystem* ChargeEffect = nullptr;
	UPROPERTY()
	UNiagaraComponent* ChargeEffectComponent = nullptr;
	FVector ChargeEffectLocation;
	FRotator ChargeEffectRotation;
	FVector ChargeEffenctScale;

	//Sound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* ChargeSound = nullptr;
};
