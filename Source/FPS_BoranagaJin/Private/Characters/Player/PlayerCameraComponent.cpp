


#include "Characters/Player/PlayerCameraComponent.h"
#include "Characters/Player/PlayerCameraMovementData.h"
#include "Characters/Player/CharacterPlayer.h"
#include "Characters/Player/PlayerMovementComponent.h"
//#include "ActorComponents/DamageComponent/ACDamageSystem.h"

#include "Subsystems/WorldSubsystem.h"
#include "CameraAnimationCameraModifier.h"
#include "Engine/World.h"

#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"


UPlayerCameraComponent::UPlayerCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerRef = GetOwner<ACharacterPlayer>();

	if (!IsValid(PlayerRef))
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerCameraComponent: PlayerRef is invalid."));
		return;
	}

	MovementComponent = PlayerRef->GetPlayerMovementComponent();
	PlayerCamera = PlayerRef->GetCameraComponent();
	PlayerController = PlayerRef->GetController<APlayerController>();

	InitCameraShakes();

	if (IsValid(MovementComponent))
	{
		MovementComponent->OnMoveDelegate.AddDynamic(this, &UPlayerCameraComponent::OnMove);
		MovementComponent->OnAirborneDelegate.AddDynamic(this, &UPlayerCameraComponent::OnAirborne);
		MovementComponent->OnSlideDelegate.AddDynamic(this, &UPlayerCameraComponent::OnSlide);
		MovementComponent->OnWallRunDelegate.AddDynamic(this, &UPlayerCameraComponent::OnWallRun);

		MovementComponent->OnLandDelegate.AddDynamic(this, &UPlayerCameraComponent::OnLand);
		MovementComponent->OnPrimaryJumpDelegate.AddDynamic(this, &UPlayerCameraComponent::OnPrimaryJump);
		MovementComponent->OnWallJumpDelegate.AddDynamic(this, &UPlayerCameraComponent::OnWallJump);
		MovementComponent->OnMantleDelegate.AddDynamic(this, &UPlayerCameraComponent::OnMantle);
		MovementComponent->OnDashDelegate.AddDynamic(this, &UPlayerCameraComponent::OnDash);
		MovementComponent->OnDownedDelegate.AddDynamic(this, &UPlayerCameraComponent::OnDowned);
	}

	DownedFloorImpactDelegate.BindWeakLambda(this, [this]()
		{
			PlayOneShotCameraShake(DownedFloorImpactShake);
			PlayOneShotCameraShake(DownedFloorIdleShake);
		});

	DownedGoingUpShakeDelegate.BindWeakLambda(this, [this]()
		{
			PlayOneShotCameraShake(DownedGoingUpShake);
		});
}

void UPlayerCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(PlayerCamera))
	{
		PlayerCamera->ClearAdditiveOffset();
	}

	ActiveZoomRequests.Empty();
	ActiveCameraMovementRequests.Empty();

	GetWorld()->GetTimerManager().ClearTimer(DownFloorImpactTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DownGoingUpTimerHandle);

	Super::EndPlay(EndPlayReason);
}

FGuid UPlayerCameraComponent::RequestZoom(const FPlayerCameraZoomRequest& Request)
{
	const FGuid Handle = FGuid::NewGuid();
	FActiveCameraZoomRequest ActiveRequest;
	ActiveRequest.Data = Request;
	ActiveRequest.RequestOrder = ++NextZoomRequestOrder;

	ActiveZoomRequests.Add(Handle, MoveTemp(ActiveRequest));

	return Handle;
}

bool UPlayerCameraComponent::UpdateZoomRequest(const FGuid& Handle, const FPlayerCameraZoomRequest& Request)
{
	FActiveCameraZoomRequest* ActiveRequest = ActiveZoomRequests.Find(Handle);
	if (!ActiveRequest) { return false; }
	ActiveRequest->Data = Request;
	ActiveRequest->RequestOrder = ++NextZoomRequestOrder;
	return true;
}

