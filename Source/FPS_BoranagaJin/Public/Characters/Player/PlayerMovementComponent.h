

#pragma once

#include "CoreMinimal.h"
#include "Characters/EDamageType.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Characters/Player/MovementTriggerEnums.h"
#include "PlayerMovementComponent.generated.h"

class UCurveVector;
enum class EDamageTypeTest;
class AController;
//class APlayerController;
class ACharacterPlayer;

USTRUCT(BlueprintType)
struct FCachedInput
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FVector WorldInputDir = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere)
	FVector2D MovementInput2D = FVector2D::ZeroVector;
	UPROPERTY(VisibleAnywhere)
	bool bJumpPressed = false;
	UPROPERTY(VisibleAnywhere)
	bool bShiftPressed = false;
	UPROPERTY(VisibleAnywhere)
	bool bCrouchHeld = false;
};

UENUM(BlueprintType)
enum class EWallRunEnter : uint8
{
	EWRE_Upward,
	EWRE_Downward,
	EWRE_Neutral
};

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	EMS_Move,
	EMS_Slide,
	EMS_Airborne,
	EMS_WallRun,
	EMS_Mantle,
	EMS_Downed,
	EMS_Dead,
};

UENUM(BlueprintType)
enum class EWallRunSide : uint8
{
	EWRS_None,
	EWRS_Left,
	EWRS_Right,
};

USTRUCT()
struct FWallInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FHitResult Hit;

	UPROPERTY()
	double TimeStamp = 0;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMoveSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAirborneSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMantleSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDownedSignature);


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWallRunSignature); //
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWallRunEndSignature); //

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlideSignature); //
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlideEndSignature); //

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLandSignature, float, ZSpeed); //
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPrimaryJumpSignature); //
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWallJumpSignature); //

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDashSignature, FVector2D, InputVector); //
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDashEndSignature); //


UCLASS()
class FPS_BORANAGAJIN_API UPlayerMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()
public:
	UPlayerMovementComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetMovementInputVector(const FVector2D& InMovementInputVector);
	void SetJumpPressed(bool bPressed);
	void SetShiftPressed(bool bPressed);
	void SetCrouchPressed(bool bPressed);

	void SetDefaultCapsuleValues(float Radius, float HalfHeight)
	{
		DefaultCapsuleRadius = Radius;
		DefaultCapsuleHalfHeight = HalfHeight;
	}

	bool FindGroundPoint(FVector& OutPoint); //MEMO: AI에서 사용됨

	FVector2D GetMovementInputVector() const { return MovementInputVector; };

	UFUNCTION(BlueprintCallable)
	bool IsGrounded();

	UFUNCTION(BlueprintCallable)
	bool IsRunning() const { return bIsRunning; }
	virtual bool IsCrouching() const override { return bIsCrouching; }
	UFUNCTION(BlueprintCallable)
	bool IsDashing() const { return bIsDashing; }

	UFUNCTION(BlueprintCallable)
	float GetDashGauge() const { return DashGauge; } //TOOD: Stamina System으로 변경
	UFUNCTION(BlueprintCallable)
	float GetRunSpeed() const { return RunSpeed; }
	UFUNCTION(BlueprintCallable)
	float GetWalkSpeed() const { return WalkSpeed; }

	UFUNCTION(BlueprintCallable)
	EMovementState GetMovementState() const { return CurrentMovementState; }

	bool GetIsInvincible() const { return bIsInvincible; }

	void NotifyDamageData(EDamageType DamageType, const FVector& DamageDirection, float DamageForce);

	void NotifyGravityLaunchForce(const FVector& Direction, float ForceAmount);

	void NotifyJumpPadLaunchForce(float ForceAmount);

	//void NotifyMovementDataModification(const TArray<FPlayerMovementDataModifier>& Modifiers);

	//void NotifyMovementKeyHoldModification(const TArray<FPlayerKeyHoldModifier>& Modifiers);

	void NotifyResetModification();

	void NotifyDeath();

	FOnMoveSignature OnMoveDelegate;
	FOnWallRunSignature OnWallRunDelegate;
	FOnWallRunEndSignature OnWallRunEndDelegate;
	FOnAirborneSignature OnAirborneDelegate;
	FOnSlideSignature OnSlideDelegate;
	FOnSlideEndSignature OnSlideEndDelegate;

	FOnLandSignature OnLandDelegate;
	FOnPrimaryJumpSignature OnPrimaryJumpDelegate;
	FOnWallJumpSignature OnWallJumpDelegate;
	FOnMantleSignature OnMantleDelegate;
	FOnDashSignature OnDashDelegate;
	FOnDownedSignature OnDownedDelegate;
	FOnDashEndSignature OnDashEndDelegate;
