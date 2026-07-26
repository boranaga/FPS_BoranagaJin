// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "GameFramework/Character.h" //TODO: Pawn vs Character which one?
#include "GameFramework/Pawn.h"
#include "Interface/DamageInterface.h"
#include "Items/WeaponState/WeaponStateType.h"
#include "CharacterPlayer.generated.h"

class UCameraComponent;
class UCapsuleComponent;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class UNiagaraComponent;

class UPlayerMovementComponent;
class UHealthComponent;
class UBloodTrailComponent;
class UPlayerCameraComponent;
class UInventorySystemComponent;
class UInteractionComponent;
class UPlayerSound_DataAsset;
class UCustomGameInstance;
struct FPlayerSoundData;



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIWidgetCreated, UBaseUIWidget*, NewUIWidgetPtr);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaInit, float, MaxStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaUpdated, float, NewStaminaVal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractionUIPopUp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionUIUpdated, bool, bFlag, FVector, NewLocation);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryCreated, int32, InventoryCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryUpdated, const TArray<FInventorySlot>&, Inventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventorySwapRequested, FName, InventoryName, int32, FromIndex, int32, ToIndex);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventorySlotDropRequested, FName InventoryName, int32 SlotIndex);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnInventorySlotUseRequested, FName InventoryName, int32 SlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponInventoryCreated, int32, WeaponInventoryCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponInventoryUpdated, const TArray<FInventorySlot>&, WeaponInventory);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThrowableWeaponInventoryCreated, int32, ThrowableWeaponInventoryCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThrowableWeaponInventoryUpdated, const TArray<FInventorySlot>&, ThrowableWeaponInventory);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerHealthHalved);

UCLASS()
class FPS_BORANAGAJIN_API ACharacterPlayer : public APawn, public IDamageableInterface
{
	GENERATED_BODY()
public:
	ACharacterPlayer();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;

	UCapsuleComponent* GetCapsuleComponent();
	UCameraComponent* GetCameraComponent() const { return Camera; };
	FVector GetDefaultCameraRelativeLocation() const { return DefaultCameraRelativeLocation; };

	UInventorySystemComponent* GetInventorySystemComponent() const { return InventorySystem; }


	USkeletalMeshComponent* GetArmMesh() { return ArmMesh; }
	USkeletalMeshComponent* GetHandsMesh() { return HandsMesh; }

	//UACUIMangerComponent* GetUIManager() const { return UIManager; }
	//UACHitScreenManager* GetHitScreenManager() const { return HitScreenManager; }

	bool HasWeapon() const;
	EWeaponStateType GetWeaponStateType() const;

	void UpdateLookInputVector2D(const FInputActionValue& InputValue);
	void SetLookInputVector2DZero();
	FVector2D GetPlayerLookInputVector() const { return PlayerLookInputVector2D; }
	UPlayerMovementComponent* GetPlayerMovementComponent() { return MovementComponent; }
	UHealthComponent* GetHealthComponent() { return HealthComponent; }

	// for damage system comp and interactions with enemies
	//UACDamageSystem* GetDamageSystemComponent() const { return DamageSystemComponent; }
	//UACPlayerAttackTokens* GetAttackTokensComponent() const { return AttackTokensComponent; }

	virtual float ReceiveDamage(const FDamageParams& DamageInfo) override;
	virtual bool IsDead() const override;
	float GetCurrentHealth() const;
	float GetMaxHealth() const;

public:
	FOnUIWidgetCreated OnUIWidgetCreatedDelegate;
	FOnStaminaInit OnStaminaInit;
	FOnStaminaUpdated OnStaminaUpdated;
	FOnPlayerHealthHalved OnPlayerHealthHalved;
	FOnInteractionUIPopUp OnInteractionUIPopUpDelegate;
	FOnInteractionUIUpdated OnInteractionUIUpdatedDelegate;

	FOnInventoryCreated OnInventoryCreatedDelegate;
	FOnInventoryUpdated OnInventoryUpdatedDelegate;
	FOnInventorySwapRequested OnInventorySwapRequestedDelegate;
	FOnInventorySlotDropRequested OnInventorySlotDropRequestedDelegate;
	FOnInventorySlotUseRequested OnInventorySlotUseRequestedDelegate;

	FOnWeaponInventoryCreated OnWeaponInventoryCreatedDelegate;
	FOnWeaponInventoryUpdated OnWeaponInventoryUpdatedDelegate;

	FOnThrowableWeaponInventoryCreated OnThrowableWeaponInventoryCreatedDelegate;
	FOnThrowableWeaponInventoryUpdated OnThrowableWeaponInventoryUpdatedDelegate;

	//virtual void GravityLaunchPlayer(const FVector& Direction, float ForceAmount) override;

	//virtual void JumpPadLaunchPlayer(float ForceAmount) override;

	//virtual void RequestMovementDataModification(const TArray<FPlayerMovementDataModifier>& Modifiers) override;