void UPlayerCameraComponent::ReleaseZoom(const FGuid& Handle)
{
	FActiveCameraZoomRequest* Request = ActiveZoomRequests.Find(Handle);
	if (!Request) { return; }
	if (Handle == CurrentResolvedZoomHandle)
	{
		LastZoomBlendOutSpeed = Request->Data.BlendOutSpeed;
	}
	ActiveZoomRequests.Remove(Handle);
}

FGuid UPlayerCameraComponent::RequestCameraMovement(const FPlayerCameraMovementRequest& Request)
{
	const FGuid Handle = FGuid::NewGuid();
	FActiveCameraMovementRequest ActiveRequest;
	ActiveRequest.Data = Request;
	ActiveCameraMovementRequests.Add(Handle, MoveTemp(ActiveRequest));
	return Handle;
}

bool UPlayerCameraComponent::UpdateCameraMovementRequest(const FGuid& Handle, const FPlayerCameraMovementRequest& Request)
{
	FActiveCameraMovementRequest* ActiveRequest = ActiveCameraMovementRequests.Find(Handle);
	if (!ActiveRequest) { return false; }
	ActiveRequest->Data = Request;
	ActiveRequest->bReleasing = false;
	return true;
}

void UPlayerCameraComponent::ReleaseCameraMovement(const FGuid& Handle)
{
	FActiveCameraMovementRequest* Request = ActiveCameraMovementRequests.Find(Handle);
	if (!Request) { return; }
	Request->bReleasing = true;
}

APlayerCameraManager* UPlayerCameraComponent::GetPlayerCameraManager()
{
	if (!IsValid(PlayerController) && IsValid(PlayerRef))
	{
		PlayerController = PlayerRef->GetController<APlayerController>();
	}

	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	return PlayerController->PlayerCameraManager;
}

const FActiveCameraZoomRequest* UPlayerCameraComponent::FindHighestPriorityZoomRequest(FGuid& OutHandle) const
{
	const FActiveCameraZoomRequest* BestRequest = nullptr;
	int32 BestPriority = TNumericLimits<int32>::Lowest();
	uint64 BestOrder = 0;

	for (const TPair<FGuid, FActiveCameraZoomRequest>& Pair : ActiveZoomRequests)
	{
		const FActiveCameraZoomRequest& Request = Pair.Value;
		const int32 Priority = Request.Data.Priority;

		if (!BestRequest || Priority > BestPriority || (Priority == BestPriority && Request.RequestOrder > BestOrder))
		{
			BestRequest = &Request;

			BestPriority = Priority;
			BestOrder = Request.RequestOrder;

			OutHandle = Pair.Key;
		}
	}

	return BestRequest;
}

void UPlayerCameraComponent::TickExternalCameraRequests(float DeltaTime)
{
	TickZoomRequests(DeltaTime);
	TickCameraMovementRequests(DeltaTime);

	ApplyExternalCameraOffset();
}

void UPlayerCameraComponent::TickZoomRequests(float DeltaTime)
{
	if (!IsValid(PlayerCamera)) { return; }
	FGuid BestHandle;
	const FActiveCameraZoomRequest* Request = FindHighestPriorityZoomRequest(BestHandle);
	float TargetFOVOffset = 0.f;
	float InterpSpeed = LastZoomBlendOutSpeed;
	if (Request)
	{
		const float TargetFOV = FMath::Clamp(Request->Data.TargetFOV, 5.f, 170.f);
		TargetFOVOffset = TargetFOV - PlayerCamera->FieldOfView;
		InterpSpeed = Request->Data.BlendInSpeed;
		CurrentResolvedZoomHandle = BestHandle;
	}
	else
	{
		CurrentResolvedZoomHandle.Invalidate();
	}

	//MEMO: 이렇게하면 선형적으로 Interp 되는 것이 아니라, 지수적으로 Interp되는 것 아닌가? Interp 함수 구현 살펴보아야함.
	CurrentZoomFOVOffset = FMath::FInterpTo(CurrentZoomFOVOffset, TargetFOVOffset, DeltaTime, InterpSpeed);

	if (!Request && FMath::IsNearlyZero(CurrentZoomFOVOffset, 0.01f))
	{
		CurrentZoomFOVOffset = 0.f;
	}
}

