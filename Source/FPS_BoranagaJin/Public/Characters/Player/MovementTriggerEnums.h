

#pragma once

#include "CoreMinimal.h"


UENUM(BlueprintType)
enum class EMovementTriggerType : uint8
{
	Start UMETA(DisplayName = "Start"),
	End UMETA(DisplayName = "End"),
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMovementTriggerKey : uint8
{
	Ctrl UMETA(DisplayName = "Ctrl"),
	Shift UMETA(DisplayName = "Shift"),
	Space UMETA(DisplayName = "Space"),
	W UMETA(DisplayName = "W"),
	A UMETA(DisplayName = "A"),
	S UMETA(DisplayName = "S"),
	D UMETA(DisplayName = "D"),
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMovementDataType : uint8
{
	GravityScale UMETA(DisplayName = "GravityScale"),
	WalkSpeed UMETA(DisplayName = "WalkSpeed"),
	DashStartSpeed UMETA(DisplayName = "DashStartSpeed"),
	DashEndSpeed UMETA(DisplayName = "DashEndSpeed"),
	DashDecelerationTime UMETA(DisplayName = "DashDecelerationTime"),
	DashCooldown UMETA(DisplayName = "DashCooldown"),
	RunSpeed UMETA(DisplayName = "RunSpeed"),
	CrouchSpeed UMETA(DisplayName = "CrouchSpeed"),
	CrouchHeightScale UMETA(DisplayName = "CrouchHeightScale"),
	PrimaryJumpHeight UMETA(DisplayName = "PrimaryJumpHeight"),
	DoubleJumpHeight UMETA(DisplayName = "DoubleJumpHeight"),
	WallJumpHeight UMETA(DisplayName = "WallJumpHeight"),
	Acceleration UMETA(DisplayName = "Acceleration"),
	Deceleration UMETA(DisplayName = "Deceleration"),
	AirDirectionInterpSpeed UMETA(DisplayName = "AirDirectionInterpSpeed"),
	AirAcceleration UMETA(DisplayName = "AirAcceleration"),
	AirDeceleration UMETA(DisplayName = "AirDeceleration"),
	MaxFallVerticalSpeed UMETA(DisplayName = "MaxFallVerticalSpeed"),
	MaxWalkableFloorAngle UMETA(DisplayName = "MaxWalkableFloorAngle"),
	MaxStepHeight UMETA(DisplayName = "MaxStepHeight"),
	WallRunMaxDuration UMETA(DisplayName = "WallRunMaxDuration"),
	WallRunAcceleration UMETA(DisplayName = "WallRunAcceleration"),
	WallRunDeceleration UMETA(DisplayName = "WallRunDeceleration"),
	WallRunMaxSpeed UMETA(DisplayName = "WallRunMaxSpeed"),
	WallRunBackwardMaxSpeed UMETA(DisplayName = "WallRunBackwardMaxSpeed"),
	WallRunJumpAirSpeed2D UMETA(DisplayName = "WallRunJumpAirSpeed2D"),
	WallRunJumpNormalForce UMETA(DisplayName = "WallRunJumpNormalForce"),
	WallRunCameraTiltAngle UMETA(DisplayName = "WallRunCameraTiltAngle"),
	PreWallRunDetectionRange UMETA(DisplayName = "PreWallRunDetectionRange"),
	WallRunCameraTiltInterpSpeed UMETA(DisplayName = "WallRunCameraTiltInterpSpeed"),
	SlideInitialWindow UMETA(DisplayName = "SlideInitialWindow"),
	SlideMaxDuration UMETA(DisplayName = "SlideMaxDuration"),
	SlideAdditionalSpeed UMETA(DisplayName = "SlideAdditionalSpeed"),
	GroundPointDetectionLength UMETA(DisplayName = "GroundPointDetectionLength"),
	CoyoteTime UMETA(DisplayName = "CoyoteTime"),
	DamageSlowDebuffMultiplier UMETA(DisplayName = "DamageSlowDebuffMultiplier"),
	DamageSlowDebuffDuration UMETA(DisplayName = "DamageSlowDebuffDuration"),
	DownedDuration UMETA(DisplayName = "DownedDuration"),
	DownedInvincibleDuration UMETA(DisplayName = "DownedInvincibleDuration"),
};