protected:

	UPROPERTY(EditAnywhere)
	bool bPrintMovementDebug = false;

	UPROPERTY(EditAnywhere, Category = "Movement")
	TObjectPtr<UDataTable> MovementDataTable;

	UPROPERTY(EditAnywhere, Category = "Movement|Capsule")
	float DefaultCapsuleRadius;

	UPROPERTY(EditAnywhere, Category = "Movement|Capsule")
	float DefaultCapsuleHalfHeight;

	UPROPERTY(EditAnywhere, Category = "Movement|Capsule")
	float CrouchCapsuleHalfHeight = 40.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Gravity")
	float GravityScale = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Walk")
	float WalkSpeed = 600.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Dash")
	float DashStartSpeed = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Dash")
	float DashEndSpeed = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Dash")
	float DashDecelerationTime = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Movement|Dash")
	float DashCooldown = 5.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Run")
	float RunSpeed = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
	float CrouchSpeed = 400.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
	float CrouchHeightScale = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Movement|Jump")
	float PrimaryJumpHeight = 200.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Jump")
	float ExtraJumpHeight = 200.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Jump")
	float WallJumpHeight = 200.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Jump")
	float JumpHorizontalBoost = 20000.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Jump")
	float JumpBoostInputWindow = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float Acceleration = 8000.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float Deceleration = 8000.f;

	// Interp speed when over MaxHorizontalSpeed in air
	UPROPERTY(EditAnywhere, Category = "Movement|Air")
	float AirDirectionInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Air")
	float AirAcceleration = 5000.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Air")
	float AirDeceleration = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Air")
	float MaxFallVerticalSpeed = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Walk")
	float MaxWalkableFloorAngle = 50.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Walk")
	float MaxStepHeight = 30.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	float WallRunMaxDuration = 2.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	float WallRunAcceleration = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	float WallRunDeceleration = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	float WallRunMaxSpeed = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	float WallRunBackwardMaxSpeed = 100.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	float WallRunJumpAirSpeed2D = 1400.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	TObjectPtr<UCurveFloat> WallRunTiltRecoverCurve;

	UPROPERTY(EditAnywhere, Category = "Movement|Slide")
	float SlideInitialWindow = 0.65f;

	UPROPERTY(EditAnywhere, Category = "Movement|Slide")
	float SlideMaxDuration = 1.f;

protected:

	FVector LastVelocityBeforeLand = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	FCachedInput Input;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float GroundPointDetectionLength = 1000.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	bool bControllerTilting = false;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	bool bIsCrouching = false;

	FHitResult GroundHit;

#pragma region Move

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsRunning = false;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	bool bIsStepping = false;

	FHitResult StepWallHit;
	FHitResult StepFloorHit;

	FVector LastVelocityBeforeStep;

#pragma endregion Move