void UPlayerCameraComponent::TickCameraMovementRequests(float DeltaTime)
{
	CurrentExternalLocationOffset = FVector::ZeroVector;
	CurrentExternalRotationOffset = FRotator::ZeroRotator;
	TArray<FGuid> RemoveRequests;

	for (TPair<FGuid, FActiveCameraMovementRequest>& Pair : ActiveCameraMovementRequests)
	{
		FActiveCameraMovementRequest& Request = Pair.Value;
		const float Weight = FMath::Max(Request.Data.Weight, 0.f);

		FVector TargetLocation = FVector::ZeroVector;
		FRotator TargetRotation = FRotator::ZeroRotator;

		float InterpSpeed = Request.Data.BlendOutSpeed;

		if (!Request.bReleasing)
		{
			TargetLocation = Request.Data.LocationOffset * Weight;
			TargetRotation = FRotator(
				Request.Data.RotationOffset.Pitch * Weight,
				Request.Data.RotationOffset.Yaw * Weight,
				Request.Data.RotationOffset.Roll * Weight
			);
			InterpSpeed = Request.Data.BlendInSpeed;
		}

		Request.CurrentLocationOffset =
			FMath::VInterpTo(
				Request.CurrentLocationOffset,
				TargetLocation,
				DeltaTime,
				InterpSpeed
			);

		Request.CurrentRotationOffset =
			FMath::RInterpTo(
				Request.CurrentRotationOffset,
				TargetRotation,
				DeltaTime,
				InterpSpeed
			);

		CurrentExternalLocationOffset += Request.CurrentLocationOffset;
		CurrentExternalRotationOffset += Request.CurrentRotationOffset;

		if (Request.bReleasing &&
			Request.CurrentLocationOffset.IsNearlyZero(0.01f) &&
			Request.CurrentRotationOffset.IsNearlyZero(0.01f))
		{
			RemoveRequests.Add(Pair.Key);
		}
	}

	for (const FGuid& Handle : RemoveRequests)
	{
		ActiveCameraMovementRequests.Remove(Handle);
	}
}

void UPlayerCameraComponent::ApplyExternalCameraOffset()
{
	if (!IsValid(PlayerCamera)) { return; }
	// 이전 프레임의 External Layer 제거
	PlayerCamera->ClearAdditiveOffset();

	const FTransform CameraOffset(
		FQuat(CurrentExternalRotationOffset),
		CurrentExternalLocationOffset,
		FVector::OneVector
	);

	PlayerCamera->AddAdditiveOffset(CameraOffset, CurrentZoomFOVOffset);
}

void UPlayerCameraComponent::ChangeCameraLoopShake(const TSubclassOf<UCameraShakeBase>& InShake)
{
	APlayerCameraManager* CameraManager = GetPlayerCameraManager();
	if (!IsValid(CameraManager)) { return; }

	// 이미 같은 Shake가 실행 중이라면 아무것도 하지 않음
	if (CurrentLoopShake == InShake) { return; }

	if (CurrentLoopShake)
	{
		CameraManager->StopAllInstancesOfCameraShake(CurrentLoopShake, false);
	}

	CurrentLoopShake = InShake;
	if (CurrentLoopShake)
	{
		CameraManager->StartCameraShake(CurrentLoopShake);
	}
}

UCameraShakeBase* UPlayerCameraComponent::RequestCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!ShakeClass) { return nullptr; }
	APlayerCameraManager* CameraManager = GetPlayerCameraManager();
	if (!IsValid(CameraManager)) { return nullptr; }
	return CameraManager->StartCameraShake(ShakeClass, Scale);
}

