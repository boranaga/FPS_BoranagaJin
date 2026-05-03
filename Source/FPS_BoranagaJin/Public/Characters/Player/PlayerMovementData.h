

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PlayerMovementData.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FPlayerMovementRow : public FTableRowBase
{
	GENERATED_BODY()

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
	float DoubleJumpHeight = 300.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Jump")
	float WallJumpHeight = 200.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float Acceleration = 8000.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float Deceleration = 8000.f;

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
	float WallRunJumpNormalForce = 1800.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	float WallRunCameraTiltAngle = 15.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	float PreWallRunDetectionRange = 200.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	float WallRunCameraTiltInterpSpeed = 5.f;

	UPROPERTY(EditAnywhere, Category = "Movement|WallRun")
	TObjectPtr<UCurveFloat> WallRunTiltRecoverCurve;

	UPROPERTY(EditAnywhere, Category = "Movement|Slide")
	float SlideInitialWindow = 0.65f;

	UPROPERTY(EditAnywhere, Category = "Movement|Slide")
	float SlideMaxDuration = 1.25f;

	UPROPERTY(EditAnywhere, Category = "Movement|Slide")
	float SlideAdditionalSpeed = 700.f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = 0))
	float GroundPointDetectionLength = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = 0))
	float CoyoteTime = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = 0, ClampMax = 1))
	float DamageSlowDebuffMultiplier = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = 0))
	float DamageSlowDebuffDuration = 1.f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = 0))
	float DownedDuration = 2.f;

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = 0))
	float DownedInvincibleDuration = 1.f;
};