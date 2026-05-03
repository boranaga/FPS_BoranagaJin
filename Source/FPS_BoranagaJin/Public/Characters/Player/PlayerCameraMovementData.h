

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveVector.h"
#include "Engine/DataTable.h"
#include "PlayerCameraMovementData.generated.h"

class UCameraAnimationSequence;

USTRUCT(BlueprintType)
struct FMovementCameraData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "목표 시야각"))
	float TargetFOV = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "시야각 전환 속도"))
	float FOVInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "목표 초점 거리"))
	float TargetFocalDistance = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "초점 전환 속도"))
	float FocalDistanceInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "목표 카메라 위치"))
	FVector TargetCameraPosition = FVector(0, 0, 70.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "카메라 위치 전환 속도"))
	float CameraPositionInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "목표 카메라 회전"))
	float TargetCameraRoll = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "카메라 회전 전환 속도"))
	float CameraRotationInterpSpeed = 10.f;

};

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FPlayerCameraMovementRow : public FTableRowBase
{
	GENERATED_BODY()

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


	// Loops
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


	// One shot cam shakes
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
};