void UPlayerCameraComponent::StopCameraShake(UCameraShakeBase* ShakeInstance, bool bImmediately)
{
	if (!IsValid(ShakeInstance)) { return; }
	APlayerCameraManager* CameraManager = GetPlayerCameraManager();
	if (!IsValid(CameraManager)) { return; }
	CameraManager->StopCameraShake(ShakeInstance, bImmediately);
}


void UPlayerCameraComponent::PlayOneShotCameraShake(const TSubclassOf<UCameraShakeBase>& InShake)
{
	PlayerController->PlayerCameraManager->StartCameraShake(InShake);

	//--------------------

	RequestCameraShake(InShake, 1.f);
}

void UPlayerCameraComponent::ResetExternalCameraRequests(bool bImmediate)
{
	ActiveZoomRequests.Empty();
	CurrentResolvedZoomHandle.Invalidate();

	if (bImmediate)
	{
		ActiveCameraMovementRequests.Empty();
		CurrentZoomFOVOffset = 0.f;
		CurrentExternalLocationOffset =	FVector::ZeroVector;
		CurrentExternalRotationOffset =	FRotator::ZeroRotator;
		if (IsValid(PlayerCamera))
		{
			PlayerCamera->ClearAdditiveOffset();
		}
		return;
	}

	for (TPair<FGuid, FActiveCameraMovementRequest>& Pair : ActiveCameraMovementRequests)
	{
		Pair.Value.bReleasing = true;
	}
}

void UPlayerCameraComponent::OnDamaged()
{
	PlayOneShotCameraShake(HitCameraShake);
}


void UPlayerCameraComponent::TickMoveStateCamera(float DeltaTime)
{
	if (MovementComponent->GetMovementState() == EMovementState::EMS_Move)
	{
		if (MovementComponent->IsCrouching())
		{
			if (MovementComponent->Velocity.IsNearlyZero(0.001))
			{
				ChangeCameraLoopShake(CrouchIdleCameraShake);
			}
			else
			{
				ChangeCameraLoopShake(CrouchMoveCameraShake);
			}

			InterpCameraData(CrouchCameraData, DeltaTime);
		}
		else
		{
			if (MovementComponent->Velocity.IsNearlyZero(0.001))
			{
				ChangeCameraLoopShake(IdleCameraShake);

				InterpCameraData(IdleCameraData, DeltaTime);
			}
			else if (MovementComponent->IsRunning())
			{
				ChangeCameraLoopShake(RunCameraShake);

				InterpCameraData(RunCameraData, DeltaTime);
			}
			else
			{
				ChangeCameraLoopShake(WalkCameraShake);

				if (MovementComponent->GetMovementInputVector().Y < 0.f)
				{
					InterpCameraData(WalkBackwardCameraData, DeltaTime);
				}
				else if (MovementComponent->GetMovementInputVector().IsZero())
				{
					InterpCameraData(IdleCameraData, DeltaTime);
				}
				else
				{
					InterpCameraData(WalkHorizontalCameraData, DeltaTime);
				}
			}
		}
	}
}

void UPlayerCameraComponent::TickWallRunStateCamera(float DeltaTime)
{
	if (MovementComponent->GetMovementState() == EMovementState::EMS_WallRun)
	{
		InterpCameraData(WallRunCameraData, DeltaTime);
	}
}

void UPlayerCameraComponent::TickSlideStateCamera(float DeltaTime)
{
	if (MovementComponent->GetMovementState() == EMovementState::EMS_Slide)
	{
		InterpCameraData(SlideCameraData, DeltaTime);
	}
}

void UPlayerCameraComponent::TickAirborneStateCamera(float DeltaTime)
{
	if (MovementComponent->GetMovementState() == EMovementState::EMS_Airborne)
	{
		InterpCameraData(FallCameraData, DeltaTime);
	}
}

