
#include "Characters/Player/PlayerMovementComponent.h"

#include "Characters/Player/PlayerMovementData.h"
#include "Characters/Player/CharacterPlayer.h"

#include "Items/Weapons/Weapon.h"
#include "Items/Weapons/WeaponSystemComponent.h"
#include "Items/WeaponState/WeaponBaseState.h"
#include "UI/WeaponAimUIWidget.h"

#include "EngineCamerasSubsystem.h"
#include "KismetTraceUtils.h"
#include "Animations/CameraAnimationCameraModifier.h"
#include "Camera/CameraComponent.h"

#include "Components/CapsuleComponent.h"
#include "Curves/CurveVector.h"
#include "GameModes/DefaultGameMode.h" //TODO: GameMode 구현

#define WALL_TRACE_CHANNEL ECC_GameTraceChannel2 //TODO: 수정
#define ENEMY_TRACE_CHANNEL ECC_GameTraceChannel6 //TODO: 수정

UPlayerMovementComponent::UPlayerMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
}

void UPlayerMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (PawnOwner)
	{
		CharacterPlayer = Cast<ACharacterPlayer>(PawnOwner);
		PlayerController = Cast<APlayerController>(PawnOwner->GetController());
	}

	// Init trigger box related maps
	InitMovementDataTypeMap();
	InitMovementTriggerKeyMap();

	// Initialize player movement data values.
	ApplyMovementDataTable();

	// Start as airborne state
	CurrentMovementState = EMovementState::EMS_Airborne;
	OnAirborneDelegate.Broadcast();
}

void UPlayerMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bPrintMovementDebug)
	{
		GEngine->AddOnScreenDebugMessage(
			0, 0.f, FColor::Cyan, FString::Printf(TEXT("Current State : %s"),
				*UEnum::GetValueAsString(CurrentMovementState)));
		GEngine->AddOnScreenDebugMessage(
			1, 0.f, FColor::Cyan,
			FString::Printf(TEXT("Velocity : (%f, %f, %f) / Size : %f / Size2D : %f "),
				Velocity.X, Velocity.Y, Velocity.Z, Velocity.Size(), Velocity.Size2D()));
	}

	if (!PawnOwner || !UpdatedComponent) { return; }

	ConsumeDeadRequest();

	//// If requested reset and remove all movement modifications.
	//if (bMovementModificationResetRequested)
	//{
	//	bMovementModificationResetRequested = false;

	//	bMovementKeyHoldActive = false;
	//	for (auto& Elem : MovementTriggerKeyMap)
	//	{
	//		Elem.Value = false;
	//	}
	//	MovementDataModifiers.Empty();

	//	ApplyMovementDataTable();
	//}

	//ApplyMovementDataModifiers();
	//ApplyKeyHoldModifiers();

	//UpdateDashGauge(DeltaTime);

	UpdateWallCooldowns();

	if (!bControllerTilting)
	{
		if (PlayerController->GetControlRotation().Roll != 0.f)
		{
			FRotator CurrentControlRotation = CharacterPlayer->GetControlRotation();
			FRotator NewRotation = FMath::RInterpTo(CurrentControlRotation, FRotator(CurrentControlRotation.Pitch, CurrentControlRotation.Yaw, 0.f), DeltaTime, 7.f);
			PlayerController->SetControlRotation(NewRotation);
		}
	}

	UpdateDamageFlags();

	CacheInput();

	TickState(DeltaTime);

	FVector DesiredTickMovement = Velocity * DeltaTime;
	if (!DesiredTickMovement.IsNearlyZero())
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(DesiredTickMovement, UpdatedComponent->GetComponentRotation(), true, Hit);
		if (Hit.IsValidBlockingHit())
		{
			if (CurrentMovementState == EMovementState::EMS_Move && Hit.ImpactNormal.Z < MinWalkableFloorZ)
			{
				FVector AdjustedTickMovement = FVector(DesiredTickMovement.X, DesiredTickMovement.Y, 0).GetSafeNormal() * DesiredTickMovement.Size();
				FVector AdjustedNormal = FVector(Hit.Normal.X, Hit.Normal.Y, 0).GetSafeNormal();
				SlideAlongSurface(AdjustedTickMovement, 1.f - Hit.Time, AdjustedNormal, Hit);
			}
			else
			{
				SlideAlongSurface(DesiredTickMovement, 1.f - Hit.Time, Hit.Normal, Hit);
			}
		}
	}

	UpdateComponentVelocity();
}

void UPlayerMovementComponent::TickState(float DeltaTime)
{
	switch (CurrentMovementState)
	{
	case EMovementState::EMS_Move:
		TickMove(DeltaTime);
		break;
	case EMovementState::EMS_Slide:
		TickSlide(DeltaTime);
		break;
	case EMovementState::EMS_Airborne:
		TickAirborne(DeltaTime);
		break;
	case EMovementState::EMS_WallRun:
		TickWallRun(DeltaTime);
		break;
	case EMovementState::EMS_Mantle:
		TickMantle(DeltaTime);
		break;
	case EMovementState::EMS_Downed:
		TickDowned(DeltaTime);
		break;
	case EMovementState::EMS_Dead:
		TickDead(DeltaTime);
		break;
	default:
		// UE_LOG(LogTemp, Error, TEXT("CurrentMovementState not valid"));
		break;
	}
}

void UPlayerMovementComponent::CrouchCapsule(float DeltaTime)
{
	UCapsuleComponent* PlayerCapsule = CharacterPlayer->GetCapsuleComponent();
	if (!PlayerCapsule) return;
	float CurrentHalfHeight = PlayerCapsule->GetScaledCapsuleHalfHeight();
	if (CurrentHalfHeight > CrouchCapsuleHalfHeight)
	{
		float NewHeight = FMath::FInterpTo(CurrentHalfHeight, CrouchCapsuleHalfHeight, DeltaTime, 7.f);
		float HeightDelta = CurrentHalfHeight - NewHeight;
		PlayerCapsule->SetCapsuleHalfHeight(NewHeight);
		CharacterPlayer->AddActorWorldOffset(FVector(0, 0, -HeightDelta));
	}
}

void UPlayerMovementComponent::UnCrouchCapsule(float DeltaTime)
{
	UCapsuleComponent* PlayerCapsule = CharacterPlayer->GetCapsuleComponent();
	float CurrentHalfHeight = PlayerCapsule->GetScaledCapsuleHalfHeight();
	if (CurrentHalfHeight < DefaultCapsuleHalfHeight)
	{
		float NewHeight = FMath::FInterpTo(CurrentHalfHeight, DefaultCapsuleHalfHeight, DeltaTime, 7.f);
		float HeightDelta = NewHeight - CurrentHalfHeight;
		CharacterPlayer->AddActorWorldOffset(FVector(0, 0, HeightDelta));
		PlayerCapsule->SetCapsuleHalfHeight(NewHeight);
	}
}