	//virtual void RequestMovementKeyHoldModification(const TArray<FPlayerKeyHoldModifier>& Modifiers) override;

	//virtual void RequestResetModification() override;

	//UAudioComponent* GetWallRunAudioComponent() const { return WallRunAudioComponent; }
	UAudioComponent* GetSlideAudioComponent() const { return SlideAudioComponent; }

protected:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAudioComponent> WallRunAudioComponent;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAudioComponent> SlideAudioComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UPlayerSound_DataAsset> PlayerSound_DataAsset;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(EditAnywhere, Category = "Editor Assign")
	TObjectPtr<USkeletalMeshComponent> ArmMesh;

	UPROPERTY(EditAnywhere, Category = "Blueprint Assign")
	TObjectPtr<USkeletalMeshComponent> HandsMesh;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, Category = "MovementComponent")
	TObjectPtr<UPlayerMovementComponent> MovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HealthComponent")
	TObjectPtr<UHealthComponent> HealthComponent;
	UPROPERTY(EditAnywhere, Category = "InventorySystemComponent")
	TObjectPtr<UInventorySystemComponent> InventorySystem;
	UPROPERTY(VisibleAnywhere, Category = "BloodTrailComponent")
	TObjectPtr<UBloodTrailComponent> BloodTrailComponent;

	// This actor component is for handling camera shakes and state-based movement
	// IT IS NOT THE CAMERA!!
	UPROPERTY(EditAnywhere)
	TObjectPtr<UPlayerCameraComponent> CameraMovementComponent;

	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack Tokens")
	//UACPlayerAttackTokens* AttackTokensComponent;
	// 
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BaseUI", meta = (AllowPrivateAccess = "true"))
	//UACHitScreenManager* HitScreenManager;

	UPROPERTY(EditDefaultsOnly, Category = "Editor Assign")
	TObjectPtr<UNiagaraComponent> ForwardDashEffectComponent;
	UPROPERTY(EditDefaultsOnly, Category = "Editor Assign")
	TObjectPtr<UNiagaraComponent> BackwardDashEffectComponent;
	UPROPERTY(EditDefaultsOnly, Category = "Editor Assign")
	TObjectPtr<UNiagaraComponent> LeftDashEffectComponent;
	UPROPERTY(EditDefaultsOnly, Category = "Editor Assign")
	TObjectPtr<UNiagaraComponent> RightDashEffectComponent;

	UPROPERTY(EditAnywhere, Category = "Editor Assign")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Editor Assign")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Editor Assign")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category = "Editor Assign")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category = "Editor Assign")
	TObjectPtr<UInputAction> ShiftAction;

	UPROPERTY(EditAnywhere, Category = "Editor Assign")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Editor Assign")
	TObjectPtr<UInputAction> TeleportToLastCheckpointAction;

	FVector2D PlayerLookInputVector2D;

	FVector DefaultCameraRelativeLocation;

	UPROPERTY()
	TObjectPtr<UCustomGameInstance> CachedGameInstance;

	UFUNCTION()
	void OnJump();
	UFUNCTION()
	void OnWallJump();
	UFUNCTION()
	void OnSlide();
	UFUNCTION()
	void OnSlideEnd();
	UFUNCTION()
	void OnWallRun();
	UFUNCTION()
	void OnWallRunEnd();
	UFUNCTION()
	void OnLand(float ZSpeed);

	UFUNCTION()
	void HandleWallRunAudioPlayback(const USoundWave* PlayingSoundWave, const float PlaybackPercent);

	UFUNCTION()
	void HandleSlideAudioPlayback(const USoundWave* PlayingSoundWave, const float PlaybackPercent);


	void HandleMoveInput(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void StartJumpInput();
	void StopJumpInput();
	void StartShiftInput();
	void StopShiftInput();
	void StartCrouchInput();
	void StopCrouchInput();
	void StartTeleportToLastCheckpointInput();
	void CalculateMappedSoundValue(const FPlayerSoundData& Data, float Speed, float& OutVolumeMultiplier,
		float& OutPitchMultiplier);

	// Damage Comp Event Delegate Functions
	void OnDamaged();
	void OnDeath();

	UFUNCTION()
	void OnDash(FVector2D MovementInput);
	UFUNCTION()
	void OnDashEnd();



	float SlideEndTime = 0.f;

	//---------------
public:
	void InitUIManager();


#pragma region InteractionComponent
protected:
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Component"
	)
	TObjectPtr<UInteractionComponent> InteractionComponent;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Input"
	)
	TObjectPtr<UInputAction> InteractInputAction;

protected:
	void InputInteract(
		const FInputActionValue& InputActionValue
	);

public:
	UFUNCTION(BlueprintPure, Category = "Component")
	UInteractionComponent* GetInteractionComponent() const
	{
		return InteractionComponent;
	}
#pragma endregion
};