void UPlayerCameraComponent::InterpCameraData(const FMovementCameraData& InData, float DeltaTime)
{
	PlayerCamera->SetFieldOfView(FMath::FInterpTo(PlayerCamera->FieldOfView, InData.TargetFOV, DeltaTime, InData.FOVInterpSpeed));
	PlayerCamera->PostProcessSettings.DepthOfFieldFocalDistance =
		FMath::FInterpTo(PlayerCamera->PostProcessSettings.DepthOfFieldFocalDistance,
			InData.TargetFocalDistance, DeltaTime, InData.FocalDistanceInterpSpeed);
	PlayerCamera->SetRelativeLocation(FMath::VInterpTo(PlayerCamera->GetRelativeLocation(),
		InData.TargetCameraPosition, DeltaTime, InData.CameraPositionInterpSpeed));

	if (MovementComponent->GetMovementState() != EMovementState::EMS_WallRun)
	{
		FRotator CurrentControlRot = PlayerController->GetControlRotation();
		FRotator NewControlRot = FMath::RInterpTo(CurrentControlRot, FRotator(CurrentControlRot.Pitch, CurrentControlRot.Yaw, InData.TargetCameraRoll),
			DeltaTime, InData.CameraRotationInterpSpeed);
		PlayerController->SetControlRotation(NewControlRot);
	}

}

// Called every frame
void UPlayerCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TickMoveStateCamera(DeltaTime);
	TickWallRunStateCamera(DeltaTime);
	TickSlideStateCamera(DeltaTime);
	TickAirborneStateCamera(DeltaTime);

	TickExternalCameraRequests(DeltaTime);
}

void UPlayerCameraComponent::OnAirborne()
{
	ChangeCameraLoopShake(AirborneCameraShake);
}

void UPlayerCameraComponent::OnMove()
{

}

void UPlayerCameraComponent::OnWallRun()
{
	PlayOneShotCameraShake(WallLandCameraShake);
	ChangeCameraLoopShake(WallRunCameraShake);
}

void UPlayerCameraComponent::OnSlide()
{
	ChangeCameraLoopShake(SlideCameraShake);
}

void UPlayerCameraComponent::OnMantle()
{
	PlayOneShotCameraShake(MantleCameraShake);
}


void UPlayerCameraComponent::OnPrimaryJump()
{
	PlayOneShotCameraShake(PrimaryJumpCameraShake);
}

void UPlayerCameraComponent::OnDoubleJump()
{
	PlayOneShotCameraShake(DoubleJumpCameraShake);
}

void UPlayerCameraComponent::OnWallJump()
{
	PlayOneShotCameraShake(WallJumpCameraShake);
}

void UPlayerCameraComponent::OnLand(float ZVelocity)
{
	if (ZVelocity < -2800.f)
	{
		PlayOneShotCameraShake(StrongLandCameraShake);
	}
	else if (ZVelocity < -1800.f)
	{
		PlayOneShotCameraShake(MediumLandCameraShake);
	}
	else if (ZVelocity <= -800.f)
	{
		PlayOneShotCameraShake(LightLandCameraShake);
	}
}

void UPlayerCameraComponent::OnDash(FVector2D MovementInput)
{
	if (MovementInput.IsNearlyZero(0.001))
	{
		PlayOneShotCameraShake(ForwardDashCameraShake);
	}
	else if (MovementInput.Y > 0.f)
	{
		if (MovementInput.X > 0.f)
		{
			PlayOneShotCameraShake(ForwardRightDashCameraShake);
		}
		else if (MovementInput.X < 0.f)
		{
			PlayOneShotCameraShake(ForwardLeftDashCameraShake);
		}
		else
		{
			PlayOneShotCameraShake(ForwardDashCameraShake);
		}
	}
	else if (MovementInput.Y < 0.f)
	{
		if (MovementInput.X > 0.f)
		{
			PlayOneShotCameraShake(BackwardRightDashCameraShake);
		}
		else if (MovementInput.X < 0.f)
		{
			PlayOneShotCameraShake(BackwardLeftDashCameraShake);
		}
		else
		{
			PlayOneShotCameraShake(BackwardDashCameraShake);
		}
	}
	else
	{
		if (MovementInput.X > 0.f)
		{
			PlayOneShotCameraShake(RightDashCameraShake);
		}
		else if (MovementInput.X < 0.f)
		{
			PlayOneShotCameraShake(LeftDashCameraShake);
		}
		else
		{
			PlayOneShotCameraShake(ForwardDashCameraShake);
		}
	}

}