void UPlayerMovementComponent::TickMove(float DeltaTime)
{
	if (bDownedDamage)
	{
		bDownedDamage = false;
		SetMovementState(EMovementState::EMS_Downed);
		return;
	}

	if (bIsStepping)
	{
		FVector StepWallRight = FVector::CrossProduct(StepWallHit.ImpactNormal, FVector::UpVector).GetSafeNormal();
		FVector StepUpDir = FVector::CrossProduct(StepWallRight, StepWallHit.ImpactNormal).GetSafeNormal();

		FHitResult SteppingFrontHit;
		FCollisionQueryParams SteppingFrontParams;
		SteppingFrontParams.AddIgnoredActor(CharacterPlayer);

		bool bSteppingFrontHit = GetWorld()->SweepSingleByChannel(SteppingFrontHit, CharacterPlayer->GetActorLocation(),
			CharacterPlayer->GetActorLocation() + CharacterPlayer->GetActorForwardVector() * 50.f, CharacterPlayer->GetActorQuat(),
			ECC_WorldStatic, CharacterPlayer->GetCapsuleComponent()->GetCollisionShape(), SteppingFrontParams);

		Velocity = StepUpDir * (bIsRunning ? RunSpeed : WalkSpeed);

		Velocity = bDamageSlowDebuff ? Velocity * DamageSlowDebuffMultiplier : Velocity;


		if (!bSteppingFrontHit || FVector::DotProduct(Input.WorldInputDir, StepWallHit.ImpactNormal) >= 0.f)
		{
			Velocity = FVector::VectorPlaneProject(Input.WorldInputDir, StepFloorHit.ImpactNormal).GetSafeNormal() * LastVelocityBeforeStep.Size();
			bIsStepping = false;
		}

		return;
	}

	if (!IsGrounded())
	{
		AirborneStartTime = GetWorld()->GetTimeSeconds();
		bCoyoteTimeActivated = true;

		// CurrentJumpCount++; Commented out since we are going to implement coyote time!
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (Input.bCrouchHeld)
	{
		bIsCrouching = true;

		if (Velocity.Size() >= RunSpeed - 50.f && GroundHit.ImpactNormal.Z >= MinWalkableFloorZ && !bIsDashing)
		{
			SlideStartDirection = FVector::VectorPlaneProject(Velocity, GroundHit.ImpactNormal).GetSafeNormal();
			Velocity = bHasRecentlySlid ? SlideStartDirection * Velocity.Size() : SlideStartDirection * (Velocity.Size() + SlideAdditionalSpeed);
			SetIsDashing(false);
			SlideResetTimer = 0.f;
			SetMovementState(EMovementState::EMS_Slide);
			return;
		}

		CrouchCapsule(DeltaTime);
	}
	else
	{
		float CurrentCapsuleHalfHeight = CharacterPlayer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		if (GetWorld() && CurrentCapsuleHalfHeight < DefaultCapsuleHalfHeight)
		{
			FHitResult UnCrouchHitResult;
			FCollisionQueryParams UnCrouchParams;
			UnCrouchParams.AddIgnoredActor(CharacterPlayer);



			FVector UnCrouchSweepStart = CharacterPlayer->GetActorLocation();
			// (DefaultHalfHeight - CurrentHalfHeight) for default height assumed actor location adjustment
			// (DefaultHalfHeight - DefaultRadius) for default capsule half height excluding hemisphere
			// 5.f for offset
			// (DefaultHalfHeight - CurrentHalfHeight) + (DefaultHalfHeight - DefaultRadius) + 5.f
			// There is a built-in function to get the half height without hemisphere but didn't want to make another member var
			FVector UnCrouchSweepEnd = UnCrouchSweepStart + FVector::UpVector * (2 * DefaultCapsuleHalfHeight - CurrentCapsuleHalfHeight - DefaultCapsuleRadius + 5.f);

			bool bUnCrouchHit = GetWorld()->SweepSingleByChannel(
				UnCrouchHitResult,
				UnCrouchSweepStart,
				UnCrouchSweepEnd,
				FQuat::Identity,
				ECC_Visibility,
				FCollisionShape::MakeSphere(DefaultCapsuleRadius),
				UnCrouchParams);

			if (!bUnCrouchHit)
			{
				bIsCrouching = false;

				UnCrouchCapsule(DeltaTime);
			}
		}
	}

	if (bHasRecentlySlid)
	{
		SlideResetTimer += DeltaTime;
		if (SlideResetTimer >= SlideBoostResetDelay)
		{
			bHasRecentlySlid = false;
			SlideElapsedTime = 0.f;
			SlideResetTimer = 0.;
		}
	}

	if (bIsDashing)
	{
		if (ElapsedTimeFromDash < DashDecelerationTime)
		{
			ElapsedTimeFromDash += DeltaTime;
			float T = ElapsedTimeFromDash / DashDecelerationTime;
			FVector CalculatedVelocity = Velocity.GetSafeNormal() * FMath::InterpEaseIn(DashStartSpeed, DashEndSpeed, T, 2.f);
			Velocity = CalculatedVelocity;

			if (FVector::DotProduct(Input.WorldInputDir, Velocity.GetSafeNormal2D()) < 0.f)
			{
				SetIsDashing(false);
				ElapsedTimeFromDash = 0.f;
				Velocity = FVector::ZeroVector;
			}
		}
		else
		{
			SetIsDashing(false);
			ElapsedTimeFromDash = 0.f;
			Velocity = Velocity.GetSafeNormal() * DashEndSpeed;
		}
	}
	else
	{
		if (Input.MovementInput2D.IsNearlyZero())
		{
			if (Velocity.Size() > 0.f)
			{
				FVector DeltaVelocity = -Velocity.GetSafeNormal() * Deceleration * DeltaTime;
				if (DeltaVelocity.Size() > Velocity.Size())
				{
					Velocity = FVector::ZeroVector;
				}
				else
				{
					Velocity += DeltaVelocity;
				}
			}
		}
		else
		{
			if (bIsRunning)
			{
				if (AWeapon* CurrentWeapon = CharacterPlayer->GetWeaponSystemComponent()->GetCurrentWeapon())
				{
					EWeaponStateType CurrentWeaponState = CurrentWeapon->GetCurrentState()->GetWeaponStateType();
					if (CurrentWeaponState == EWeaponStateType::WeaponStateType_Firing)
					{
						bIsRunning = false;
					}
				}

				if (Input.MovementInput2D.Y <= 0)
				{
					bIsRunning = false;
				}
			}

			float WishSpeed = bIsCrouching ? CrouchSpeed : (bIsRunning ? RunSpeed : WalkSpeed);
			WishSpeed = bDamageSlowDebuff ? WishSpeed * DamageSlowDebuffMultiplier : WishSpeed;

			FVector AcceleratedVelocity = Velocity + Input.WorldInputDir * Acceleration * DeltaTime;
			Velocity = AcceleratedVelocity.Size() > WishSpeed ? AcceleratedVelocity.GetSafeNormal() * WishSpeed : AcceleratedVelocity;
		}

		if (GroundHit.ImpactNormal.Z >= MinWalkableFloorZ)
		{
			Velocity = FVector::VectorPlaneProject(Velocity, GroundHit.ImpactNormal).GetSafeNormal() * Velocity.Size();
		}
	}


	TArray<FHitResult> StepHits;
	FCollisionQueryParams StepParams;
	StepParams.AddIgnoredActor(CharacterPlayer);

	FVector StepTraceStart = CharacterPlayer->GetActorLocation() + FVector(0, 0, -1) * (CharacterPlayer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 5.f);
	FVector StepTraceEnd = StepTraceStart + FVector::VectorPlaneProject(Input.WorldInputDir, GroundHit.ImpactNormal).GetSafeNormal() * CharacterPlayer->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FCollisionShape StepShape = FCollisionShape::MakeSphere(5.f);

	bool bStepHit = GetWorld()->SweepMultiByChannel(StepHits, StepTraceStart, StepTraceEnd, FQuat::Identity, ECC_WorldStatic, StepShape, StepParams);

	// DrawDebugSphereTraceMulti(GetWorld(), StepTraceStart, StepTraceEnd,
	// 	5.f, EDrawDebugTrace::ForDuration, bStepHit, StepHits, FLinearColor::Red, FLinearColor::Green, 1.f);


	if (bStepHit)
	{
		bool bFoundStepWall = false;
		for (const FHitResult& StepHit : StepHits)
		{
			if (StepHit.IsValidBlockingHit() && StepHit.ImpactNormal.Z < MinWalkableFloorZ)
			{
				bFoundStepWall = true;
				StepWallHit = StepHit;
				break;
			}
		}

		if (bFoundStepWall)
		{
			FCollisionShape CapsuleShape = CharacterPlayer->GetCapsuleComponent()->GetCollisionShape();
			FPlane GroundPlane(GroundHit.ImpactPoint, GroundHit.ImpactNormal);
			float PlanePointZ = (GroundPlane.W - GroundPlane.X * StepWallHit.ImpactPoint.X - GroundPlane.Y * StepWallHit.ImpactPoint.Y) / GroundPlane.Z;
			float StepHeightZ = PlanePointZ + CapsuleShape.GetCapsuleHalfHeight() + MaxStepHeight;

			FVector DownStepTraceStart =
				FVector(StepWallHit.ImpactPoint.X, StepWallHit.ImpactPoint.Y, StepHeightZ) +
				FVector::VectorPlaneProject(Input.WorldInputDir, GroundHit.ImpactNormal).GetSafeNormal() * 3.f;
			FVector DownStepTraceEnd = DownStepTraceStart + FVector(0, 0, -1) * (StepHeightZ + 10.f);

			FCollisionQueryParams StepFloorParams;
			StepFloorParams.AddIgnoredActor(CharacterPlayer);

			bool bStepFloorHit = GetWorld()->SweepSingleByChannel(StepFloorHit, DownStepTraceStart, DownStepTraceEnd, FQuat::Identity, ECC_WorldStatic,
				CapsuleShape, StepFloorParams);

			// DrawDebugCapsuleTraceSingle(GetWorld(), DownStepTraceStart, DownStepTraceEnd, 40.f, 90.f, EDrawDebugTrace::ForDuration, bStepFloorHit,
			// 	StepFloorHit, FLinearColor::Green, FLinearColor::Red, 1.f);

			if (bStepFloorHit && StepFloorHit.IsValidBlockingHit() && StepFloorHit.ImpactNormal.Z >= MinWalkableFloorZ)
			{
				bIsStepping = true;
				LastVelocityBeforeStep = Velocity;
			}
		}
	}

	if (bGravityLaunchForceRequested)
	{
		bGravityLaunchForceRequested = false;

		Velocity.X = GravityLaunchForceDir.X * GravityLaunchForceAmount;
		Velocity.Y = GravityLaunchForceDir.Y * GravityLaunchForceAmount;
		Velocity.Z = GravityLaunchForceDir.Z * GravityLaunchForceAmount;
		CurrentJumpCount++;
		JumpPadInitialVelocityXY = FVector(Velocity.X, Velocity.Y, 0.f);
		SetIsDashing(false);
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (bJumpPadForceRequested)
	{
		bJumpPadForceRequested = false;
		Velocity.Z = JumpPadForceAmount;
		CurrentJumpCount++;
		SetIsDashing(false);
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (Input.bJumpPressed && CurrentJumpCount < MaxJumpCount)
	{
		//-----------------
		float WishSpeed = bIsCrouching ? CrouchSpeed : (bIsRunning ? RunSpeed : WalkSpeed);
		WishSpeed = bDamageSlowDebuff ? WishSpeed * DamageSlowDebuffMultiplier : WishSpeed;

		FVector AcceleratedVelocity = Velocity + Input.WorldInputDir * JumpHorizontalBoost * DeltaTime;
		Velocity = AcceleratedVelocity.Size() > WishSpeed ? AcceleratedVelocity.GetSafeNormal() * WishSpeed : AcceleratedVelocity;

		if (GroundHit.ImpactNormal.Z >= MinWalkableFloorZ)
		{
			Velocity = FVector::VectorPlaneProject(Velocity, GroundHit.ImpactNormal).GetSafeNormal() * Velocity.Size();
		}

		if (!Input.MovementInput2D.IsNearlyZero())
		{
			JumpBoostWindowRemaining = 0.f;
		}
		//-----------------
		CurrentJumpCount++;
		Velocity.Z = PrimaryJumpZVelocity;

		OnPrimaryJumpDelegate.Broadcast();
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (Input.bShiftPressed)
	{
		if (CurrStamina >= DashStaminaConsumeRate)
		{
			if (!bIsDashing && !bIsRunning)
			{
				SetIsDashing(true);
				bIsRunning = true;

				const FVector DashDirection = Input.WorldInputDir.IsNearlyZero() ? PawnOwner->GetActorForwardVector() : Input.WorldInputDir;
				Velocity = DashDirection * DashStartSpeed;
			}
		}
		else
		{
			if (!bIsDashing)
			{
				bIsRunning = !bIsRunning;
			}
		}
	}
	else
	{
		bIsRunning = false;
	}
}

void UPlayerMovementComponent::TickSlide(float DeltaTime)
{
	if (!IsGrounded() || GroundHit.ImpactNormal.Z < MinWalkableFloorZ)
	{
		AirborneStartTime = GetWorld()->GetTimeSeconds();
		bCoyoteTimeActivated = true;

		bShouldKeepSlideSpeed = true;
		LastSlideSpeedBeforeAirborne = Velocity;
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (!Input.bCrouchHeld)
	{
		bIsCrouching = false;
		SetMovementState(EMovementState::EMS_Move);
		return;
	}

	SlideStateElapsedTime += DeltaTime;

	CrouchCapsule(DeltaTime);

	bool bIsSlidingDown = false;
	FVector DownwardDirection = FVector::VectorPlaneProject(FVector::DownVector, GroundHit.ImpactNormal).GetSafeNormal();
	if (DownwardDirection.IsZero())
	{
		DownwardDirection = FVector::VectorPlaneProject(Velocity, GroundHit.ImpactNormal).GetSafeNormal();
		bIsSlidingDown = false;
	}
	else
	{
		FVector ProjectedVelocity = FVector::VectorPlaneProject(Velocity, GroundHit.ImpactNormal).GetSafeNormal();
		if (FVector::DotProduct(ProjectedVelocity, DownwardDirection) >= 0.f)
		{
			bIsSlidingDown = true;
		}
		else
		{
			bIsSlidingDown = false;
		}
	}

	FVector InterpDirection = FMath::VInterpNormalRotationTo(Velocity.GetSafeNormal(), DownwardDirection, DeltaTime, 40.f);
	FVector ProjectedSlideStartDirection = FVector::VectorPlaneProject(SlideStartDirection, GroundHit.ImpactNormal).GetSafeNormal();
	FVector SlideDirection = SlideStateElapsedTime > SlideInitialWindow ? InterpDirection : ProjectedSlideStartDirection;


	if (bIsSlidingDown)
	{
		Velocity = Velocity.Size() * SlideDirection;
	}
	else
	{
		if (SlideElapsedTime + DeltaTime < SlideMaxDuration)
		{
			SlideElapsedTime += DeltaTime;
		}
		else
		{
			bHasRecentlySlid = false;
			SlideElapsedTime = 0.f;
		}

		float DecelerationAmount = SlideDecelerationAmount * DeltaTime;
		float NewSpeed = Velocity.Size() - DecelerationAmount;
		if (NewSpeed <= CrouchSpeed)
		{
			Velocity = CrouchSpeed * SlideDirection;
			bHasRecentlySlid = false;
			SlideElapsedTime = 0.f;
			SetMovementState(EMovementState::EMS_Move);
			return;
		}

		Velocity = NewSpeed * SlideDirection;
	}

	if (bGravityLaunchForceRequested)
	{
		bGravityLaunchForceRequested = false;

		Velocity.X = GravityLaunchForceDir.X * GravityLaunchForceAmount;
		Velocity.Y = GravityLaunchForceDir.Y * GravityLaunchForceAmount;
		Velocity.Z = GravityLaunchForceDir.Z * GravityLaunchForceAmount;
		CurrentJumpCount++;
		JumpPadInitialVelocityXY = FVector(Velocity.X, Velocity.Y, 0.f);
		SetIsDashing(false);
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (bJumpPadForceRequested)
	{
		bJumpPadForceRequested = false;
		Velocity.Z = JumpPadForceAmount;
		CurrentJumpCount++;
		SetIsDashing(false);
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (Input.bJumpPressed && CurrentJumpCount < MaxJumpCount)
	{
		CurrentJumpCount++;
		Velocity.Z = PrimaryJumpZVelocity; //TODO: make this as function
		bShouldKeepSlideSpeed = SlideStateElapsedTime >= 0.1f; // TODO : Make this as a variable

		LastSlideSpeedBeforeAirborne = Velocity;
		OnPrimaryJumpDelegate.Broadcast();
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (Input.bShiftPressed)
	{
		if (DashGauge >= 1.f)
		{
			SetIsDashing(true);

			bIsRunning = true; // Player will run after dash ends

			DashGauge = FMath::Clamp(DashGauge - 1.f, 0.f, 2.f);

			const FVector DashDirection = Input.WorldInputDir.IsNearlyZero() ? PawnOwner->GetActorForwardVector() : Input.WorldInputDir;
			Velocity = DashDirection * DashStartSpeed;
			SetMovementState(EMovementState::EMS_Move);
			return;
		}
		else
		{
			if (!bIsDashing)
			{
				bIsRunning = !bIsRunning;
			}
		}
	}
}

void UPlayerMovementComponent::TickAirborne(float DeltaTime)
{
	if (PreviousMovementState == EMovementState::EMS_Move ||
		PreviousMovementState == EMovementState::EMS_WallRun ||
		PreviousMovementState == EMovementState::EMS_Slide)
	{
		ElapsedTimeFromSurface += DeltaTime;
	}

	if (bDownedDamage)
	{
		bDownedDamage = false;
		SetMovementState(EMovementState::EMS_Downed);
		return;
	}

	if (Input.bCrouchHeld)
	{
		bIsCrouching = true;
		CrouchCapsule(DeltaTime);
	}
	else
	{
		float CurrentCapsuleHalfHeight = CharacterPlayer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		if (GetWorld() && CurrentCapsuleHalfHeight < DefaultCapsuleHalfHeight)
		{
			FHitResult UnCrouchHitResult;
			FCollisionQueryParams UnCrouchParams;
			UnCrouchParams.AddIgnoredActor(CharacterPlayer);

			FVector UnCrouchSweepStart = CharacterPlayer->GetActorLocation();
			// (DefaultHalfHeight - CurrentHalfHeight) for default height assumed actor location adjustment
			// (DefaultHalfHeight - DefaultRadius) for default capsule half height excluding hemisphere
			// 5.f for offset
			// (DefaultHalfHeight - CurrentHalfHeight) + (DefaultHalfHeight - DefaultRadius) + 5.f
			// There is a built-in function to get the half height without hemisphere but didn't want to make another member var
			FVector UnCrouchSweepEnd = UnCrouchSweepStart + FVector::UpVector * (2 * DefaultCapsuleHalfHeight - CurrentCapsuleHalfHeight - DefaultCapsuleRadius + 5.f);

			bool bUnCrouchHit = GetWorld()->SweepSingleByChannel(
				UnCrouchHitResult,
				UnCrouchSweepStart,
				UnCrouchSweepEnd,
				FQuat::Identity,
				ECC_Visibility,
				FCollisionShape::MakeSphere(DefaultCapsuleRadius),
				UnCrouchParams);

			if (!bUnCrouchHit)
			{
				bIsCrouching = false;

				UnCrouchCapsule(DeltaTime);
			}
		}
	}

	if (IsGrounded())
	{
		if ((PreviousMovementState != EMovementState::EMS_Move && PreviousMovementState != EMovementState::EMS_Slide) || (ElapsedTimeFromSurface > JumpBuffer))
		{
			if (GroundHit.ImpactNormal.Z >= MinWalkableFloorZ)
			{
				if (Input.bCrouchHeld && Velocity.Size2D() >= RunSpeed)
				{
					SlideStartDirection = FVector::VectorPlaneProject(Velocity, GroundHit.ImpactNormal).GetSafeNormal();
					Velocity = bHasRecentlySlid ? SlideStartDirection * Velocity.Size() : SlideStartDirection * (Velocity.Size() + SlideAdditionalSpeed);
					SetIsDashing(false);
					ElapsedTimeFromDash = 0.f;
					SlideResetTimer = 0.f;

					SetMovementState(EMovementState::EMS_Slide);
					return;
				}

				if (bIsDashing || Input.bCrouchHeld || !Input.MovementInput2D.IsZero())
				{
					Velocity = FVector::VectorPlaneProject(Velocity, GroundHit.ImpactNormal).GetSafeNormal() * (bIsRunning ? RunSpeed : WalkSpeed);
				}
				else
				{
					Velocity = FVector::ZeroVector;
				}
			}
			SetMovementState(EMovementState::EMS_Move);
			return;
		}
	}

	FHitResult PreWallRightHit;
	FHitResult PreWallLeftHit;
	FCollisionQueryParams PreWallParams;
	PreWallParams.AddIgnoredActor(CharacterPlayer);
	FVector TraceStart = CharacterPlayer->GetActorLocation();
	FVector TraceRightEnd = TraceStart + CharacterPlayer->GetActorRightVector() * PreWallRunDetectionRange;
	FVector TraceLeftEnd = TraceStart + CharacterPlayer->GetActorRightVector() * (-PreWallRunDetectionRange);
	bool bPreWallRightHit = GetWorld()->SweepSingleByChannel(PreWallRightHit, TraceStart, TraceRightEnd, CharacterPlayer->GetActorQuat(),
		WALL_TRACE_CHANNEL, FCollisionShape::MakeSphere(40.f), PreWallParams);

	bool bPreWallLeftHit = GetWorld()->SweepSingleByChannel(PreWallLeftHit, TraceStart, TraceLeftEnd, CharacterPlayer->GetActorQuat(),
		WALL_TRACE_CHANNEL, FCollisionShape::MakeSphere(40.f), PreWallParams);

	if (bPreWallRightHit && FVector::DotProduct(CharacterPlayer->GetVelocity().GetSafeNormal2D(), PreWallRightHit.ImpactNormal) < 0.f)
	{
		bControllerTilting = true;
		FVector WallRunDirection = FVector::CrossProduct(PreWallRightHit.ImpactNormal, FVector::DownVector).GetSafeNormal();
		AddControllerRoll(DeltaTime, WallRunDirection, EWallRunSide::EWRS_Right);
	}
	else if (bPreWallLeftHit && FVector::DotProduct(CharacterPlayer->GetVelocity().GetSafeNormal2D(), PreWallLeftHit.ImpactNormal) < 0.f)
	{
		bControllerTilting = true;
		FVector WallRunDirection = FVector::CrossProduct(PreWallLeftHit.ImpactNormal, FVector::UpVector).GetSafeNormal();
		AddControllerRoll(DeltaTime, WallRunDirection, EWallRunSide::EWRS_Left);
	}
	else
	{
		bControllerTilting = false;
	}


	if (CanWallRun())
	{
		if (Velocity.Z >= 0.f)
		{
			WallRunEnterMode = EWallRunEnter::EWRE_Upward;
		}
		else if (Velocity.Z < -800.f)
		{
			WallRunEnterMode = EWallRunEnter::EWRE_Downward;
		}
		else
		{
			WallRunEnterMode = EWallRunEnter::EWRE_Neutral;
		}

		if (PreviousMovementState != EMovementState::EMS_WallRun)
		{
			SetIsDashing(false);
			ElapsedTimeFromDash = 0.f;
			SetMovementState(EMovementState::EMS_WallRun);
			return;
		}
		else
		{
			if (ElapsedTimeFromSurface > WallJumpBuffer)
			{
				SetIsDashing(false);
				ElapsedTimeFromDash = 0.f;
				SetMovementState(EMovementState::EMS_WallRun);
				return;
			}
		}
	}

	// Mantle Sweep Section
	FCollisionShape PlayerCapsule = CharacterPlayer->GetCapsuleComponent()->GetCollisionShape();
	FCollisionQueryParams MantleParams;
	MantleParams.AddIgnoredActor(CharacterPlayer);
	FVector MantleSweepEnd = CharacterPlayer->GetActorLocation() + CharacterPlayer->GetActorForwardVector() * PlayerCapsule.GetCapsuleRadius();
	bool bMantleWallHit = GetWorld()->SweepSingleByChannel(MantleWallHit, CharacterPlayer->GetActorLocation(),
		MantleSweepEnd, CharacterPlayer->GetActorQuat(), WALL_TRACE_CHANNEL, PlayerCapsule, MantleParams);

	if (bMantleWallHit && MantleWallHit.bBlockingHit && MantleWallHit.ImpactNormal.Z < MinWalkableFloorZ && MantleWallHit.ImpactPoint.Z > -0.3f)
	{
		if (Input.MovementInput2D.Y > 0.f && !Input.bCrouchHeld)
		{
			// TODO: Make Mantle Available Height as a variable 50.f
			FVector FloorHitStart = FVector(MantleWallHit.ImpactPoint.X, MantleWallHit.ImpactPoint.Y, CharacterPlayer->GetActorLocation().Z + 200.f);
			FVector FloorHitEnd = FloorHitStart - FVector(0, 0, PlayerCapsule.GetCapsuleHalfHeight() + 210.f);
			bool bMantleFloorHit = GetWorld()->SweepSingleByChannel(MantleFloorHit, FloorHitStart, FloorHitEnd,
				CharacterPlayer->GetActorQuat(), ECC_WorldStatic, PlayerCapsule, MantleParams);

			// DrawDebugCapsuleTraceSingle(GetWorld(), FloorHitStart, FloorHitEnd, PlayerCapsule.GetCapsuleRadius(),
			// PlayerCapsule.GetCapsuleHalfHeight(), EDrawDebugTrace::ForDuration, bMantleFloorHit && MantleFloorHit.IsValidBlockingHit(),
			// MantleFloorHit, FLinearColor::Red, FLinearColor::Green, 1.f);

			if (bMantleFloorHit && MantleFloorHit.IsValidBlockingHit() && MantleFloorHit.ImpactNormal.Z >= MinWalkableFloorZ)
			{
				SetIsDashing(false);
				ElapsedTimeFromDash = 0.f;
				SetMovementState(EMovementState::EMS_Mantle);
				return;
			}
		}
	}

	if (bIsDashing) //TODO: 수정
	{
		if (ElapsedTimeFromDash < DashDecelerationTime)
		{
			ElapsedTimeFromDash += DeltaTime;
			float T = ElapsedTimeFromDash / DashDecelerationTime;
			FVector HorizontalVelocity = Velocity.GetSafeNormal2D() * FMath::Lerp(DashStartSpeed, DashEndSpeed, T * T);
			Velocity = FVector(HorizontalVelocity.X, HorizontalVelocity.Y, Velocity.Z);
		}
		else
		{
			SetIsDashing(false);
			ElapsedTimeFromDash = 0.f;
			FVector HorizontalVelocity = Velocity.GetSafeNormal2D() * DashEndSpeed;
			Velocity = FVector(HorizontalVelocity.X, HorizontalVelocity.Y, Velocity.Z);
		}
	}
	else
	{
		float MaxHorizontalSpeed = bIsRunning ? RunSpeed : WalkSpeed;
		if (bWallJumpAirBoost)
		{
			MaxHorizontalSpeed = WallRunJumpAirSpeed2D;
		}
		else
		{
			if (bShouldKeepSlideSpeed)
			{
				MaxHorizontalSpeed = FMath::Max(DashEndSpeed, LastSlideSpeedBeforeAirborne.Size2D());
			}
			else if (!bShouldKeepSlideSpeed)
			{
				MaxHorizontalSpeed = DashEndSpeed;
			}
			else if (bShouldKeepSlideSpeed)
			{
				MaxHorizontalSpeed = LastSlideSpeedBeforeAirborne.Size2D();
			}
		}

		if (bAirborneFromGravityLauncher)
		{
			MaxHorizontalSpeed = FMath::Max(MaxHorizontalSpeed, JumpPadInitialVelocityXY.Size2D());
		}

		if (Velocity.Size2D() > MaxHorizontalSpeed)
		{
			if (!Input.MovementInput2D.IsNearlyZero())
			{
				FVector CurrentDir2D = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
				FVector TargetDir2D = Input.WorldInputDir;

				// Interpolate current direction towards target input direction
				FVector NewDirection = FMath::VInterpTo(CurrentDir2D, TargetDir2D, DeltaTime, AirDirectionInterpSpeed);

				Velocity.X = NewDirection.X * MaxHorizontalSpeed;
				Velocity.Y = NewDirection.Y * MaxHorizontalSpeed;
			}
			else //TODO: 여기에서 공중에서 이동 입력 불가능 하도록 수정하기
			{
				Velocity.X = Velocity.GetSafeNormal2D().X * MaxHorizontalSpeed;
				Velocity.Y = Velocity.GetSafeNormal2D().Y * MaxHorizontalSpeed;
			}
		}
		else
		{
			FVector VelocityXY = FVector(Velocity.X, Velocity.Y, 0.f);
			FVector NewVelocityXY = VelocityXY + Input.WorldInputDir * AirAcceleration * DeltaTime;

			if (!Input.WorldInputDir.IsNearlyZero())
			{
				if (JumpBoostWindowRemaining > 0.f)
				{
					NewVelocityXY += Input.WorldInputDir * JumpHorizontalBoost * DeltaTime;
					JumpBoostWindowRemaining = 0.f;
				}
			}

			if (NewVelocityXY.Size() > MaxHorizontalSpeed)
			{
				NewVelocityXY = NewVelocityXY.GetSafeNormal() * MaxHorizontalSpeed;
			}

			Velocity.X = NewVelocityXY.X;
			Velocity.Y = NewVelocityXY.Y;
		}
	}

	Velocity.Z = FMath::Max(Velocity.Z - GravityScale * DeltaTime, -MaxFallVerticalSpeed);

	if (bCoyoteTimeActivated)
	{
		if (GetWorld()->GetTimeSeconds() - AirborneStartTime > CoyoteTime)
		{
			bCoyoteTimeActivated = false;
			CurrentJumpCount++; // Prevent performing two jumps in air when player fell off
		}
	}

	if (bGravityLaunchForceRequested)
	{
		bGravityLaunchForceRequested = false;

		Velocity.X = GravityLaunchForceDir.X * GravityLaunchForceAmount;
		Velocity.Y = GravityLaunchForceDir.Y * GravityLaunchForceAmount;
		Velocity.Z = GravityLaunchForceDir.Z * GravityLaunchForceAmount;

		JumpPadInitialVelocityXY = FVector(Velocity.X, Velocity.Y, 0.f);
		SetIsDashing(false);
	}

	if (bJumpPadForceRequested)
	{
		bJumpPadForceRequested = false;
		Velocity.Z = JumpPadForceAmount;
		SetIsDashing(false);
	}

	if (bCanExtendJump)
	{
		if (Input.bJumpPressed)
		{
			CurrJumpHoldTime += DeltaTime;

			if (CurrJumpHoldTime < MaxJumpHoldTime)
			{
				Velocity.Z += ExtraJumpZVel * DeltaTime;
			}
			else
			{
				bCanExtendJump = false;
			}
		}
		else
		{
			bCanExtendJump = false;
		}
	}
	else
	{
		if (Input.bJumpPressed && CurrentJumpCount < MaxJumpCount)
		{
			CurrentJumpCount++;

			if (bCoyoteTimeActivated)
			{
				bCoyoteTimeActivated = false;
				Velocity.Z = PrimaryJumpZVelocity;

				OnPrimaryJumpDelegate.Broadcast();
			}
		}
	}

	JumpBoostWindowRemaining -= DeltaTime;
}

void UPlayerMovementComponent::TickWallRun(float DeltaTime)
{
	FVector WallRunDir;
	if (CurrentWallRunSide == EWallRunSide::EWRS_Left)
	{
		WallRunDir = FVector::CrossProduct(CurrentWallHit.ImpactNormal, FVector::UpVector).GetSafeNormal();
	}
	else if (CurrentWallRunSide == EWallRunSide::EWRS_Right)
	{
		WallRunDir = FVector::CrossProduct(CurrentWallHit.ImpactNormal, FVector::DownVector).GetSafeNormal();
	}

	bControllerTilting = true;

	if (WallRunElapsedTime < (WallRunMaxDuration - 1.f))
	{
		AddControllerRoll(DeltaTime, WallRunDir, CurrentWallRunSide);
	}
	else
	{
		if (!bTiltRecovering)
		{
			bTiltRecovering = true;
			RecoverStartRoll = FMath::UnwindDegrees(CharacterPlayer->GetControlRotation().Roll);
		}

		float RecoverElapsedTime = WallRunElapsedTime - (WallRunMaxDuration - 1.f);
		float Alpha = FMath::Clamp(RecoverElapsedTime / 1.f, 0.f, 1.f);
		float NewRoll = 0.f;

		if (WallRunTiltRecoverCurve)
		{
			float CurveMultiplier = WallRunTiltRecoverCurve->GetFloatValue(Alpha);
			NewRoll = CurveMultiplier * RecoverStartRoll;
		}

		FRotator CurrentControlRotation = CharacterPlayer->GetControlRotation();
		CurrentControlRotation.Roll = NewRoll;
		PlayerController->SetControlRotation(CurrentControlRotation);
	}

	if (WallRunEnterMode == EWallRunEnter::EWRE_Upward && bIsDeceleratingZ)
	{
		Velocity.Z = FMath::Max(Velocity.Z - GravityScale * DeltaTime, 0.f);
		if (Velocity.Z == 0.f)
		{
			bIsDeceleratingZ = false;
		}
	}
	else if (WallRunEnterMode == EWallRunEnter::EWRE_Downward && bIsDeceleratingZ)
	{
		Velocity.Z = FMath::Min(Velocity.Z + GravityScale * DeltaTime, 0.f);
		if (Velocity.Z == 0.f)
		{
			bIsDeceleratingZ = false;
		}
	}

	if (WallRunElapsedTime < WallRunMaxDuration)
	{
		WallRunElapsedTime += DeltaTime;
	}
	else
	{
		WallRunElapsedTime = 0.f;
		Velocity += CurrentWallHit.ImpactNormal * 100.f;
		CurrentJumpCount++;
		SetMovementState(EMovementState::EMS_Airborne);

		return;
	}


	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(CharacterPlayer);
	FVector TraceStart = CharacterPlayer->GetActorLocation();
	FVector TraceEnd = TraceStart - CurrentWallHit.ImpactNormal * 200.f;

	bool bHit = GetWorld()->SweepSingleByChannel(HitResult, TraceStart, TraceEnd, PawnOwner->GetActorQuat(),
		WALL_TRACE_CHANNEL, FCollisionShape::MakeSphere(30.f), Params);

	if (!bHit || !HitResult.IsValidBlockingHit())
	{
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	// Wall was hit - continue with wall run logic
	CurrentWallHit = HitResult;

	FHitResult ForwardHitResult;
	FCollisionQueryParams ForwardParams;
	ForwardParams.AddIgnoredActor(CharacterPlayer);
	FVector ForwardTraceStart = CharacterPlayer->GetActorLocation();
	FVector ForwardTraceEnd = ForwardTraceStart + Velocity.GetSafeNormal() * 75.f;
	bool bForwardHit = GetWorld()->SweepSingleByChannel(ForwardHitResult, ForwardTraceStart, ForwardTraceEnd,
		PawnOwner->GetActorQuat(), WALL_TRACE_CHANNEL, FCollisionShape::MakeSphere(20.f), ForwardParams);

	if (bForwardHit && ForwardHitResult.bBlockingHit)
	{
		CurrentWallHit = ForwardHitResult;
	}

	FVector VelocityDir;

	// Determine velocity direction based on which side we're running on
	if (CurrentWallRunSide == EWallRunSide::EWRS_Left)
	{
		VelocityDir = FVector::CrossProduct(CurrentWallHit.ImpactNormal, FVector::UpVector).GetSafeNormal();

		// Check if player wants to exit wall to the right
		if (Input.MovementInput2D.X > 0.f)
		{
			Velocity += CurrentWallHit.ImpactNormal * 100.f;
			CurrentJumpCount++;
			SetMovementState(EMovementState::EMS_Airborne);
			return;
		}

		float WallRunViewAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(VelocityDir, CharacterPlayer->GetActorForwardVector())));
		bool bIsRight = FVector::CrossProduct(VelocityDir, CharacterPlayer->GetActorForwardVector()).GetSafeNormal().Z > 0;

		if (WallRunViewAngle > 60.f && bIsRight)
		{
			Velocity += CurrentWallHit.ImpactNormal * 100.f;
			CurrentJumpCount++;
			SetMovementState(EMovementState::EMS_Airborne);
			return;
		}

		if (FMath::FindDeltaAngleDegrees(VelocityDir.Rotation().Yaw, CharacterPlayer->GetControlRotation().Yaw) < 0)
		{
			CharacterPlayer->AddControllerYawInput(20.f * DeltaTime);
		}
	}
	else // EWRS_Right
	{
		VelocityDir = FVector::CrossProduct(CurrentWallHit.ImpactNormal, FVector::DownVector).GetSafeNormal();

		// Check if player wants to exit wall to the left
		if (Input.MovementInput2D.X < 0.f)
		{
			Velocity += CurrentWallHit.ImpactNormal * 100.f;
			SetMovementState(EMovementState::EMS_Airborne);
			return;
		}

		float WallRunViewAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(VelocityDir, CharacterPlayer->GetActorForwardVector())));
		bool bIsLeft = FVector::CrossProduct(VelocityDir, CharacterPlayer->GetActorForwardVector()).GetSafeNormal().Z < 0;

		if (WallRunViewAngle > 60.f && bIsLeft)
		{
			Velocity += CurrentWallHit.ImpactNormal * 100.f;
			CurrentJumpCount++;
			SetMovementState(EMovementState::EMS_Airborne);
			return;
		}

		if (FMath::FindDeltaAngleDegrees(VelocityDir.Rotation().Yaw, CharacterPlayer->GetControlRotation().Yaw) > 0)
		{
			CharacterPlayer->AddControllerYawInput(-20.f * DeltaTime);
		}
	}

	// Input W key (Forward)
	if (Input.MovementInput2D.Y > 0.f)
	{
		// Always accelerate in the forward wall run direction when pressing W
		float CurrentSpeed = Velocity.Size2D();
		float NewSpeed;
		if (CurrentSpeed > WallRunMaxSpeed)
		{
			NewSpeed = FMath::Max(CurrentSpeed - WallRunDeceleration * DeltaTime, WallRunMaxSpeed);
		}
		else
		{
			NewSpeed = FMath::Min(CurrentSpeed + WallRunAcceleration * DeltaTime, WallRunMaxSpeed);
		}

		// Always set velocity in forward wall run direction when pressing W
		Velocity.X = NewSpeed * VelocityDir.X;
		Velocity.Y = NewSpeed * VelocityDir.Y;

	}
	// Input S Key (Backward)
	else if (Input.MovementInput2D.Y < 0.f)
	{
		// // Always accelerate in the backward wall run direction when pressing S
		// float CurrentSpeed = Velocity.Size2D();
		// float NewSpeed;
		// if (CurrentSpeed > WallRunBackwardMaxSpeed)
		// {
		// 	NewSpeed = FMath::Max(CurrentSpeed - WallRunDeceleration * DeltaTime, WallRunBackwardMaxSpeed);
		// }
		// else
		// {
		// 	NewSpeed = FMath::Min(CurrentSpeed + WallRunAcceleration * DeltaTime, WallRunBackwardMaxSpeed);
		// }

		// // Always set velocity in backward wall run direction when pressing S
		// Velocity.X = NewSpeed * -VelocityDir.X;
		// Velocity.Y = NewSpeed * -VelocityDir.Y;

		// Calculate if we're already moving backward along the wall
		bool bIsMovingBackward = (FVector2D::DotProduct(FVector2D(Velocity), FVector2D(VelocityDir)) < 0);
		float CurrentSpeed = Velocity.Size2D();
		float NewSpeed;

		// If already moving backward and above max speed
		if (bIsMovingBackward && CurrentSpeed > WallRunBackwardMaxSpeed)
		{
			// Allow a more gradual deceleration when entering with high backward speed
			// Apply a smaller deceleration rate initially to preserve momentum
			float InitialDecelRate = WallRunDeceleration * 0.5f;
			NewSpeed = FMath::Max(CurrentSpeed - InitialDecelRate * DeltaTime, WallRunBackwardMaxSpeed);
		}
		// Standard backward acceleration/deceleration
		else if (bIsMovingBackward)
		{
			NewSpeed = FMath::Min(CurrentSpeed + WallRunAcceleration * DeltaTime, WallRunBackwardMaxSpeed);
		}
		// Moving forward but pressing S, decelerate and then reverse
		else
		{
			// First decelerate to zero
			NewSpeed = FMath::Max(CurrentSpeed - WallRunDeceleration * 1.5f * DeltaTime, 0.f);

			// If completely decelerated, start moving backward
			if (NewSpeed <= 0.1f)
			{
				NewSpeed = FMath::Min(WallRunAcceleration * 0.5f * DeltaTime, WallRunBackwardMaxSpeed);
				bIsMovingBackward = true;
			}
		}

		// Apply velocity in the correct direction
		if (bIsMovingBackward)
		{
			Velocity.X = NewSpeed * -VelocityDir.X;
			Velocity.Y = NewSpeed * -VelocityDir.Y;
		}
		else
		{
			Velocity.X = NewSpeed * VelocityDir.X;
			Velocity.Y = NewSpeed * VelocityDir.Y;
		}
	}
	else
	{
		float NewSpeed = FMath::Max(Velocity.Size2D() - WallRunDeceleration * DeltaTime, 0.f);
		Velocity.X = VelocityDir.X * NewSpeed;
		Velocity.Y = VelocityDir.Y * NewSpeed;
		if (!bIsDeceleratingZ)
		{
			Velocity.Z = FMath::Max(Velocity.Z - 0.2f * GravityScale * DeltaTime, -300.f);
		}
	}

	if (!bIsDeceleratingZ && Input.MovementInput2D.Y != 0.f)
	{
		Velocity.Z = 0.f;
	}

	if (bGravityLaunchForceRequested)
	{
		bGravityLaunchForceRequested = false;

		Velocity.X = GravityLaunchForceDir.X * GravityLaunchForceAmount;
		Velocity.Y = GravityLaunchForceDir.Y * GravityLaunchForceAmount;
		Velocity.Z = GravityLaunchForceDir.Z * GravityLaunchForceAmount;
		CurrentJumpCount++;
		JumpPadInitialVelocityXY = FVector(Velocity.X, Velocity.Y, 0.f);
		SetIsDashing(false);
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (bJumpPadForceRequested)
	{
		bJumpPadForceRequested = false;
		Velocity.Z = JumpPadForceAmount;
		CurrentJumpCount++;
		SetIsDashing(false);
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (Input.bJumpPressed && CurrentJumpCount < MaxJumpCount)
	{
		CurrentJumpCount++;
		FVector WallNormal2D = CurrentWallHit.ImpactNormal.GetSafeNormal2D();
		Velocity = FVector(Velocity.X, Velocity.Y, 0.f) + WallNormal2D * WallRunJumpNormalForce + FVector::UpVector * WallJumpZVelocity;
		OnWallJumpDelegate.Broadcast();
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (Input.bCrouchHeld)
	{
		Velocity += CurrentWallHit.ImpactNormal * 100.f;
		SetMovementState(EMovementState::EMS_Airborne);
		return;
	}

	if (IsGrounded())
	{
		SetMovementState(EMovementState::EMS_Move);
		return;
	}
}

void UPlayerMovementComponent::TickMantle(float DeltaTime)
{
	FVector MantleFloorSlope = FVector::VectorPlaneProject(CharacterPlayer->GetActorForwardVector(),
		MantleFloorHit.ImpactNormal).GetSafeNormal();

	FHitResult Hit;
	FCollisionShape PlayerCapsule = CharacterPlayer->GetCapsuleComponent()->GetCollisionShape();
	FCollisionQueryParams MantleParams;
	MantleParams.AddIgnoredActor(CharacterPlayer);
	FVector MantleSweepEnd = CharacterPlayer->GetActorLocation() + MantleFloorSlope * PlayerCapsule.GetCapsuleRadius();
	bool bMantleWallHit = GetWorld()->SweepSingleByChannel(Hit, CharacterPlayer->GetActorLocation(),
		MantleSweepEnd, CharacterPlayer->GetActorQuat(), WALL_TRACE_CHANNEL, PlayerCapsule, MantleParams);

	if (!bMantleWallHit)
	{
		Velocity = MantleFloorSlope * (bIsRunning ? RunSpeed : WalkSpeed);
		OnMoveDelegate.Broadcast();
		SetMovementState(EMovementState::EMS_Move);
		return;
	}

	Velocity = FVector::VectorPlaneProject(FVector::UpVector, Hit.ImpactNormal).GetSafeNormal() * 800.f;
}

void UPlayerMovementComponent::TickDowned(float DeltaTime)
{
	const float ElapsedTime = GetWorld()->GetTimeSeconds() - DownedStartTime;

	if (ElapsedTime >= DownedDuration)
	{
		SetMovementState(EMovementState::EMS_Move);
		return;
	}

	FVector DefaultCameraRelativeLocation = CharacterPlayer->GetDefaultCameraRelativeLocation();
	FVector TargetCameraLocation = DefaultCameraRelativeLocation + DownedPositionCurve->GetVectorValue(ElapsedTime);
	CharacterPlayer->GetCameraComponent()->SetRelativeLocation(TargetCameraLocation);

	FVector DownedRotationVector = DownedRotationCurve->GetVectorValue(ElapsedTime);
	FRotator TargetControlRotation = DownedStartControlRotation + FRotator(DownedRotationVector.Y, DownedRotationVector.Z, DownedRotationVector.X);
	GetController()->SetControlRotation(TargetControlRotation);

	if (!IsGrounded())
	{
		float MaxHorizontalSpeed = bIsRunning ? RunSpeed : WalkSpeed;
		if (bWallJumpAirBoost)
		{
			MaxHorizontalSpeed = WallRunJumpAirSpeed2D;
		}
		else
		{
			if (bShouldKeepSlideSpeed)
			{
				MaxHorizontalSpeed = FMath::Max(DashEndSpeed, LastSlideSpeedBeforeAirborne.Size2D());
			}
			else if (!bShouldKeepSlideSpeed)
			{
				MaxHorizontalSpeed = DashEndSpeed;
			}
			else if (bShouldKeepSlideSpeed)
			{
				MaxHorizontalSpeed = LastSlideSpeedBeforeAirborne.Size2D();
			}

		}

		if (Velocity.Size2D() > MaxHorizontalSpeed)
		{
			Velocity.X = Velocity.GetSafeNormal2D().X * MaxHorizontalSpeed;
			Velocity.Y = Velocity.GetSafeNormal2D().Y * MaxHorizontalSpeed;
		}

		Velocity.Z = FMath::Max(Velocity.Z - GravityScale * DeltaTime, -MaxFallVerticalSpeed);
	}
	else
	{
		Velocity = FMath::VInterpTo(Velocity, FVector::ZeroVector, DeltaTime, 3.f);
	}
}

void UPlayerMovementComponent::TickDead(float DeltaTime)
{
	// 1. Play death animation
	float ElapsedTime = GetWorld()->GetTimeSeconds() - DeathStartTime;
	if (ElapsedTime >= DeathCameraAnimDuration + 0.7f)
	{
		//ADefaultGameMode* LevelGameMode = Cast<ADefaultGameMode>(GetWorld()->GetAuthGameMode());
		//if (ensure(LevelGameMode))
		//{
		//	LevelGameMode->RespawnToLastCheckpoint(CharacterPlayer);
		//}
		CharacterPlayer->GetCameraComponent()->SetRelativeLocation(CharacterPlayer->GetDefaultCameraRelativeLocation());
		PlayerController->SetControlRotation(CharacterPlayer->GetActorRotation());
		SetMovementState(EMovementState::EMS_Move);
		return;
	}

	FVector DefaultCameraRelativeLocation = CharacterPlayer->GetDefaultCameraRelativeLocation();
	FVector TargetCameraLocation = DefaultCameraRelativeLocation + DeathCameraLocationCurve->GetVectorValue(ElapsedTime);
	CharacterPlayer->GetCameraComponent()->SetRelativeLocation(TargetCameraLocation);

	FVector DeathRotationVector = DeathCameraRotationCurve->GetVectorValue(ElapsedTime);
	FRotator TargetControlRotation = DeathStartControlRotation + FRotator(DeathRotationVector.Y, DeathRotationVector.Z, DeathRotationVector.X);
	if (FRotator::NormalizeAxis(TargetControlRotation.Pitch) < -90.f)
	{
		TargetControlRotation.Pitch = -90.f;
	}
	else if (FRotator::NormalizeAxis(TargetControlRotation.Pitch) > 90.f)
	{
		TargetControlRotation.Pitch = 90.f;
	}
	GetController()->SetControlRotation(TargetControlRotation);

	if (!IsGrounded())
	{
		float MaxHorizontalSpeed = bIsRunning ? RunSpeed : WalkSpeed;
		if (bWallJumpAirBoost)
		{
			MaxHorizontalSpeed = WallRunJumpAirSpeed2D;
		}
		else
		{
			if (bShouldKeepSlideSpeed)
			{
				MaxHorizontalSpeed = FMath::Max(DashEndSpeed, LastSlideSpeedBeforeAirborne.Size2D());
			}
			else if (!bShouldKeepSlideSpeed)
			{
				MaxHorizontalSpeed = DashEndSpeed;
			}
			else if (bShouldKeepSlideSpeed)
			{
				MaxHorizontalSpeed = LastSlideSpeedBeforeAirborne.Size2D();
			}

		}


		if (Velocity.Size2D() > MaxHorizontalSpeed)
		{
			Velocity.X = Velocity.GetSafeNormal2D().X * MaxHorizontalSpeed;
			Velocity.Y = Velocity.GetSafeNormal2D().Y * MaxHorizontalSpeed;
		}

		Velocity.Z = FMath::Max(Velocity.Z - GravityScale * DeltaTime, -MaxFallVerticalSpeed);
	}
	else
	{
		Velocity = FMath::VInterpTo(Velocity, FVector::ZeroVector, DeltaTime, 3.f);
	}
}

bool UPlayerMovementComponent::CanWallRun()
{
	if (Input.bCrouchHeld) return false;

	FCollisionQueryParams WallQueryParams;
	WallQueryParams.AddIgnoredActor(CharacterPlayer);

	FVector WallTraceStart = PawnOwner->GetActorLocation();
	FVector WallTraceRightEnd = WallTraceStart + PawnOwner->GetActorRightVector() * 70.f;
	FVector WallTraceLeftEnd = WallTraceStart - PawnOwner->GetActorRightVector() * 70.f;


	FHitResult WallRightHit;
	bool bWallRightHit = GetWorld()->SweepSingleByChannel(WallRightHit, WallTraceStart, WallTraceRightEnd, FQuat::Identity,
		WALL_TRACE_CHANNEL, FCollisionShape::MakeSphere(10.f), WallQueryParams);

	bool bRightWallRunnable = false;

	if (bWallRightHit && WallRightHit.bBlockingHit && WallRightHit.ImpactNormal.Z < MinWalkableFloorZ &&
		FVector::DotProduct(Velocity.GetSafeNormal2D(), WallRightHit.ImpactNormal.GetSafeNormal2D()) < 0.f)
	{

		bRightWallRunnable = CheckWallCooldown({ WallRightHit, GetWorld()->GetTimeSeconds() });
	}

	FHitResult WallLeftHit;
	bool bWallLeftHit = GetWorld()->SweepSingleByChannel(WallLeftHit, WallTraceStart, WallTraceLeftEnd, FQuat::Identity,
		WALL_TRACE_CHANNEL, FCollisionShape::MakeSphere(10.f), WallQueryParams);

	bool bLeftWallRunnable = false;

	if (bWallLeftHit&& WallLeftHit.bBlockingHit&& WallLeftHit.ImpactNormal.Z < MinWalkableFloorZ&&
		FVector::DotProduct(Velocity.GetSafeNormal2D(), WallLeftHit.ImpactNormal.GetSafeNormal2D()) < 0.f)
	{
		bLeftWallRunnable = CheckWallCooldown({ WallLeftHit, GetWorld()->GetTimeSeconds() });
	}

	if (bRightWallRunnable && bLeftWallRunnable)
	{
		WallRunEnterSpeed2D = Velocity.Size2D();
		CurrentWallRunSide = WallLeftHit.Distance > WallRightHit.Distance ? EWallRunSide::EWRS_Right : EWallRunSide::EWRS_Left;
		CurrentWallHit = WallLeftHit.Distance > WallRightHit.Distance ? WallRightHit : WallLeftHit;
		return true;
	}
	else if (bLeftWallRunnable)
	{
		WallRunEnterSpeed2D = Velocity.Size2D();
		CurrentWallRunSide = EWallRunSide::EWRS_Left;
		CurrentWallHit = WallLeftHit;
		return true;
	}
	else if (bRightWallRunnable)
	{
		WallRunEnterSpeed2D = Velocity.Size2D();
		CurrentWallRunSide = EWallRunSide::EWRS_Right;
		CurrentWallHit = WallRightHit;
		return true;
	}
	else
	{
		CurrentWallRunSide = EWallRunSide::EWRS_None;
	}
	return false;
}

bool UPlayerMovementComponent::CheckWallCooldown(const FWallInfo& InWallInfo)
{
	if (CooldownWalls.IsEmpty()) return true;

	float CurrentTime = GetWorld()->GetTimeSeconds();

	for (const FWallInfo& CooldownWall : CooldownWalls)
	{
		if (CurrentTime - CooldownWall.TimeStamp > WallCooldown) continue;

		float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(InWallInfo.Hit.ImpactNormal, CooldownWall.Hit.ImpactNormal)));
		float XYDistance = FVector::Dist2D(InWallInfo.Hit.ImpactPoint, CooldownWall.Hit.ImpactPoint);

		if (Angle > 15.f)
		{
			return true;
		}
		else if (XYDistance >= 2000.f)
		{
			return true;
		}
	}

	return false;
}

void UPlayerMovementComponent::UpdateWallCooldowns()
{
	CooldownWalls.RemoveAllSwap([this](const FWallInfo& CooldownWall)
		{
			return GetWorld()->GetTimeSeconds() - CooldownWall.TimeStamp > WallCooldown;
		});
}

void UPlayerMovementComponent::OnMovementStateChanged(EMovementState OldState, EMovementState NewState)
{
	switch (OldState)
	{
	case EMovementState::EMS_Move:
	{
		SlideResetTimer = 0.f;
		break;
	}

	case EMovementState::EMS_Airborne:
	{
		bWallJumpAirBoost = false;
		bShouldKeepSlideSpeed = false;
		bAirborneFromGravityLauncher = false;
		ElapsedTimeFromSurface = 0.f;
		CurrentJumpCount = 0;

		CurrJumpHoldTime = 0;
		bCanExtendJump = true;

		JumpBoostWindowRemaining = JumpBoostInputWindow;

		bCoyoteTimeActivated = false;
		break;
	}
	case EMovementState::EMS_WallRun:
	{
		OnWallRunEndDelegate.Broadcast();
		WallRunElapsedTime = 0.f;
		bIsDeceleratingZ = false;
		bTiltRecovering = false;

		CooldownWalls.Add({ CurrentWallHit, GetWorld()->GetTimeSeconds() });

		if (NewState == EMovementState::EMS_Airborne)
		{
			bWallJumpAirBoost = true;
		}
		else
		{
			bControllerTilting = false;
		}
		break;
	}

	case EMovementState::EMS_Mantle:
		break;
	case EMovementState::EMS_Slide:
	{
		OnSlideEndDelegate.Broadcast();
		// This is for slide direction.
		SlideStateElapsedTime = 0.f;
	}
	break;
	case EMovementState::EMS_Downed:
		break;
	case EMovementState::EMS_Dead:
		break;
	default:
		break;
	}

	switch (NewState)
	{
	case EMovementState::EMS_Move:
	{
		OnMoveDelegate.Broadcast();
		break;
	}
	case EMovementState::EMS_Airborne:
	{
		OnAirborneDelegate.Broadcast();
		break;
	}
	case EMovementState::EMS_WallRun:
	{
		OnWallRunDelegate.Broadcast();
		bHasRecentlySlid = false;
		SlideElapsedTime = 0.f;

		Velocity.X = Velocity.GetSafeNormal2D().X * WallRunEnterSpeed2D;
		Velocity.Y = Velocity.GetSafeNormal2D().Y * WallRunEnterSpeed2D;

		if (WallRunEnterMode == EWallRunEnter::EWRE_Upward)
		{
			bIsDeceleratingZ = true;
			Velocity.Z = 700.f;
		}
		else if (WallRunEnterMode == EWallRunEnter::EWRE_Downward)
		{
			bIsDeceleratingZ = true;
			Velocity.Z = -700.f;
		}
		else if (WallRunEnterMode == EWallRunEnter::EWRE_Neutral)
		{
			bIsDeceleratingZ = false;
			Velocity.Z = 0.f;
		}
		break;
	}
	case EMovementState::EMS_Mantle:
	{
		OnMantleDelegate.Broadcast();
		bHasRecentlySlid = false;
		SlideElapsedTime = 0.f;
		break;
	}
	case EMovementState::EMS_Slide:
	{
		OnSlideDelegate.Broadcast();
		bHasRecentlySlid = true;
		SlideDecelerationAmount = (Velocity.Size() - CrouchSpeed) / FMath::Max(
			SlideMaxDuration - SlideElapsedTime, 0.1f);
		break;
	}
	case EMovementState::EMS_Downed:
	{
		OnDownedDelegate.Broadcast();
		DownedStartControlRotation = CharacterPlayer->GetControlRotation();
		DownedStartTime = GetWorld()->GetTimeSeconds();
		// if (OldState == EMovementState::EMS_Move)
		// {
		// 	Velocity = FVector::ZeroVector;
		// }

		Velocity += ReceivedDamageDirection * ReceivedDamageForce;
		break;
	}
	case EMovementState::EMS_Dead:
	{
		Velocity = FVector::ZeroVector;
		DeathStartControlRotation = CharacterPlayer->GetControlRotation();
		DeathStartTime = GetWorld()->GetTimeSeconds();
		break;
	}
	default:
		break;
	}

	if (OldState == EMovementState::EMS_Airborne && (NewState == EMovementState::EMS_Move || NewState == EMovementState::EMS_Slide))
	{
		OnLandDelegate.Broadcast(LastVelocityBeforeLand.Z);
	}

	if (OldState == EMovementState::EMS_Dead && NewState == EMovementState::EMS_Move)
	{
		PlayerController->PlayerCameraManager->StopCameraFade();
		PlayerController->PlayerCameraManager->StartCameraFade(1.f, 0.f, 1.f, FLinearColor::Black, true);
	}
}

bool UPlayerMovementComponent::IsGrounded()
{
	FCollisionQueryParams GroundSweepParams;
	GroundSweepParams.AddIgnoredActor(PawnOwner);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel3, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_GameTraceChannel4, ECR_Ignore);

	FVector SweepStart = CharacterPlayer->GetActorLocation();
	FVector SweepEnd = CharacterPlayer->GetActorLocation() +
		FVector::DownVector * (CharacterPlayer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	float CapsuleRadius = CharacterPlayer->GetCapsuleComponent()->GetScaledCapsuleRadius();

	bool bHit = GetWorld()->SweepSingleByChannel(GroundHit, SweepStart, SweepEnd, FQuat::Identity, ECC_WorldStatic,
		FCollisionShape::MakeSphere(CapsuleRadius - 5.f), GroundSweepParams, ResponseParams);
	//MEMO: 왜 CapsuleRadius - 5 를 하는가?

	if (!bHit || !GroundHit.bBlockingHit)
	{
		return false;
	}

	if (GroundHit.ImpactNormal.Z < MinWalkableFloorZ)
	{
		return false;
	}

	LastVelocityBeforeLand = Velocity;

	return true;
}

void UPlayerMovementComponent::NotifyDamageData(EDamageType DamageType, const FVector& DamageDirection, float DamageForce)
{
	ReceivedDamageType = DamageType;
	LastDamagedWorldTime = GetWorld()->GetTimeSeconds();
	ReceivedDamageDirection = DamageDirection;
	ReceivedDamageForce = DamageForce;

	switch (DamageType)
	{
	case EDamageType::Charge:
		if (CurrentMovementState == EMovementState::EMS_Move ||
			CurrentMovementState == EMovementState::EMS_Airborne)
		{
			bIsInvincible = true;
			bDownedDamage = true;
		}
		break;
	default:
		bDamageSlowDebuff = true;
		break;
	}
}

void UPlayerMovementComponent::NotifyGravityLaunchForce(const FVector& Direction, float ForceAmount)
{
	if (CurrentMovementState == EMovementState::EMS_Airborne ||
		CurrentMovementState == EMovementState::EMS_Move ||
		CurrentMovementState == EMovementState::EMS_Slide ||
		CurrentMovementState == EMovementState::EMS_WallRun)
	{
		bGravityLaunchForceRequested = true;
		bAirborneFromGravityLauncher = true;
		GravityLaunchForceDir = Direction;
		GravityLaunchForceAmount = ForceAmount;
	}
}

void UPlayerMovementComponent::NotifyJumpPadLaunchForce(float ForceAmount)
{
	if (CurrentMovementState == EMovementState::EMS_Airborne ||
		CurrentMovementState == EMovementState::EMS_Move ||
		CurrentMovementState == EMovementState::EMS_Slide ||
		CurrentMovementState == EMovementState::EMS_WallRun)
	{
		bJumpPadForceRequested = true;
		JumpPadForceAmount = ForceAmount;
	}
}

//void UPlayerMovementComponent::NotifyMovementDataModification(const TArray<FPlayerMovementDataModifier>& Modifiers)
//{
//	bMovementDataModificationRequested = true;
//	MovementDataModifiers = Modifiers;
//}
//
//void UPlayerMovementComponent::NotifyMovementKeyHoldModification(const TArray<FPlayerKeyHoldModifier>& Modifiers)
//{
//	bMovementKeyHoldModificationRequested = true;
//	KeyHoldModifiers = Modifiers;
//}

void UPlayerMovementComponent::NotifyResetModification()
{
	bMovementModificationResetRequested = true;
}

void UPlayerMovementComponent::NotifyDeath()
{
	bDeadRequested = true;
}


void UPlayerMovementComponent::SetMovementInputVector(const FVector2D& InMovementInputVector)
{
	MovementInputVector = InMovementInputVector;
}

void UPlayerMovementComponent::SetMovementState(EMovementState NewState)
{
	PreviousMovementState = CurrentMovementState;
	CurrentMovementState = NewState;

	OnMovementStateChanged(PreviousMovementState, NewState);
}

void UPlayerMovementComponent::SetJumpPressed(bool bPressed)
{
	bJumpPressed = bPressed;
}

void UPlayerMovementComponent::SetShiftPressed(bool bPressed)
{
	bShiftPressed = bPressed;
}


void UPlayerMovementComponent::SetCrouchPressed(bool bPressed)
{
	bCrouchPressed = bPressed;
}

bool UPlayerMovementComponent::FindGroundPoint(FVector& OutPoint)
{
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(PawnOwner);
	FVector TraceStart = PawnOwner->GetActorLocation();
	FVector TraceEnd = TraceStart + FVector(0, 0, -GroundPointDetectionLength);
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECollisionChannel::ECC_WorldStatic, Params);

	if (bHit)
	{
		OutPoint = Hit.ImpactPoint;
		// DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.f, 10, FColor::Green, false);
	}

	return bHit;
}

void UPlayerMovementComponent::UpdateDashGauge(float DeltaTime)
{
	if (DashGauge < 2.f)
	{
		/*
		 * Gauge max value is 2. and the dash cooldown is 5 seconds, and dash cooldown is accounted for a single dash, So
		 * the increment factor becomes 1 second divided by dash cooldown.
		 */

		DashGauge = FMath::Clamp(DashGauge + (1.f / DashCooldown) * DeltaTime, 0.f, 2.f);
	}
}

void UPlayerMovementComponent::CacheInput()
{
	Input.WorldInputDir = ConsumeInputVector().GetSafeNormal();
	Input.MovementInput2D = MovementInputVector;
	Input.bJumpPressed = bJumpPressed;
	Input.bShiftPressed = bShiftPressed;
	Input.bCrouchHeld = bCrouchPressed;

	//bJumpPressed = false;
	//bShiftPressed = false;

	if (bMovementKeyHoldActive)
	{
		float InputAxisX = (MovementTriggerKeyMap[EMovementTriggerKey::A] ? -1.f : 0.f) +
			(MovementTriggerKeyMap[EMovementTriggerKey::D] ? 1.f : 0.f);

		float InputAxisY = (MovementTriggerKeyMap[EMovementTriggerKey::S] ? -1.f : 0.f) +
			(MovementTriggerKeyMap[EMovementTriggerKey::W] ? 1.f : 0.f);

		Input.MovementInput2D.X += !FMath::IsNearlyZero(InputAxisX) ? InputAxisX : 0.f;
		Input.MovementInput2D.X = FMath::Clamp(Input.MovementInput2D.X, -1.f, 1.f);

		Input.MovementInput2D.Y += !FMath::IsNearlyZero(InputAxisY) ? InputAxisY : 0.f;
		Input.MovementInput2D.Y = FMath::Clamp(Input.MovementInput2D.Y, -1.f, 1.f);


		const FRotator YawRot(0.f, PlayerController->GetControlRotation().Yaw, 0.f);
		const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

		FVector WorldDir = Forward * Input.MovementInput2D.Y + Right * Input.MovementInput2D.X;
		Input.WorldInputDir = WorldDir.GetSafeNormal();

		Input.bJumpPressed = MovementTriggerKeyMap.FindRef(EMovementTriggerKey::Space) || Input.bJumpPressed;
		Input.bShiftPressed = MovementTriggerKeyMap.FindRef(EMovementTriggerKey::Shift) || Input.bShiftPressed;
		Input.bCrouchHeld = MovementTriggerKeyMap.FindRef(EMovementTriggerKey::Ctrl) || Input.bCrouchHeld;
	}
}

void UPlayerMovementComponent::UpdateDamageFlags()
{
	const float ElapsedTimeFromLastDamage = GetWorld()->GetTimeSeconds() - LastDamagedWorldTime;

	if (bDamageSlowDebuff)
	{
		if (ElapsedTimeFromLastDamage > DamageSlowDebuffDuration)
		{
			bDamageSlowDebuff = false;
		}
	}

	if (bIsInvincible)
	{
		if (ElapsedTimeFromLastDamage > DownedInvincibleDuration)
		{
			bIsInvincible = false;
		}
	}
}

void UPlayerMovementComponent::UpdateDependentMovementData()
{
	MinWalkableFloorZ = FMath::Cos(FMath::DegreesToRadians(MaxWalkableFloorAngle));
	GravityAcceleration = FVector::DownVector * GravityScale;
	PrimaryJumpZVelocity = FMath::Sqrt(2 * GravityScale * PrimaryJumpHeight);
	ExtraJumpZVel = FMath::Sqrt(2 * GravityScale * ExtraJumpHeight);
	//DoubleJumpZVelocity = FMath::Sqrt(2 * GravityScale * DoubleJumpHeight);
	WallJumpZVelocity = FMath::Sqrt(2 * GravityScale * WallJumpHeight);
}

void UPlayerMovementComponent::UpdateStamina(float DeltaTime, float UpdateRate)
{
	CurrStamina += UpdateRate;
	CurrStamina = FMath::Clamp(CurrStamina, 0.f, MaxStamina);
}

void UPlayerMovementComponent::AddControllerRoll(float DeltaTime, const FVector& WallRunDirection, EWallRunSide WallRunSide)
{
	FVector PlayerForward = CharacterPlayer->GetActorForwardVector();

	float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(PlayerForward, WallRunDirection)));
	float DirectionSign = FMath::Sign(FVector::CrossProduct(WallRunDirection, PlayerForward).Z);
	float SignedAngle = Angle * DirectionSign;
	float TargetRoll = 0.f;

	if (WallRunSide == EWallRunSide::EWRS_Left)
	{
		TargetRoll = FMath::GetMappedRangeValueClamped(
			FVector2D(0.f, 90.f),
			FVector2D(WallRunCameraTiltAngle, 0.f),
			FMath::Abs(SignedAngle));

	}
	else if (WallRunSide == EWallRunSide::EWRS_Right)
	{
		TargetRoll = FMath::GetMappedRangeValueClamped(
			FVector2D(0.f, 90.f),
			FVector2D(-WallRunCameraTiltAngle, 0.f),
			FMath::Abs(SignedAngle));
	}

	FRotator CurrentControlRotation = CharacterPlayer->GetControlRotation();
	float CurrentRoll = FMath::UnwindDegrees(CurrentControlRotation.Roll);
	float NewRoll = FMath::FInterpTo(CurrentRoll, TargetRoll, DeltaTime, WallRunCameraTiltInterpSpeed);
	FRotator NewControlRotation = CurrentControlRotation;
	NewControlRotation.Roll = NewRoll;
	PlayerController->SetControlRotation(NewControlRotation);
}

void UPlayerMovementComponent::ApplyMovementDataModifiers()
{
	//if (bMovementDataModificationRequested)
	//{
	//	bMovementDataModificationRequested = false;
	//	if (MovementDataModifiers.IsEmpty()) return;

	//	for (auto Modifier : MovementDataModifiers)
	//	{
	//		*MovementDataTypeMap.FindRef(Modifier.DataType) = Modifier.ModifiedValue;
	//	}

	//	UpdateDependentMovementData();
	//}
}

void UPlayerMovementComponent::ApplyKeyHoldModifiers()
{
	//if (bMovementKeyHoldModificationRequested)
	//{
	//	bMovementKeyHoldModificationRequested = false;
	//	if (KeyHoldModifiers.IsEmpty()) return;

	//	for (auto Modifier : KeyHoldModifiers)
	//	{
	//		MovementTriggerKeyMap[Modifier.Key] = true;
	//	}
	//	bMovementKeyHoldActive = true;
	//}
}

void UPlayerMovementComponent::SetIsDashing(bool bNewIsDashing)
{
	bIsDashing = bNewIsDashing;
	OnIsDashingChanged(bNewIsDashing);
}

void UPlayerMovementComponent::OnIsDashingChanged(bool bNewIsDashing)
{
	if (bNewIsDashing)
	{
		ElapsedTimeFromDash = 0.f;
		OnDashDelegate.Broadcast(MovementInputVector);
	}
	else
	{
		OnDashEndDelegate.Broadcast();
	}
}


void UPlayerMovementComponent::ApplyMovementDataTable()
{
	ensureMsgf(MovementDataTable, TEXT("MovementDataTable is not assigned in PawnPlayer blueprint!!!!!!!!"));

	if (!MovementDataTable) return;
	const FPlayerMovementRow* Row = MovementDataTable->FindRow<FPlayerMovementRow>("Player", "");
	if (!Row) return;
	GravityScale = Row->GravityScale;
	WalkSpeed = Row->WalkSpeed;
	DashStartSpeed = Row->DashStartSpeed;
	DashEndSpeed = Row->DashEndSpeed;
	DashDecelerationTime = Row->DashDecelerationTime;
	DashCooldown = Row->DashCooldown;
	RunSpeed = Row->RunSpeed;
	CrouchSpeed = Row->CrouchSpeed;
	CrouchHeightScale = Row->CrouchHeightScale;
	PrimaryJumpHeight = Row->PrimaryJumpHeight;
	ExtraJumpHeight = Row->ExtraJumpHeight;
	WallJumpHeight = Row->WallJumpHeight;
	JumpHorizontalBoost = Row->JumpHorizontalBoost;
	JumpBoostInputWindow = Row->JumpBoostInputWindow;
	MaxJumpHoldTime = Row->MaxJumpHoldTime;
	Acceleration = Row->Acceleration;
	Deceleration = Row->Deceleration;
	AirDirectionInterpSpeed = Row->AirDirectionInterpSpeed;
	AirAcceleration = Row->AirAcceleration;
	AirDeceleration = Row->AirDeceleration;
	MaxFallVerticalSpeed = Row->MaxFallVerticalSpeed;
	MaxWalkableFloorAngle = Row->MaxWalkableFloorAngle;
	MaxStepHeight = Row->MaxStepHeight;
	WallRunMaxDuration = Row->WallRunMaxDuration;
	WallRunAcceleration = Row->WallRunAcceleration;
	WallRunDeceleration = Row->WallRunDeceleration;
	WallRunMaxSpeed = Row->WallRunMaxSpeed;
	WallRunBackwardMaxSpeed = Row->WallRunBackwardMaxSpeed;
	WallRunJumpAirSpeed2D = Row->WallRunJumpAirSpeed2D;
	WallRunJumpNormalForce = Row->WallRunJumpNormalForce;
	WallRunCameraTiltAngle = Row->WallRunCameraTiltAngle;
	PreWallRunDetectionRange = Row->PreWallRunDetectionRange;
	WallRunCameraTiltInterpSpeed = Row->WallRunCameraTiltInterpSpeed;
	WallRunTiltRecoverCurve = Row->WallRunTiltRecoverCurve;
	SlideInitialWindow = Row->SlideInitialWindow;
	SlideMaxDuration = Row->SlideMaxDuration;
	SlideAdditionalSpeed = Row->SlideAdditionalSpeed;
	GroundPointDetectionLength = Row->GroundPointDetectionLength;
	CoyoteTime = Row->CoyoteTime;
	DamageSlowDebuffMultiplier = Row->DamageSlowDebuffMultiplier;
	DamageSlowDebuffDuration = Row->DamageSlowDebuffDuration;
	DownedDuration = Row->DownedDuration;
	DownedInvincibleDuration = Row->DownedInvincibleDuration;

	UpdateDependentMovementData();
}

void UPlayerMovementComponent::ConsumeDeadRequest()
{
	if (!bDeadRequested) return;

	bDeadRequested = false;
	SetMovementState(EMovementState::EMS_Dead);
}

void UPlayerMovementComponent::InitMovementDataTypeMap()
{
	MovementDataTypeMap.Add(EMovementDataType::GravityScale, &GravityScale);
	MovementDataTypeMap.Add(EMovementDataType::WalkSpeed, &WalkSpeed);
	MovementDataTypeMap.Add(EMovementDataType::DashStartSpeed, &DashStartSpeed);
	MovementDataTypeMap.Add(EMovementDataType::DashEndSpeed, &DashEndSpeed);
	MovementDataTypeMap.Add(EMovementDataType::DashDecelerationTime, &DashDecelerationTime);
	MovementDataTypeMap.Add(EMovementDataType::DashCooldown, &DashCooldown);
	MovementDataTypeMap.Add(EMovementDataType::RunSpeed, &RunSpeed);
	MovementDataTypeMap.Add(EMovementDataType::CrouchSpeed, &CrouchSpeed);
	MovementDataTypeMap.Add(EMovementDataType::CrouchHeightScale, &CrouchHeightScale);
	MovementDataTypeMap.Add(EMovementDataType::PrimaryJumpHeight, &PrimaryJumpHeight);
	MovementDataTypeMap.Add(EMovementDataType::ExtraJumpHeight, &ExtraJumpHeight);
	MovementDataTypeMap.Add(EMovementDataType::WallJumpHeight, &WallJumpHeight);
	MovementDataTypeMap.Add(EMovementDataType::Acceleration, &Acceleration);
	MovementDataTypeMap.Add(EMovementDataType::Deceleration, &Deceleration);
	MovementDataTypeMap.Add(EMovementDataType::JumpHorizontalBoost, &JumpHorizontalBoost);
	MovementDataTypeMap.Add(EMovementDataType::AirDirectionInterpSpeed, &AirDirectionInterpSpeed);
	MovementDataTypeMap.Add(EMovementDataType::AirAcceleration, &AirAcceleration);
	MovementDataTypeMap.Add(EMovementDataType::AirDeceleration, &AirDeceleration);
	MovementDataTypeMap.Add(EMovementDataType::MaxFallVerticalSpeed, &MaxFallVerticalSpeed);
	MovementDataTypeMap.Add(EMovementDataType::MaxWalkableFloorAngle, &MaxWalkableFloorAngle);
	MovementDataTypeMap.Add(EMovementDataType::MaxStepHeight, &MaxStepHeight);
	MovementDataTypeMap.Add(EMovementDataType::MaxJumpHoldTime, &MaxJumpHoldTime);
	MovementDataTypeMap.Add(EMovementDataType::WallRunMaxDuration, &WallRunMaxDuration);
	MovementDataTypeMap.Add(EMovementDataType::WallRunAcceleration, &WallRunAcceleration);
	MovementDataTypeMap.Add(EMovementDataType::WallRunDeceleration, &WallRunDeceleration);
	MovementDataTypeMap.Add(EMovementDataType::WallRunMaxDuration, &WallRunMaxDuration);
	MovementDataTypeMap.Add(EMovementDataType::WallRunAcceleration, &WallRunAcceleration);
	MovementDataTypeMap.Add(EMovementDataType::WallRunDeceleration, &WallRunDeceleration);
	MovementDataTypeMap.Add(EMovementDataType::WallRunMaxSpeed, &WallRunMaxSpeed);
	MovementDataTypeMap.Add(EMovementDataType::WallRunBackwardMaxSpeed, &WallRunBackwardMaxSpeed);
	MovementDataTypeMap.Add(EMovementDataType::WallRunJumpAirSpeed2D, &WallRunJumpAirSpeed2D);
	MovementDataTypeMap.Add(EMovementDataType::WallRunJumpNormalForce, &WallRunJumpNormalForce);
	MovementDataTypeMap.Add(EMovementDataType::WallRunCameraTiltAngle, &WallRunCameraTiltAngle);
	MovementDataTypeMap.Add(EMovementDataType::PreWallRunDetectionRange, &PreWallRunDetectionRange);
	MovementDataTypeMap.Add(EMovementDataType::WallRunCameraTiltInterpSpeed, &WallRunCameraTiltInterpSpeed);
	MovementDataTypeMap.Add(EMovementDataType::SlideInitialWindow, &SlideInitialWindow);
	MovementDataTypeMap.Add(EMovementDataType::SlideMaxDuration, &SlideMaxDuration);
	MovementDataTypeMap.Add(EMovementDataType::SlideAdditionalSpeed, &SlideAdditionalSpeed);
	MovementDataTypeMap.Add(EMovementDataType::GroundPointDetectionLength, &GroundPointDetectionLength);
	MovementDataTypeMap.Add(EMovementDataType::CoyoteTime, &CoyoteTime);
	MovementDataTypeMap.Add(EMovementDataType::DamageSlowDebuffMultiplier, &DamageSlowDebuffMultiplier);
	MovementDataTypeMap.Add(EMovementDataType::DamageSlowDebuffDuration, &DamageSlowDebuffDuration);
	MovementDataTypeMap.Add(EMovementDataType::DownedDuration, &DownedDuration);
	MovementDataTypeMap.Add(EMovementDataType::DownedInvincibleDuration, &DownedInvincibleDuration);
}

void UPlayerMovementComponent::InitMovementTriggerKeyMap()
{
	MovementTriggerKeyMap.Add(EMovementTriggerKey::Ctrl, false);
	MovementTriggerKeyMap.Add(EMovementTriggerKey::Shift, false);
	MovementTriggerKeyMap.Add(EMovementTriggerKey::Space, false);
	MovementTriggerKeyMap.Add(EMovementTriggerKey::W, false);
	MovementTriggerKeyMap.Add(EMovementTriggerKey::A, false);
	MovementTriggerKeyMap.Add(EMovementTriggerKey::S, false);
	MovementTriggerKeyMap.Add(EMovementTriggerKey::D, false);
}

