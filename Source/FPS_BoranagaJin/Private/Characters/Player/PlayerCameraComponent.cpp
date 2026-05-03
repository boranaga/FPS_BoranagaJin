


#include "Characters/Player/PlayerCameraComponent.h"
#include "Characters/Player/PlayerCameraMovementData.h"
#include "Characters/Player/CharacterPlayer.h"
#include "Characters/Player/PlayerMovementComponent.h"
//#include "ActorComponents/DamageComponent/ACDamageSystem.h"

#include "Subsystems/WorldSubsystem.h"
#include "CameraAnimationCameraModifier.h"
#include "Engine/World.h"

#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"


UPlayerCameraComponent::UPlayerCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacterPlayer* Player = GetOwner<ACharacterPlayer>())
	{
		PlayerRef = Player;
		MovementComponent = Player->GetPlayerMovementComponent();
		PlayerCamera = Player->GetCameraComponent();
		PlayerController = Player->GetController<APlayerController>();
	}

	InitCameraShakes();

	//PlayerRef->GetDamageSystemComponent()->OnDeath.AddUObject(this, &ThisClass::OnDeath);

	MovementComponent->OnMoveDelegate.AddDynamic(this, &UPlayerCameraComponent::OnMove);
	MovementComponent->OnAirborneDelegate.AddDynamic(this, &UPlayerCameraComponent::OnAirborne);
	MovementComponent->OnSlideDelegate.AddDynamic(this, &UPlayerCameraComponent::OnSlide);
	MovementComponent->OnWallRunDelegate.AddDynamic(this, &UPlayerCameraComponent::OnWallRun);

	MovementComponent->OnLandDelegate.AddDynamic(this, &UPlayerCameraComponent::OnLand);
	MovementComponent->OnPrimaryJumpDelegate.AddDynamic(this, &UPlayerCameraComponent::OnPrimaryJump);
	MovementComponent->OnDoubleJumpDelegate.AddDynamic(this, &UPlayerCameraComponent::OnDoubleJump);
	MovementComponent->OnWallJumpDelegate.AddDynamic(this, &UPlayerCameraComponent::OnWallJump);
	MovementComponent->OnMantleDelegate.AddDynamic(this, &UPlayerCameraComponent::OnMantle);
	MovementComponent->OnDashDelegate.AddDynamic(this, &UPlayerCameraComponent::OnDash);
	MovementComponent->OnDownedDelegate.AddDynamic(this, &UPlayerCameraComponent::OnDowned);

	DownedFloorImpactDelegate.BindWeakLambda(this, [&]
		{
			PlayOneShotCameraShake(DownedFloorImpactShake);
			PlayOneShotCameraShake(DownedFloorIdleShake);
		});

	DownedGoingUpShakeDelegate.BindWeakLambda(this, [&]
		{
			PlayOneShotCameraShake(DownedGoingUpShake);
		});

}

void UPlayerCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	//PlayerRef->GetDamageSystemComponent()->OnDeath.RemoveAll(this);
}


void UPlayerCameraComponent::ChangeCameraLoopShake(const TSubclassOf<UCameraShakeBase>& InShake)
{
	if (CurrentLoopShake && CurrentLoopShake != InShake)
	{
		PlayerController->PlayerCameraManager->StopAllInstancesOfCameraShake(CurrentLoopShake, false);
	}
	CurrentLoopShake = InShake;
	PlayerController->PlayerCameraManager->StartCameraShake(InShake);

}

void UPlayerCameraComponent::PlayOneShotCameraShake(const TSubclassOf<UCameraShakeBase>& InShake)
{
	PlayerController->PlayerCameraManager->StartCameraShake(InShake);
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
