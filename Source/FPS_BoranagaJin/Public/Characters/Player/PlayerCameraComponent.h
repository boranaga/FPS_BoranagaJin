#pragma once

#include "CoreMinimal.h"
#include "PlayerCameraMovementData.h"
#include "Components/ActorComponent.h"
#include "PlayerCameraComponent.generated.h"

class ACharacterPlayer;
class APlayerController;
class APlayerCameraManager;
class UCameraComponent;
class UCameraShakeBase;
class UPlayerMovementComponent;

struct FActiveCameraZoomRequest
{
	FPlayerCameraZoomRequest Data;
	uint64 RequestOrder = 0;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UPlayerCameraComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPlayerCameraComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


#pragma region ExternalCameraAPI
public:
	// ----------------------------------------------------------------
	// Zoom
	// ----------------------------------------------------------------
	FGuid RequestZoom(const FPlayerCameraZoomRequest& Request);
	bool UpdateZoomRequest(const FGuid& Handle, const FPlayerCameraZoomRequest& Request);
	void ReleaseZoom(const FGuid& Handle);
	// ----------------------------------------------------------------
	// Camera Movement
	// ----------------------------------------------------------------
	FGuid RequestCameraMovement(const FPlayerCameraMovementRequest& Request);
	bool UpdateCameraMovementRequest(const FGuid& Handle, const FPlayerCameraMovementRequest& Request);
	void ReleaseCameraMovement(const FGuid& Handle);

	// ----------------------------------------------------------------
	// Camera Shake
	// ----------------------------------------------------------------
	UCameraShakeBase* RequestCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale = 1.f);
	void StopCameraShake(UCameraShakeBase* ShakeInstance, bool bImmediately = false);

	// 기존 코드와의 호환성 유지
	void PlayOneShotCameraShake(const TSubclassOf<UCameraShakeBase>& InShake);
	// ----------------------------------------------------------------
	// Reset
	// ----------------------------------------------------------------
	void ResetExternalCameraRequests(bool bImmediate = false);
#pragma endregion

public:
	void OnDamaged();
protected:
	// =========================================================================
	// References
	// =========================================================================
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<ACharacterPlayer> PlayerRef;
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UPlayerMovementComponent> MovementComponent;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCameraComponent> PlayerCamera;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<APlayerController> PlayerController;

	// =========================================================================
	// Existing Movement Camera
	// =========================================================================

	bool bIsMoveState = false;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDataTable> CameraDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Camera Transition")
	FMovementCameraData IdleCameraData;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Transition")
	FMovementCameraData WalkHorizontalCameraData;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Transition")
	FMovementCameraData WalkBackwardCameraData;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Transition")
	FMovementCameraData RunCameraData;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Transition")
	FMovementCameraData WallRunCameraData;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Transition")
	FMovementCameraData SlideCameraData;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Transition")
	FMovementCameraData FallCameraData;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Transition")
	FMovementCameraData CrouchCameraData;

	// =========================================================================
	// Loop Camera Shakes
	// =========================================================================

	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|Loop")
	TSubclassOf<UCameraShakeBase> IdleCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|Loop")
	TSubclassOf<UCameraShakeBase> WalkCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|Loop")
	TSubclassOf<UCameraShakeBase> RunCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|Loop")
	TSubclassOf<UCameraShakeBase> CrouchIdleCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|Loop")
	TSubclassOf<UCameraShakeBase> CrouchMoveCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|Loop")
	TSubclassOf<UCameraShakeBase> AirborneCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|Loop")
	TSubclassOf<UCameraShakeBase> WallRunCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|Loop")
	TSubclassOf<UCameraShakeBase> SlideCameraShake;

	// =========================================================================
	// One Shot Camera Shakes
	// =========================================================================

	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> PrimaryJumpCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> DoubleJumpCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> WallJumpCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> LightLandCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> MediumLandCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> StrongLandCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> WallLandCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> MantleCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> ForwardDashCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> ForwardRightDashCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> ForwardLeftDashCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> RightDashCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> LeftDashCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> BackwardDashCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> BackwardRightDashCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> BackwardLeftDashCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Camera Shake|One Shot")
	TSubclassOf<UCameraShakeBase> HitCameraShake;
	UPROPERTY(VisibleAnywhere, Category = "Camera Shake")
	TSubclassOf<UCameraShakeBase> CurrentLoopShake;

	// =========================================================================
	// Downed
	// =========================================================================

	UPROPERTY(EditDefaultsOnly, Category = "Downed")
	TSubclassOf<UCameraShakeBase> DownedImpactShake;
	UPROPERTY(EditDefaultsOnly, Category = "Downed")
	TSubclassOf<UCameraShakeBase> DownedFallShake;
	UPROPERTY(EditDefaultsOnly, Category = "Downed")
	TSubclassOf<UCameraShakeBase> DownedFloorImpactShake;
	UPROPERTY(EditDefaultsOnly, Category = "Downed")
	TSubclassOf<UCameraShakeBase> DownedFloorIdleShake;
	UPROPERTY(EditDefaultsOnly, Category = "Downed")
	TSubclassOf<UCameraShakeBase> DownedGoingUpShake;

	FTimerHandle DownFloorImpactTimerHandle;
	FTimerHandle DownGoingUpTimerHandle;

	FTimerDelegate DownedFloorImpactDelegate;
	FTimerDelegate DownedGoingUpShakeDelegate;


	// =========================================================================
	// Death
	// =========================================================================
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TSubclassOf<UCameraShakeBase> DeathStartCameraShake;
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TSubclassOf<UCameraShakeBase> DeathFloorHitCameraShake;

