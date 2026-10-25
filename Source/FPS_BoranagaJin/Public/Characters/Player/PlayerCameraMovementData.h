

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

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FPlayerCameraZoomRequest
{
	GENERATED_BODY()
	// 최종적으로 원하는 절대 FOV.
	// 예: 기본 FOV가 90이고 ADS 시 65를 원하면 65.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float TargetFOV = 65.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float BlendInSpeed = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "0.0"))
	float BlendOutSpeed = 10.f;

	// 동시에 여러 Zoom Request가 들어온 경우 높은 Priority 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	int32 Priority = 0;
};


USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FPlayerCameraMovementRequest
{
	GENERATED_BODY()

	// 기존 Movement Camera 위치에 추가되는 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Movement")
	FVector LocationOffset = FVector::ZeroVector;

	// 실제 ControlRotation을 변경하지 않는 시각적 Camera Rotation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Movement")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Movement", meta = (ClampMin = "0.0"))
	float BlendInSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Movement", meta = (ClampMin = "0.0"))
	float BlendOutSpeed = 10.f;

	// 동일 Request의 강도 조절용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Movement", meta = (ClampMin = "0.0"))
	float Weight = 1.f;
};