#pragma region WallRun

	EWallRunEnter WallRunEnterMode;
	bool bWallJumpAirBoost = false;
	bool bIsDeceleratingZ = false;
	UPROPERTY(VisibleAnywhere, Category = "Movement|WallRun")
	float WallRunElapsedTime = 0.f;
	float WallRunEnterSpeed2D;
	float WallJumpBuffer = 0.5f;
	EWallRunSide CurrentWallRunSide = EWallRunSide::EWRS_None;
	FHitResult CurrentWallHit;
	bool bShouldMoveUpWall = false;
	bool bShouldMoveDownWall = false;

	float WallRunCameraTiltInterpSpeed = 5.f;
	float WallRunCameraTiltAngle = 15.f;
	float PreWallRunDetectionRange = 200.f;
	float WallRunJumpNormalForce = 1800.f;

	bool bTiltRecovering = false;
	UPROPERTY(VisibleAnywhere, Category = "Movement|WallRun")
	float RecoverStartRoll;

	TArray<FWallInfo> CooldownWalls;
	float WallCooldown = 2.f;
#pragma endregion WallRun

#pragma region Dash

	UPROPERTY(VisibleAnywhere, Category = "Movement|Dash")
	bool bIsDashing = false;
	UPROPERTY(VisibleAnywhere, Category = "Movement|Dash")
	float ElapsedTimeFromDash = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Dash")
	float DashGauge = 2.f;

#pragma endregion Dash

#pragma region Jump

	float WallJumpZVelocity = 0.f;
	float PrimaryJumpZVelocity = 0.f;
	float JumpBuffer = 0.1f;
	UPROPERTY(VisibleAnywhere, Category = "Movement|Jump")
	float ElapsedTimeFromSurface = 0.f;
	int32 MaxJumpCount = 1;
	UPROPERTY(VisibleAnywhere, Category = "Movement|Jump")
	int32 CurrentJumpCount = 0;

	float CoyoteTime = 0.2f;
	float AirborneStartTime;
	UPROPERTY(VisibleAnywhere, Category = "Movement|Jump")
	bool bCoyoteTimeActivated = false;
	//-----------------------------
	bool bCanExtendJump = true;
	float MaxJumpHoldTime = 1.f;
	float CurrJumpHoldTime = 0.f;
	float ExtraJumpZVel = 1000.f;
	float JumpBoostWindowRemaining = 0.f;

#pragma endregion Jump

#pragma region Slide

	// Used for determining when to lerp slide direction.
	// Reset to 0 whenever exiting slide state
	UPROPERTY(VisibleAnywhere, Category = "Movement|Slide")
	float SlideStateElapsedTime = 0.f;

	// Used for max slide time cap
	UPROPERTY(VisibleAnywhere, Category = "Movement|Slide")
	float SlideElapsedTime = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Slide")
	float SlideResetTimer = 0.f;

	float SlideBoostResetDelay = 0.35f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Slide")
	float SlideDecelerationAmount = 1600.f;

	FVector SlideStartDirection;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Slide")
	bool bHasRecentlySlid = false;

	bool bShouldKeepSlideSpeed = false;

	float SlideAdditionalSpeed = 700.f;

	FVector LastSlideSpeedBeforeAirborne;


#pragma endregion Slide

#pragma region Mantle

	FHitResult MantleWallHit;

	FHitResult MantleFloorHit;


#pragma endregion Mantle

#pragma region Damage

	UPROPERTY(EditAnywhere, Category = "Movement|Damage")
	EDamageType ReceivedDamageType;

	bool bDamageSlowDebuff = false;

	float LastDamagedWorldTime = 0.f;

	float DamageSlowDebuffDuration = 1.f;

	float DamageSlowDebuffMultiplier = 0.3f;

	bool bIsInvincible = false;

#pragma endregion Damage

#pragma region Downed

	bool bDownedDamage = false;

	FRotator DownedStartControlRotation;

	float DownedDuration = 2.f;

	float DownedStartTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Editor Assign")
	TObjectPtr<UCurveVector> DownedPositionCurve;
	UPROPERTY(EditDefaultsOnly, Category = "Editor Assign")
	TObjectPtr<UCurveVector> DownedRotationCurve;

	float DownedInvincibleDuration = 1.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Downed")
	FVector ReceivedDamageDirection = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, Category = "Movement|Downed")
	float ReceivedDamageForce = 0;