private:
	// =========================================================================
	// External Zoom Request
	// =========================================================================
	TMap<FGuid, FActiveCameraZoomRequest> ActiveZoomRequests;

	uint64 NextZoomRequestOrder = 0;

	FGuid CurrentResolvedZoomHandle;

	float CurrentZoomFOVOffset = 0.f;

	float LastZoomBlendOutSpeed = 10.f;

	// =========================================================================
	// External Camera Movement Request
	// =========================================================================
	struct FActiveCameraMovementRequest
	{
		FPlayerCameraMovementRequest Data;
		FVector CurrentLocationOffset = FVector::ZeroVector;
		FRotator CurrentRotationOffset = FRotator::ZeroRotator;
		bool bReleasing = false;
	};

	TMap<FGuid, FActiveCameraMovementRequest> ActiveCameraMovementRequests;

	FVector CurrentExternalLocationOffset = FVector::ZeroVector;

	FRotator CurrentExternalRotationOffset = FRotator::ZeroRotator;

private:
	APlayerCameraManager* GetPlayerCameraManager();
	const FActiveCameraZoomRequest* FindHighestPriorityZoomRequest(FGuid& OutHandle) const;

	void TickExternalCameraRequests(float DeltaTime);
	void TickZoomRequests(float DeltaTime);
	void TickCameraMovementRequests(float DeltaTime);
	void ApplyExternalCameraOffset();
protected:

	void ChangeCameraLoopShake(const TSubclassOf<UCameraShakeBase>& InShake);

	void TickMoveStateCamera(float DeltaTime);
	void TickWallRunStateCamera(float DeltaTime);
	void TickSlideStateCamera(float DeltaTime);
	void TickAirborneStateCamera(float DeltaTime);

	void InterpCameraData(const FMovementCameraData& InData, float DeltaTime);

	UFUNCTION()
	void OnAirborne();
	UFUNCTION()
	void OnMove();
	UFUNCTION()
	void OnWallRun();
	UFUNCTION()
	void OnSlide();
	UFUNCTION()
	void OnLand(float ZVelocity);
	UFUNCTION()
	void OnPrimaryJump();
	UFUNCTION()
	void OnDoubleJump();
	UFUNCTION()
	void OnWallJump();
	UFUNCTION()
	void OnMantle();
	UFUNCTION()
	void OnDash(FVector2D MovementInput);
	UFUNCTION()
	void OnDowned();
	void OnDeath();

	void InitCameraShakes();
};