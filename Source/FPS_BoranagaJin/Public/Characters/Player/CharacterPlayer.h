// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "GameFramework/Character.h" //TODO: Pawn vs Character which one?
#include "GameFramework/Pawn.h"
#include "Characters/DamageInterface.h"
#include "CharacterPlayer.generated.h"

class UCameraComponent;
class UCapsuleComponent;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class UNiagaraComponent;

class UPlayerMovementComponent;
class UPlayerCameraComponent;
class UWeaponSystemComponent;
class UPlayerSound_DataAsset;
class UCustomGameInstance;
struct FPlayerSoundData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerHealthHalved);

UCLASS()
class FPS_BORANAGAJIN_API ACharacterPlayer : public APawn, public IDamageInterface
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

	UWeaponSystemComponent* GetWeaponSystemComponent() const { return WeaponSystem; }

	USkeletalMeshComponent* GetArmMesh() { return ArmMesh; }
	USkeletalMeshComponent* GetHandsMesh() { return HandsMesh; }

	//UACUIMangerComponent* GetUIManager() const { return UIManager; }
	//UACHitScreenManager* GetHitScreenManager() const { return HitScreenManager; }

	bool HasWeapon() const;

	void UpdateLookInputVector2D(const FInputActionValue& InputValue);
	void SetLookInputVector2DZero();
	FVector2D GetPlayerLookInputVector() const { return PlayerLookInputVector2D; }
	UPlayerMovementComponent* GetPlayerMovementComponent() { return MovementComponent; };

	// for damage system comp and interactions with enemies
	//UACDamageSystem* GetDamageSystemComponent() const { return DamageSystemComponent; }
	//UACPlayerAttackTokens* GetAttackTokensComponent() const { return AttackTokensComponent; }
	virtual bool TakeDamage(const FDamageParams& DamageData, AActor* DamageCauser) override;

	FOnPlayerHealthHalved OnPlayerHealthHalved;

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

	UPROPERTY(EditAnywhere)
	TObjectPtr<UPlayerMovementComponent> MovementComponent;

	UPROPERTY(EditAnywhere, Category = "WeaponSystem")
	TObjectPtr<UWeaponSystemComponent> WeaponSystem;


	// This actor component is for handling camera shakes and state-based movement
	// IT IS NOT THE CAMERA!!
	UPROPERTY(EditAnywhere)
	TObjectPtr<UPlayerCameraComponent> CameraMovementComponent;

	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Damage")
	//UACDamageSystem* DamageSystemComponent;

	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack Tokens")
	//UACPlayerAttackTokens* AttackTokensComponent;

	// UI component - suhyeon
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BaseUI", meta = (AllowPrivateAccess = "true"))
	//UACUIMangerComponent* UIManager;

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
};