#pragma endregion Downed

#pragma region GravityLaunch

	bool bGravityLaunchForceRequested = false;
	bool bAirborneFromGravityLauncher = false;
	FVector GravityLaunchForceDir = FVector::ZeroVector;
	float GravityLaunchForceAmount = 0.f;
	FVector JumpPadInitialVelocityXY = FVector::ZeroVector;

#pragma endregion GRavityLaunch

#pragma region JumpPad

	bool bJumpPadForceRequested = false;
	float JumpPadForceAmount = 0.f;

#pragma endregion JumpPad

#pragma region Dead

	bool bDeadRequested = false;

	void ConsumeDeadRequest();

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveVector> DeathCameraLocationCurve;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveVector> DeathCameraRotationCurve;

	const float DeathCameraAnimDuration = 0.7667f;

	FRotator DeathStartControlRotation;

	float DeathStartTime = 0.f;

#pragma endregion Dead

	UPROPERTY()
	TObjectPtr<ACharacterPlayer> CharacterPlayer = nullptr;

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController = nullptr;

	FVector GravityAcceleration;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	EMovementState PreviousMovementState;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	EMovementState CurrentMovementState;

	UPROPERTY(VisibleAnywhere, Category = "Movement")
	FVector2D MovementInputVector = FVector2D::ZeroVector;

	float MinWalkableFloorZ = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Jump")
	bool bJumpPressed = false;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Dash")
	bool bShiftPressed = false;

	UPROPERTY(VisibleAnywhere, Category = "Movement|Crouch")
	bool bCrouchPressed = false;

	bool bMovementDataModificationRequested = false;

	bool bMovementKeyHoldModificationRequested = false;

	bool bMovementKeyHoldActive = false;

	bool bMovementModificationResetRequested = false;

	//TArray<FPlayerMovementDataModifier> MovementDataModifiers;

	//TArray<FPlayerKeyHoldModifier> KeyHoldModifiers;

	TMap<EMovementDataType, float*> MovementDataTypeMap;

	UPROPERTY(VisibleAnywhere, Category = "Movement Modification")
	TMap<EMovementTriggerKey, bool> MovementTriggerKeyMap;

	void InitMovementDataTypeMap();

	void InitMovementTriggerKeyMap();

	void ApplyMovementDataModifiers();

	void ApplyKeyHoldModifiers();

	void SetIsDashing(bool bNewIsDashing);

	void OnIsDashingChanged(bool bNewIsDashing);

	void ApplyMovementDataTable();

	void AddControllerRoll(float DeltaTime, const FVector& WallRunDirection, EWallRunSide WallRunSide);

	void SetMovementState(EMovementState NewState);

	void OnMovementStateChanged(EMovementState OldState, EMovementState NewState);

	void TickState(float DeltaTime);

	void CrouchCapsule(float DeltaTime);

	void UnCrouchCapsule(float DeltaTime);

	void TickMove(float DeltaTime);

	void TickSlide(float DeltaTime);

	bool CanWallRun();

	bool CheckWallCooldown(const FWallInfo& InWallInfo);

	void UpdateWallCooldowns();

	void TickAirborne(float DeltaTime);

	void TickWallRun(float DeltaTime);

	void TickMantle(float DeltaTime);

	void TickDowned(float DeltaTime);

	void TickDead(float DeltaTime);

	void UpdateDashGauge(float DeltaTime);

	void CacheInput();

	void UpdateDamageFlags();

	void UpdateDependentMovementData();

#pragma region Stamina
protected:
	float MaxStamina = 1000.f;
	float CurrStamina = 1000.f;

	float DashStaminaConsumeRate = 100.f;
	float RunStaminaConsumeRate = 10.f;

	void UpdateStamina(float DeltaTime, float UpdateRate = 0.f);


#pragma endregion
};