void UPlayerCameraComponent::OnDowned()
{
	PlayOneShotCameraShake(DownedImpactShake);

	GetWorld()->GetTimerManager().SetTimer(DownFloorImpactTimerHandle, DownedFloorImpactDelegate, 0.4f, false);
	GetWorld()->GetTimerManager().SetTimer(DownGoingUpTimerHandle, DownedGoingUpShakeDelegate, 1.2f, false);

}

void UPlayerCameraComponent::OnDeath()
{
	PlayOneShotCameraShake(DeathStartCameraShake);

	TWeakObjectPtr WeakThis = this;

	FTimerHandle DeathFloorHitTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(DeathFloorHitTimerHandle, [WeakThis]()
		{
			if (UPlayerCameraComponent* Comp = WeakThis.Get())
			{
				Comp->PlayOneShotCameraShake(Comp->DeathFloorHitCameraShake);
			}
		}, 0.5, false);

	FTimerHandle DeathFadeTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(DeathFadeTimerHandle, [WeakThis]()
		{
			if (auto Comp = WeakThis.Get())
			{
				Comp->PlayerController->PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.5f, FColor::Black, true,
					true);
			}
		}, 0.7, false);
}

void UPlayerCameraComponent::InitCameraShakes()
{
	FPlayerCameraMovementRow* Row = CameraDataTable->FindRow<FPlayerCameraMovementRow>("Player", "");
	if (!Row) return;

	IdleCameraData = Row->IdleCameraData;
	WalkHorizontalCameraData = Row->WalkHorizontalCameraData;
	WalkBackwardCameraData = Row->WalkBackwardCameraData;
	RunCameraData = Row->RunCameraData;
	WallRunCameraData = Row->WallRunCameraData;
	SlideCameraData = Row->SlideCameraData;
	FallCameraData = Row->FallCameraData;
	CrouchCameraData = Row->CrouchCameraData;
	IdleCameraShake = Row->IdleCameraShake;
	WalkCameraShake = Row->WalkCameraShake;
	RunCameraShake = Row->RunCameraShake;
	CrouchIdleCameraShake = Row->CrouchIdleCameraShake;
	CrouchMoveCameraShake = Row->CrouchMoveCameraShake;
	AirborneCameraShake = Row->AirborneCameraShake;
	WallRunCameraShake = Row->WallRunCameraShake;
	SlideCameraShake = Row->SlideCameraShake;
	PrimaryJumpCameraShake = Row->PrimaryJumpCameraShake;
	DoubleJumpCameraShake = Row->DoubleJumpCameraShake;
	WallJumpCameraShake = Row->WallJumpCameraShake;
	LightLandCameraShake = Row->LightLandCameraShake;
	MediumLandCameraShake = Row->MediumLandCameraShake;
	StrongLandCameraShake = Row->StrongLandCameraShake;
	WallLandCameraShake = Row->WallLandCameraShake;
	MantleCameraShake = Row->MantleCameraShake;
	ForwardDashCameraShake = Row->ForwardDashCameraShake;
	ForwardRightDashCameraShake = Row->ForwardRightDashCameraShake;
	ForwardLeftDashCameraShake = Row->ForwardLeftDashCameraShake;
	LeftDashCameraShake = Row->LeftDashCameraShake;
	RightDashCameraShake = Row->RightDashCameraShake;
	BackwardDashCameraShake = Row->BackwardDashCameraShake;
	BackwardRightDashCameraShake = Row->BackwardRightDashCameraShake;
	BackwardLeftDashCameraShake = Row->BackwardLeftDashCameraShake;
	HitCameraShake = Row->HitCameraShake;
}
