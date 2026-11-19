#include "Characters/Enemies/EnemyStateMachineComponent.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyPatrolPoint.h"
#include "Characters/HealthComponent.h"
#include "Characters/StaminaComponent.h"
#include "Characters/BloodStainActor.h"
#include "SoundSystem/EnemyAudioComponent.h"
#include "Objects/DestructibleDoor.h"
#include "Objects/DestructibleObject.h"
#include "Components/StaticMeshComponent.h"

#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "UObject/EnumProperty.h"

UEnemyStateMachineComponent::UEnemyStateMachineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyStateMachineComponent::BeginPlay()
{
	Super::BeginPlay();

	Enemy = Cast<AEnemyBase>(GetOwner());

	if (Enemy)
	{
		AIController = Cast<AEnemyBaseAIController>(Enemy->GetController());
		HealthComponent = Enemy->GetHealthComponent();
		StaminaComponent = Enemy->GetStaminaComponent();
		EnemyAudioComponent = Enemy->GetAudioComponent();
	}

	TargetActor = nullptr;

	if (PatrolPoints.Num() > 0)
	{
		SetState(EEnemyStateType::Patrol);
	}
	else
	{
		SetState(EEnemyStateType::Idle);
	}
}

void UEnemyStateMachineComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Controllers can possess the Enemy after BeginPlay.
	if (Enemy && !IsValid(AIController))
	{
		AIController = Cast<AEnemyBaseAIController>(Enemy->GetController());
	}
	UpdateState(DeltaTime);
}

void UEnemyStateMachineComponent::OnIdleFinished()
{
	// TODO: 일단 무조건 Patrol인데, 조건 추가해야할듯
	SetState(EEnemyStateType::Patrol);
}

void UEnemyStateMachineComponent::BeginIdleLook()
{
	if (!Enemy || !GetWorld()) { return; }

	Task_StopMovement();
	//SetOrientRotationToMovement(false);

	IdleBaseRotation = Enemy->GetActorRotation();
	IdleBaseRotation.Pitch = 0.f;
	IdleBaseRotation.Roll = 0.f;

	IdleLookDirectionIndex = 0;
	IdleLookPhase = EEnemyIdleLookPhase::Waiting;
	IdlePhaseStartTime = GetWorld()->GetTimeSeconds();

	CurrentIdleLookWaitTime = FMath::FRandRange(IdleLookWaitTimeMin, IdleLookWaitTimeMax);

	IdleTargetRotation = IdleBaseRotation;
}

void UEnemyStateMachineComponent::UpdateIdleLook(float DeltaTime)
{
	if (!Enemy || !GetWorld()) { return; }

	switch (IdleLookPhase)
	{
	case EEnemyIdleLookPhase::Waiting:
	{
		const float ElapsedTime = GetWorld()->GetTimeSeconds() - IdlePhaseStartTime;

		if (ElapsedTime >= CurrentIdleLookWaitTime)
		{
			SelectNextIdleLookDirection();
			IdleLookPhase = EEnemyIdleLookPhase::Rotating;
		}
		break;
	}

	case EEnemyIdleLookPhase::Rotating:
	{
		const FRotator CurrentRotation = Enemy->GetActorRotation();

		const FRotator NewRotation = FMath::RInterpConstantTo(
			CurrentRotation,
			IdleTargetRotation,
			DeltaTime,
			IdleLookRotationRate
		);

		Enemy->SetActorRotation(FRotator(0.f, NewRotation.Yaw, 0.f));

		if (HasReachedIdleLookRotation())
		{
			Enemy->SetActorRotation(IdleTargetRotation);

			IdleLookPhase = EEnemyIdleLookPhase::Waiting;
			IdlePhaseStartTime = GetWorld()->GetTimeSeconds();

			CurrentIdleLookWaitTime = FMath::FRandRange(IdleLookWaitTimeMin, IdleLookWaitTimeMax);
		}

		break;
	}
	}
}

void UEnemyStateMachineComponent::SelectNextIdleLookDirection()
{
	if (!Enemy || IdleLookYawOffsets.IsEmpty()) { return; }

	if (!IdleLookYawOffsets.IsValidIndex(IdleLookDirectionIndex))
	{
		IdleLookDirectionIndex = 0;
	}

	const float YawOffset = IdleLookYawOffsets[IdleLookDirectionIndex];

	IdleTargetRotation = FRotator(0.f, IdleBaseRotation.Yaw + YawOffset, 0.f);
	IdleTargetRotation.Normalize();
	IdleLookDirectionIndex = (IdleLookDirectionIndex + 1) % IdleLookYawOffsets.Num();
}

bool UEnemyStateMachineComponent::HasReachedIdleLookRotation() const
{
	if (!Enemy) { return false; }
	const float YawDifference = FMath::Abs(
		FMath::FindDeltaAngleDegrees(Enemy->GetActorRotation().Yaw, IdleTargetRotation.Yaw));
	return YawDifference <= IdleLookAcceptanceAngle;
}

//void UEnemyStateMachineComponent::SetOrientRotationToMovement(bool bEnable)
//{
//	if (!Enemy) { return; }
//
//	//TODO: Rotation 함수 수정하기
//	if (UCharacterMovementComponent* MovementComponent = Enemy->GetCharacterMovement())
//	{
//		MovementComponent->bOrientRotationToMovement = bEnable;
//	}
//
//	/*
//	 * 직접 Actor를 회전할 것이므로
//	 * Controller Rotation Yaw도 사용하지 않는다.
//	 */
//	Enemy->bUseControllerRotationYaw = false;
//}

void UEnemyStateMachineComponent::SetTarget(AActor* NewTarget)
{
	// Never transfer the previous target's exact sight memory to a new actor.
	if (TargetActor.Get() != NewTarget)
	{
		bHasLastSeenTargetLocation = false;
		LastSeenTargetLocation = FVector::ZeroVector;
	}
	TargetActor = NewTarget;
}

void UEnemyStateMachineComponent::ClearTarget()
{
	TargetActor = nullptr;
	bTargetCurrentlyVisible = false;
	ResetCombatDodgeMovement();
}


void UEnemyStateMachineComponent::OnTargetFound()
{
	const bool bSightJustRegained = !bTargetCurrentlyVisible;
	bTargetCurrentlyVisible = true;
	if (HasTarget())
	{
		LastSeenTargetLocation = TargetActor->GetActorLocation();
		bHasLastSeenTargetLocation = true;
	}
	// If sight was regained while escaping an impact, use the same sound
	// memory to begin lateral movement immediately on resuming Chase.
	if (bSightJustRegained && IsRecentImpactForDodge() && Enemy &&
		FVector::Dist2D(Enemy->GetActorLocation(), LastImpactLocation) <= ImpactReactionRadius)
	{
		RegisterCombatThreat(true);
	}
	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (TimerManager.IsTimerActive(TargetDurationTimerHandle))
	{
		TimerManager.ClearTimer(TargetDurationTimerHandle);
	}
}

void UEnemyStateMachineComponent::OnTargetMissed()
{
	bTargetCurrentlyVisible = false;
	// TargetMemoryDuration remains intact; do not use live Actor coordinates
	// for new lateral waypoints after sight is lost.
	bHasCombatDodgeWaypoint = false;
	TWeakObjectPtr WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(TargetDurationTimerHandle, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (auto* HardThis = WeakThis.Get())
			{
				HardThis->StopChase();
			}
		}), TargetMemoryDuration, false);
}

void UEnemyStateMachineComponent::SetState(EEnemyStateType NewState)
{
	if (CurrentState == NewState) return;

	ExitState(CurrentState);
	CurrentState = NewState;
	EnterState(CurrentState);

	if (EnemyAudioComponent)
	{
		EnemyAudioComponent->NotifyStateChanged(CurrentState);
	}
}

EEnemyStateType UEnemyStateMachineComponent::GetCurrentState() const
{
	return CurrentState;
}

bool UEnemyStateMachineComponent::IsInDefensiveState() const
{
	return IsDefensiveState();
}

void UEnemyStateMachineComponent::NotifyThreatSeen(AActor* ThreatActor)
{
	if (ThreatActor)
	{
		TargetActor = ThreatActor;
		bTargetCurrentlyVisible = true;
	}
}

void UEnemyStateMachineComponent::EnterState(EEnemyStateType NewState)
{
	switch (NewState)
	{
	case EEnemyStateType::Idle:
		UE_LOG(LogTemp, Warning, TEXT("Enter Idle"));
		Task_StopMovement();
		GetWorld()->GetTimerManager().SetTimer(
			IdleToPatrolTimerHandle,
			this,
			&UEnemyStateMachineComponent::OnIdleFinished,
			IdleToPatrolTime,
			false
		);
		BeginIdleLook();
		break;

	case EEnemyStateType::Patrol:
		UE_LOG(LogTemp, Warning, TEXT("Enter Patrol"));
		bIsWaitingAtPatrolPoint = false;
		MoveToCurrentPatrolPoint();
		//SetOrientRotationToMovement(true);
		break;

	case EEnemyStateType::Chase:
		ResetDoorBreachProgress();
		bHasCombatDodgeWaypoint = false;
		bForceCombatDodgeRepath = true;
		UE_LOG(LogTemp, Warning, TEXT("Enter Chase"));

		//FTimerManager& TimerManager = GetWorld()->GetTimerManager();
		if (GetWorld()->GetTimerManager().IsTimerActive(TargetDurationTimerHandle))
		{
			GetWorld()->GetTimerManager().ClearTimer(TargetDurationTimerHandle);
		}

		if (EnemyAudioComponent)
		{
			EnemyAudioComponent->PlayEvent(EEnemyAudioEvent::ChaseStart);
		}

		break;

	case EEnemyStateType::BreachDoor:
	{
		Task_StopMovement();
		DoorBreachStartedAt = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		LastDoorApproachMoveAt = -BIG_NUMBER;
		bHasDoorBreachApproachPosition = false;
		break;
	}

	case EEnemyStateType::EvadeImpact:
	{
		Task_StopMovement();
		ImpactEvadeStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		bHasImpactEvadeLocation = FindImpactEvadeLocation(ImpactEvadeLocation);
		if (bHasImpactEvadeLocation)
		{
			MoveToImpactEvadeLocation();
		}
		break;
	}

	case EEnemyStateType::Investigate:
		UE_LOG(LogTemp, Warning, TEXT("Enter Investigate"));
		InvestigateStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		bIsWaitingAtInvestigateLocation = false;
		MoveToLastHeardLocation();
		break;

	case EEnemyStateType::TrackBlood:
	{
		if (CurrentBloodStain && AIController)
		{
			AIController->MoveToLocationPoint_Upgrade(CurrentBloodStain->GetActorLocation(), BloodTrackAcceptanceRadius);
		}
		break;
	}
	case EEnemyStateType::Flee:
	{
		UE_LOG(LogTemp, Warning, TEXT("Enter Flee"));
		AccumulatedBurstDamage = 0.f;

		if (!bHasValidHideLocation)
		{
			bHasValidHideLocation = FindHideLocation_Upgrade(TargetActor, HideLocation);
		}

		if (bHasValidHideLocation)
		{
			UE_LOG(LogTemp, Error, TEXT("ValidHideLocation"));

			Task_MoveToHideLocation();
		}
		else
		{
			/*
			 * 은신 위치를 못 찾았으면 단순히 플레이어 반대 방향으로
			 * 임시 도망 위치를 만든다.
			 */
			if (Enemy && TargetActor)
			{
				UE_LOG(LogTemp, Error, TEXT("No Way!!!!!!"));

				FVector AwayDirection = Enemy->GetActorLocation() - TargetActor->GetActorLocation();

				AwayDirection.Z = 0.f;
				AwayDirection.Normalize();

				HideLocation = Enemy->GetActorLocation() + AwayDirection * MinimumFleeDistanceFromPlayer;

				bHasValidHideLocation = true;
				Task_MoveToHideLocation();
			}
		}

		break;
	}

	case EEnemyStateType::Hide:
		UE_LOG(LogTemp, Warning, TEXT("Enter Hide"));
		Task_StopMovement();
		HideStartTime = GetWorld()->GetTimeSeconds();
		break;

	case EEnemyStateType::Recover:
		UE_LOG(LogTemp, Warning, TEXT("Enter Recover"));
		Task_StopMovement();
		break;

	case EEnemyStateType::Attack:
		UE_LOG(LogTemp, Warning, TEXT("Enter Attack"));
		Task_StopMovement();
		break;

	case EEnemyStateType::Dead:
		UE_LOG(LogTemp, Warning, TEXT("Enter Dead"));
		Task_StopMovement();
		ClearTarget();

		if (EnemyAudioComponent)
		{
			EnemyAudioComponent->NotifyDeath();
		}

		SetComponentTickEnabled(false);
		break;
	}
}

void UEnemyStateMachineComponent::ExitState(EEnemyStateType OldState)
{
	switch (OldState)
	{
	case EEnemyStateType::Idle:
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(IdleToPatrolTimerHandle);
		}
		break;
	case EEnemyStateType::Patrol:
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(PatrolWaitTimerHandle);
		}
		bIsWaitingAtPatrolPoint = false;
		break;

	case EEnemyStateType::Chase:
		ResetCombatDodgeMovement();
		break;

	case EEnemyStateType::BreachDoor:
		BreachDoorTarget.Reset();
		bHasDoorBreachApproachPosition = false;
		break;

	case EEnemyStateType::EvadeImpact:
		bHasImpactEvadeLocation = false;
		break;

	case EEnemyStateType::Investigate:
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(InvestigateWaitTimerHandle);
		}
		bIsWaitingAtInvestigateLocation = false;
		break;

	case EEnemyStateType::Flee:
		break;

	case EEnemyStateType::Hide:
		break;

	case EEnemyStateType::Recover:
		break;

	case EEnemyStateType::Attack:
		break;

	case EEnemyStateType::Dead:
		break;
	}
}

void UEnemyStateMachineComponent::UpdateState(float DeltaTime)
{
	if (!Enemy) return;

	if (Enemy->IsDead())
	{
		SetState(EEnemyStateType::Dead);
		return;
	}

	switch (CurrentState)
	{
	case EEnemyStateType::Idle:
		UpdateIdle(DeltaTime);
		break;

	case EEnemyStateType::Patrol:
		UpdatePatrol(DeltaTime);
		break;

	case EEnemyStateType::Chase:
		UpdateChase(DeltaTime);
		break;

	case EEnemyStateType::BreachDoor:
		UpdateBreachDoor(DeltaTime);
		break;

	case EEnemyStateType::EvadeImpact:
		UpdateEvadeImpact(DeltaTime);
		break;

	case EEnemyStateType::Investigate:
		UpdateInvestigate(DeltaTime);
		break;

	case EEnemyStateType::TrackBlood:
		UpdateTrackBlood(DeltaTime);
		break;

	case EEnemyStateType::Flee:
		UpdateFlee(DeltaTime);
		break;

	case EEnemyStateType::Hide:
		UpdateHide(DeltaTime);
		break;

	case EEnemyStateType::Recover:
		UpdateRecover(DeltaTime);
		break;

	case EEnemyStateType::Attack:
		UpdateAttack(DeltaTime);
		break;

	case EEnemyStateType::Dead:
		UpdateDead(DeltaTime);
		break;
	}
}

void UEnemyStateMachineComponent::UpdateIdle(float DeltaTime)
{
	if (HasTarget())
	{
		SetState(EEnemyStateType::Chase);
		return;
	}

	if (CanSearchForBlood())
	{
		LastBloodSearchTime = GetWorld()->GetTimeSeconds();
		if (TryAcquireBloodTrail()) { return; }
	}

	UpdateIdleLook(DeltaTime);
	//MEMO: Idle To Patrol 전환은 일단 EnterState에서 타이머로 해결했음
}

void UEnemyStateMachineComponent::UpdatePatrol(float DeltaTime)
{
	if (HasTarget())
	{
		SetState(EEnemyStateType::Chase);
		return;
	}

	if (CanSearchForBlood())
	{
		LastBloodSearchTime = GetWorld()->GetTimeSeconds();
		if (TryAcquireBloodTrail()) { return; }
	}

	if (PatrolPoints.Num() <= 0)
	{
		SetState(EEnemyStateType::Idle);
		return;
	}

	if (bIsWaitingAtPatrolPoint)
	{
		return;
	}

	FVector TargetLocation = Enemy->GetActorLocation() + Enemy->GetVelocity();
	RotateToTargetDirection(DeltaTime, TargetLocation);
	//RotateToTargetDirection(DeltaTime, GetCurrPatrolPointLocation());

	if (IsAtCurrentPatrolPoint())
	{
		bIsWaitingAtPatrolPoint = true;
		Task_StopMovement();

		UE_LOG(LogTemp, Error, TEXT("IsAtCurrentPatrolPoint()"));

		GetWorld()->GetTimerManager().SetTimer(
			PatrolWaitTimerHandle,
			this,
			&UEnemyStateMachineComponent::OnPatrolWaitFinished,
			PatrolWaitTime,
			false
		);

		return;
	}
}


// -----------------------------------------------------------------------------
// Door breach (AI knows an obstructing door, NOT the player position through it)
// -----------------------------------------------------------------------------
void UEnemyStateMachineComponent::ResetDoorBreachProgress()
{
	if (!GetWorld() || !Enemy) return;
	LastDoorChaseProgressLocation = Enemy->GetActorLocation();
	LastDoorChaseProgressAt = GetWorld()->GetTimeSeconds();
}

void UEnemyStateMachineComponent::UpdateDoorChaseMovementProgress()
{
	if (!Enemy || !GetWorld()) return;
	const FVector Here = Enemy->GetActorLocation();
	if (LastDoorChaseProgressAt < -100000.f ||
		FVector::DistSquared2D(Here, LastDoorChaseProgressLocation) >=
		FMath::Square(DoorBreachStallProgressDistance))
	{
		LastDoorChaseProgressLocation = Here;
		LastDoorChaseProgressAt = GetWorld()->GetTimeSeconds();
	}
}

bool UEnemyStateMachineComponent::IsDoorChaseMovementStalled() const
{
	return IsValid(Enemy) && GetWorld() &&
		Enemy->GetVelocity().SizeSquared2D() < FMath::Square(35.f) &&
		GetWorld()->GetTimeSeconds() - LastDoorChaseProgressAt >= DoorBreachStallDuration;
}

bool UEnemyStateMachineComponent::IsDoorBlockingLineTo(
	const ADestructibleDoor* Door, const FVector& Goal) const
{
	if (!IsValid(Door) || !IsValid(Enemy) || !GetWorld()) return false;
	FVector Start = Enemy->GetActorLocation() + FVector(0.f, 0.f, 80.f);
	FVector End = Goal + FVector(0.f, 0.f, 80.f);
	if (FVector::DistSquared2D(Start, End) < FMath::Square(40.f)) return false;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyDoorBlockingTrace), false);
	Params.AddIgnoredActor(Enemy);
	if (HasTarget()) Params.AddIgnoredActor(TargetActor.Get());

	FHitResult Result;
	return GetWorld()->LineTraceSingleByChannel(Result, Start, End, ECC_Visibility, Params)
		&& Result.GetActor() == Door;
}

bool UEnemyStateMachineComponent::FindDoorBlockingDestination(
	const FVector& Goal, ADestructibleDoor*& OutDoor) const
{
	OutDoor = nullptr;
	if (!IsValid(Enemy) || !GetWorld()) return false;
	const FVector Start = Enemy->GetActorLocation() + FVector(0.f, 0.f, 80.f);
	const FVector End = Goal + FVector(0.f, 0.f, 80.f);
	if (FVector::DistSquared2D(Start, End) <= FMath::Square(80.f)) return false;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyDoorCandidateTrace), false);
	Params.AddIgnoredActor(Enemy);
	if (HasTarget()) Params.AddIgnoredActor(TargetActor.Get());
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params)) return false;

	// ADestructibleDoor derives from ADestructibleObject. Recognize the
	// damageable object first, then apply door-specific breach policy.
	ADestructibleObject* Obstacle = Cast<ADestructibleObject>(Hit.GetActor());
	if (!IsValid(Obstacle) || !Obstacle->CanBeDamagedByAI()) return false;
	ADestructibleDoor* Candidate = Cast<ADestructibleDoor>(Obstacle);
	if (!IsValid(Candidate) || !Candidate->CanEnemyBreachDoor()) return false;
	const FVector Surface = Candidate->GetEnemyBreachSurfacePoint(Enemy->GetActorLocation());
	if (FVector::DistSquared2D(Enemy->GetActorLocation(), Surface) >
		FMath::Square(DoorBreachDetectionRadius)) return false;

	OutDoor = Candidate;
	return true;
}

bool UEnemyStateMachineComponent::IsDoorSignificantObstacle(const FVector& Goal) const
{
	if (!IsValid(Enemy) || !GetWorld()) return false;
	// Prefer a reasonable walking detour over needlessly attacking a door.
	const FVector Start = Enemy->GetActorLocation();
	const float Direct = FVector::Dist2D(Start, Goal);
	if (Direct < 80.f) return false;
	UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
		GetWorld(), Start, Goal, Enemy);
	if (!IsValid(Path) || !Path->IsValid() || Path->IsPartial() ||
		Path->PathPoints.Num() < 2)
	{
		return true; // no complete NavMesh route to a tracked/investigated goal
	}

	float RouteLength = 0.f;
	for (int32 Index = 1; Index < Path->PathPoints.Num(); ++Index)
	{
		RouteLength += FVector::Dist2D(Path->PathPoints[Index - 1], Path->PathPoints[Index]);
	}
	return RouteLength > Direct * DoorBreachDetourRatio || IsDoorChaseMovementStalled();
}

bool UEnemyStateMachineComponent::TryBeginDoorBreach(
	const FVector& Goal, EEnemyStateType ReturnState)
{
	if (!bEnableDoorBreach || !Enemy || !GetWorld() ||
		CurrentState == EEnemyStateType::BreachDoor || IsDefensiveState()) return false;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastDoorBreachCheckAt < DoorBreachSearchInterval ||
		Now - LastDoorBreachAbortAt < DoorBreachFailureCooldown) return false;
	LastDoorBreachCheckAt = Now;

	ADestructibleDoor* Found = nullptr;
	if (!FindDoorBlockingDestination(Goal, Found) ||
		!IsDoorSignificantObstacle(Goal)) return false;

	BreachDoorTarget = Found;
	DoorBreachReturnState = ReturnState;
	SetState(EEnemyStateType::BreachDoor);
	return true;
}

bool UEnemyStateMachineComponent::FindDoorBreachApproachPosition(
	const ADestructibleDoor* Door, FVector& OutLocation) const
{
	if (!IsValid(Door) || !Enemy || !GetWorld()) return false;
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav) return false;
	const FVector Start = Enemy->GetActorLocation();
	const FVector Surface = Door->GetEnemyBreachSurfacePoint(Start);
	FVector Away = (Start - Surface).GetSafeNormal2D();
	if (Away.IsNearlyZero()) Away = (Start - Door->GetActorLocation()).GetSafeNormal2D();
	if (Away.IsNearlyZero()) Away = Enemy->GetActorForwardVector() * -1.f;

	const float Reach = FMath::Min(DoorBreachAttackReach,
		FMath::Max(35.f, Enemy->AttackRange));
	// Try progressively nearer approach points on THIS side of the door.
	// Only choose a NavMesh location from which melee can actually reach it.
	const float OffsetScales[] = { 1.f, 0.68f, 0.45f };
	for (const float Scale : OffsetScales)
	{
		const FVector Candidate = Surface + Away * DoorBreachApproachOffset * Scale;
		FNavLocation Projected;
		if (!Nav->ProjectPointToNavigation(Candidate, Projected,
			FVector(140.f, 140.f, 280.f))) continue;
		// Rejected projections that landed on the far side of the doorway.
		if (FVector::DotProduct(Projected.Location - Surface, Away) < 5.f) continue;
		if (FVector::Dist2D(Projected.Location,
			Door->GetEnemyBreachSurfacePoint(Projected.Location)) > Reach - 10.f)
			continue;

		UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
			GetWorld(), Start, Projected.Location, Enemy);
		if (!IsValid(Path) || !Path->IsValid() || Path->IsPartial()) continue;

		OutLocation = Projected.Location;
		return true;
	}
	return false;
}

void UEnemyStateMachineComponent::UpdateBreachDoor(float DeltaTime)
{
	if (!Enemy || !GetWorld()) return;
	ADestructibleDoor* Door = BreachDoorTarget.Get();
	if (!IsValid(Door) || Door->IsDead() || Door->IsDoorOpen() ||
		!Door->CanEnemyBreachDoor())
	{
		FinishDoorBreach(false);
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - DoorBreachStartedAt >= DoorBreachMaximumDuration)
	{
		FinishDoorBreach(true);
		return;
	}

	// The door must STILL be the first obstruction toward our last-known
	// sight position (or the approximate gunshot investigation destination).
	// Never allow raw interface damage on a door behind some other wall.
	FVector IntendedGoal = DoorBreachReturnState == EEnemyStateType::Investigate
		? LastHeardLocation : LastSeenTargetLocation;
	const bool bHaveIntendedGoal = DoorBreachReturnState == EEnemyStateType::Investigate
		? bHasHeardLocation : bHasLastSeenTargetLocation;
	if (bHaveIntendedGoal &&
		!IsDoorBlockingLineTo(Door,
			bTargetCurrentlyVisible && HasTarget()
			? TargetActor->GetActorLocation() : IntendedGoal))
	{
		FinishDoorBreach(false);
		return;
	}

	const FVector Here = Enemy->GetActorLocation();
	const FVector Surface = Door->GetEnemyBreachSurfacePoint(Here);
	const float Distance = FVector::Dist2D(Here, Surface);
	const float AttackReach = FMath::Min(DoorBreachAttackReach,
		FMath::Max(35.f, Enemy->AttackRange));

	if (Distance <= AttackReach)
	{
		Task_StopMovement();
		RotateToTargetDirection(DeltaTime, Surface);
		// CanAttack() requires a Player target in the original implementation;
		// door damage must use the same cooldown even after sight memory expires.
		const bool bCooldownReady = Now - LastAttackTime >= Enemy->AttackCooldown;
		if (bCooldownReady)
		{
			if (StaminaComponent && !StaminaComponent->CanConsume(AttackStaminaCost))
			{
				FinishDoorBreach(true); // no endless empty attack animation
				return;
			}
			if (StaminaComponent) StaminaComponent->ConsumeStamina(AttackStaminaCost);
			LastAttackTime = Now;
			// Uses AEnemyBase's existing montage, melee damage and DamageInterface.
			Enemy->AttackTarget(Door);
		}
		return;
	}

	// Door remains ahead; move only to a point on our side of the intact mesh.
	if (Now - LastDoorApproachMoveAt >= DoorBreachMoveRequestInterval)
	{
		LastDoorApproachMoveAt = Now;
		FVector Approach;
		if (!FindDoorBreachApproachPosition(Door, Approach))
		{
			FinishDoorBreach(true);
			return;
		}
		DoorBreachApproachPosition = Approach;
		bHasDoorBreachApproachPosition = true;
		if (!IsValid(AIController))
			AIController = Cast<AEnemyBaseAIController>(Enemy->GetController());
		if (!IsValid(AIController) ||
			!AIController->MoveToDoorBreachPosition(Approach, DoorBreachApproachAcceptance))
		{
			FinishDoorBreach(true);
			return;
		}
	}
	const FVector FacingPoint = Enemy->GetActorLocation() + Enemy->GetVelocity();
	RotateToTargetDirection(DeltaTime, FacingPoint);

#if ENABLE_DRAW_DEBUG
	if (bDrawDoorBreachDebug)
	{
		DrawDebugSphere(GetWorld(), Surface, 18.f, 10, FColor::Orange, false, 0.15f);
		if (bHasDoorBreachApproachPosition)
			DrawDebugLine(GetWorld(), Here, DoorBreachApproachPosition, FColor::Orange, false, 0.15f, 0, 2.f);
	}
#endif
}

void UEnemyStateMachineComponent::FinishDoorBreach(bool bFailed)
{
	if (!GetWorld()) return;
	if (bFailed) LastDoorBreachAbortAt = GetWorld()->GetTimeSeconds();
	Task_StopMovement();

	// ExitState(BreachDoor) clears the weak pointer.
	if (DoorBreachReturnState == EEnemyStateType::Chase && HasTarget())
	{
		SetState(EEnemyStateType::Chase);
		return;
	}

	// Hearing is not equivalent to an exact player location. If sight has
	// expired, investigate the last known sight position or earlier sound.
	if (bHasLastSeenTargetLocation)
	{
		SetLastHeardLocation(LastSeenTargetLocation);
		SetState(EEnemyStateType::Investigate);
	}
	else if (bHasHeardLocation)
	{
		SetState(EEnemyStateType::Investigate);
	}
	else
	{
		SetState(PatrolPoints.Num() ? EEnemyStateType::Patrol : EEnemyStateType::Idle);
	}
}

void UEnemyStateMachineComponent::UpdateChase(float DeltaTime)
{
	if (!HasTarget() || ShouldLoseTarget())
	{
		StopChase();
		return;
	}

	if (HealthComponent && HealthComponent->IsDead())
	{
		SetState(EEnemyStateType::Dead);
		return;
	}

	if (StaminaComponent)
	{
		const float ChaseCost = ChaseStaminaCostPerSecond * DeltaTime;

		if (!StaminaComponent->ConsumeStamina(ChaseCost))
		{
			//TODO: 조건을 조금 더 상세하게 지정할 필요 있음

			SetState(EEnemyStateType::Patrol);
			return;
		}
	}

	// A closed, weakened door may be blocking the chase even when NavMesh
	// suggests a direct path through it. Detect only the FIRST visible blocker.
	// Zigzag / attack target only after this obstacle priority is resolved.
	if (bTargetCurrentlyVisible && HasTarget())
	{
		LastSeenTargetLocation = TargetActor->GetActorLocation();
		bHasLastSeenTargetLocation = true;
	}
	UpdateDoorChaseMovementProgress();
	if (bTargetCurrentlyVisible || bHasLastSeenTargetLocation)
	{
		const FVector ObstacleGoal = bTargetCurrentlyVisible
			? TargetActor->GetActorLocation() : LastSeenTargetLocation;
		if (TryBeginDoorBreach(ObstacleGoal, EEnemyStateType::Chase)) return;
	}

	// Movement plan: first honor the existing melee range, then use
	// time-limited, navmesh-based lateral waypoints while under fire.
	const bool bImmediateImpactDanger = IsCombatDodgeActive() && IsRecentImpactForDodge() &&
		FVector::Dist2D(Enemy->GetActorLocation(), LastImpactLocation) <= CombatDodgeHazardRadius;
	if (IsTargetInAttackRange() && !bImmediateImpactDanger)
	{
		SetState(EEnemyStateType::Attack);
		return;
	}

	const FVector MovementLookPoint = Enemy->GetActorLocation() + Enemy->GetVelocity();
	RotateToTargetDirection(DeltaTime, MovementLookPoint);

	if (UpdateCombatDodgeChase())
	{
		return;
	}

	// Previous behavior when not seeing the target, when fire stopped, or
	// if no complete navigable lateral path exists.
	Task_MoveToTarget();
}

void UEnemyStateMachineComponent::UpdateAttack(float DeltaTime)
{
	if (!HasTarget())
	{
		SetState(EEnemyStateType::Patrol);
		return;
	}

	if (ShouldLoseTarget())
	{
		ClearTarget();
		SetState(EEnemyStateType::Patrol);
		return;
	}

	if (!IsTargetInAttackRange())
	{
		SetState(EEnemyStateType::Chase);
		return;
	}

	// The player can be in melee range but behind a closed door.
	UpdateDoorChaseMovementProgress();
	if ((bTargetCurrentlyVisible || bHasLastSeenTargetLocation) && TryBeginDoorBreach(
		bTargetCurrentlyVisible ? TargetActor->GetActorLocation() : LastSeenTargetLocation,
		EEnemyStateType::Chase)) return;

	Task_StopMovement();
	RotateToTarget(DeltaTime);

	if (CanAttack())
	{
		if (StaminaComponent && !StaminaComponent->CanConsume(AttackStaminaCost))
		{
			SetState(EEnemyStateType::Chase);
			return;
		}

		if (StaminaComponent)
		{
			StaminaComponent->ConsumeStamina(AttackStaminaCost);
		}

		Task_AttackTarget();
	}
}

void UEnemyStateMachineComponent::UpdateFlee(float DeltaTime)
{
	if (!Enemy || !HealthComponent)
	{
		return;
	}

	if (HealthComponent->IsDead())
	{
		SetState(EEnemyStateType::Dead);
		return;
	}

	if (!bHasValidHideLocation)
	{
		if (!FindHideLocation_Upgrade(TargetActor, HideLocation))
		{
			return;
		}

		bHasValidHideLocation = true;
		Task_MoveToHideLocation();
	}

	FVector TargetLocation = Enemy->GetActorLocation() + Enemy->GetVelocity();
	RotateToTargetDirection(DeltaTime, TargetLocation);

	/*
	 * 도망 중 다시 플레이어에게 노출됐더라도 이동은 계속한다.
	 * 후보 지점에 도착하면 실제로 가려졌는지 다시 검사한다.
	 */
	if (!IsAtHideLocation())
	{
		Task_MoveToHideLocation();
		return;
	}

	Task_StopMovement();

	if (IsLocationHiddenFromActor(HideLocation, TargetActor))
	{
		SetState(EEnemyStateType::Hide);
		return;
	}

	/*
	 * 도착했는데 여전히 보이는 위치라면 다른 은신 위치를 찾는다.
	 */
	bHasValidHideLocation = FindHideLocation_Upgrade(
		TargetActor,
		HideLocation
	);

	if (bHasValidHideLocation)
	{
		Task_MoveToHideLocation();
	}
}

void UEnemyStateMachineComponent::UpdateHide(float DeltaTime)
{
	if (!Enemy || !HealthComponent)
	{
		return;
	}

	if (HealthComponent->IsDead())
	{
		SetState(EEnemyStateType::Dead);
		return;
	}

	/*
	 * 숨어 있는 동안 플레이어에게 다시 노출되면
	 * 새로운 위치로 도망간다.
	 */
	if (TargetActor &&
		!IsLocationHiddenFromActor(Enemy->GetActorLocation(), TargetActor))
	{
		bHasValidHideLocation = FindHideLocation_Upgrade(
			TargetActor,
			HideLocation
		);

		SetState(EEnemyStateType::Flee);
		return;
	}

	if (!HasMinimumHideTimeElapsed())
	{
		return;
	}

	if (HealthComponent->GetCurrentHealth() < RecoveryHealthCap)
	{
		SetState(EEnemyStateType::Recover);
		return;
	}

	ReturnToPreviousBehavior();
}

void UEnemyStateMachineComponent::UpdateRecover(float DeltaTime)
{
	if (!Enemy || !HealthComponent)
	{
		return;
	}

	if (HealthComponent->IsDead())
	{
		SetState(EEnemyStateType::Dead);
		return;
	}

	/*
	 * 회복 중 다시 발각되면 즉시 도망.
	 */
	if (TargetActor &&
		!IsLocationHiddenFromActor(Enemy->GetActorLocation(), TargetActor))
	{
		bHasValidHideLocation = FindHideLocation_Upgrade(
			TargetActor,
			HideLocation
		);

		SetState(EEnemyStateType::Flee);
		return;
	}

	if (!IsRecoveryComplete())
	{
		const float RemainingRecoverableHealth =
			RecoveryHealthCap -
			HealthComponent->GetCurrentHealth();

		const float HealAmount = FMath::Min(
			RecoveryHealthPerSecond * DeltaTime,
			RemainingRecoverableHealth
		);

		HealthComponent->Heal(HealAmount);
	}

	if (IsRecoveryComplete() || HasMaximumHideTimeElapsed())
	{
		ReturnToPreviousBehavior();
	}
}

void UEnemyStateMachineComponent::UpdateDead(float DeltaTime)
{
	// 사망 후 처리
	// Collision 끄기, Ragdoll, Destroy Timer 등
}

bool UEnemyStateMachineComponent::HasTarget() const
{
	return IsValid(TargetActor);
}

bool UEnemyStateMachineComponent::CanSeeTarget() const
{
	return HasTarget();
}

bool UEnemyStateMachineComponent::IsTargetInAttackRange() const
{
	if (!Enemy || !HasTarget()) return false;
	const float Distance = FVector::Dist(Enemy->GetActorLocation(), TargetActor->GetActorLocation());
	return Distance <= Enemy->AttackRange;
}

bool UEnemyStateMachineComponent::CanAttack() const
{
	if (!Enemy || !HasTarget()) return false;

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	return CurrentTime - LastAttackTime >= Enemy->AttackCooldown;
}

bool UEnemyStateMachineComponent::ShouldLoseTarget() const
{
	if (!Enemy || !HasTarget()) return true;

	const float Distance = FVector::Dist(
		Enemy->GetActorLocation(),
		TargetActor->GetActorLocation()
	);

	return Distance >= Enemy->LoseTargetDistance;
}

bool UEnemyStateMachineComponent::ShouldFlee() const
{
	if (!HealthComponent || HealthComponent->IsDead())
	{
		return false;
	}

	const bool bLowHealth = HealthComponent->GetHealthPercent() <= FleeHealthThresholdPercent;
	const bool bReceivedBurstDamage = AccumulatedBurstDamage >= BurstDamageThreshold;

	return bLowHealth || bReceivedBurstDamage;
}

bool UEnemyStateMachineComponent::IsDefensiveState() const
{
	return CurrentState == EEnemyStateType::Flee
		|| CurrentState == EEnemyStateType::Hide
		|| CurrentState == EEnemyStateType::Recover;
}

bool UEnemyStateMachineComponent::FindHideLocation(AActor* ThreatActor, FVector& OutLocation) const
{
	if (!Enemy || !ThreatActor || !GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("FindHideLocation Error 1"));

		return false;
	}

	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());

	if (!NavigationSystem)
	{
		UE_LOG(LogTemp, Error, TEXT("FindHideLocation Error 2"));

		return false;
	}

	const FVector EnemyLocation = Enemy->GetActorLocation();
	const FVector ThreatLocation = ThreatActor->GetActorLocation();

	bool bFoundLocation = false;
	float BestScore = -FLT_MAX;
	FVector BestLocation = FVector::ZeroVector;

	for (int32 Index = 0; Index < HideLocationSampleCount; ++Index)
	{
		FNavLocation CandidateNavLocation;

		const bool bFoundReachablePoint =
			NavigationSystem->GetRandomReachablePointInRadius(
				EnemyLocation,
				FleeSearchRadius,
				CandidateNavLocation
			);

		if (!bFoundReachablePoint)
		{
			continue;
		}

		const FVector CandidateLocation = CandidateNavLocation.Location;

		const float DistanceFromThreat = FVector::Dist2D(
			CandidateLocation,
			ThreatLocation
		);

		if (DistanceFromThreat < MinimumFleeDistanceFromPlayer)
		{
			continue;
		}

		if (!IsLocationHiddenFromActor(CandidateLocation, ThreatActor))
		{
			continue;
		}

		const float DistanceFromEnemy = FVector::Dist2D(
			CandidateLocation,
			EnemyLocation
		);

		/*
		 * 플레이어에게서 멀수록 높은 점수.
		 * Enemy가 지나치게 멀리 이동해야 하면 약간 감점.
		 */
		const float Score =
			DistanceFromThreat -
			DistanceFromEnemy * 0.25f;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestLocation = CandidateLocation;
			bFoundLocation = true;
		}
	}

	if (!bFoundLocation)
	{
		return false;
	}

	OutLocation = BestLocation;

#if ENABLE_DRAW_DEBUG
	DrawDebugSphere(
		GetWorld(),
		OutLocation,
		40.f,
		12,
		FColor::Blue,
		false,
		5.f
	);
#endif

	return true;
}

bool UEnemyStateMachineComponent::FindHideLocation_Upgrade(AActor* ThreatActor, FVector& OutLocation) const
{
	if (!Enemy || !ThreatActor || !GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("FindHideLocation failed: invalid Enemy, ThreatActor, or World"));
		return false;
	}

	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());

	if (!NavigationSystem)
	{
		UE_LOG(LogTemp, Error, TEXT("FindHideLocation failed: NavigationSystem is null"));
		return false;
	}

	if (FleeSearchRadii.IsEmpty() || HideLocationSampleCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FindHideLocation failed: invalid search radii or sample count"));
		return false;
	}

	const FVector EnemyLocation = Enemy->GetActorLocation();
	const FVector ThreatLocation = ThreatActor->GetActorLocation();

	bool bFoundLocation = false;
	float BestScore = -FLT_MAX;
	FVector BestLocation = FVector::ZeroVector;

	const float AngleStep = 360.f / static_cast<float>(HideLocationSampleCount);

	for (int32 RadiusIndex = 0; RadiusIndex < FleeSearchRadii.Num(); ++RadiusIndex)
	{
		const float SearchRadius = FleeSearchRadii[RadiusIndex];

		if (SearchRadius <= 0.f) { continue; }

		/*
		 * 모든 링이 정확히 같은 방향만 검사하지 않도록
		 * 홀수 번째 링은 각도 간격의 절반만큼 회전시킨다.
		 *
		 * 예:
		 * 첫 번째 링: 0, 30, 60, 90 ...
		 * 두 번째 링: 15, 45, 75, 105 ...
		 */
		const float RingAngleOffset = RadiusIndex % 2 == 0 ? 0.f : AngleStep * 0.5f;

		for (int32 SampleIndex = 0; SampleIndex < HideLocationSampleCount; ++SampleIndex)
		{
			const float AngleDegrees = SampleIndex * AngleStep + RingAngleOffset;
			const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);

			const FVector Direction(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.f);

			const FVector RawCandidateLocation = EnemyLocation + Direction * SearchRadius;

			/*
			 * 계산한 위치가 NavMesh 위에 정확히 있지 않을 수 있으므로
			 * 주변 NavMesh 지점으로 투영한다.
			 */
			FNavLocation ProjectedNavLocation;

			const FVector ProjectionExtent(
				100.f,
				100.f,
				300.f
			);

			const bool bProjectedToNavigation =
				NavigationSystem->ProjectPointToNavigation(
					RawCandidateLocation,
					ProjectedNavLocation,
					ProjectionExtent
				);

			if (!bProjectedToNavigation)
			{
#if ENABLE_DRAW_DEBUG
				DrawDebugSphere(
					GetWorld(),
					RawCandidateLocation,
					20.f,
					8,
					FColor::Red,
					false,
					10.f
				);
#endif

				continue;
			}

			const FVector CandidateLocation = ProjectedNavLocation.Location;

			const float DistanceFromThreat = FVector::Dist2D(CandidateLocation, ThreatLocation);

			if (DistanceFromThreat < MinimumFleeDistanceFromPlayer)
			{
#if ENABLE_DRAW_DEBUG
				DrawDebugSphere(
					GetWorld(),
					CandidateLocation,
					20.f,
					8,
					FColor::Orange,
					false,
					10.f
				);
#endif

				continue;
			}

			if (!IsLocationHiddenFromActor(CandidateLocation, ThreatActor))
			{
#if ENABLE_DRAW_DEBUG
				DrawDebugSphere(
					GetWorld(),
					CandidateLocation,
					20.f,
					8,
					FColor::Yellow,
					false,
					2.f
				);
#endif

				continue;
			}

			const float DistanceFromEnemy = FVector::Dist2D(CandidateLocation, EnemyLocation);

			/*
			 * 플레이어에게서 멀수록 높은 점수.
			 * Enemy의 이동 거리가 길수록 감점.
			 */
			const float Score = DistanceFromThreat - DistanceFromEnemy * 0.25f;

#if ENABLE_DRAW_DEBUG
			DrawDebugSphere(
				GetWorld(),
				CandidateLocation,
				25.f,
				8,
				FColor::Green,
				false,
				3.f
			);
#endif

			if (Score > BestScore)
			{
				BestScore = Score;
				BestLocation = CandidateLocation;
				bFoundLocation = true;
			}
		}
	}

	if (!bFoundLocation)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindHideLocation failed: no valid hide location found"));
		return false;
	}

	OutLocation = BestLocation;

#if ENABLE_DRAW_DEBUG
	DrawDebugSphere(
		GetWorld(),
		OutLocation,
		50.f,
		16,
		FColor::Blue,
		false,
		5.f,
		0,
		3.f
	);

	DrawDebugLine(
		GetWorld(),
		EnemyLocation,
		OutLocation,
		FColor::Blue,
		false,
		5.f,
		0,
		2.f
	);
#endif

	return true;
}

bool UEnemyStateMachineComponent::IsLocationHiddenFromActor(const FVector& Location, AActor* Observer) const
{
	if (!Observer || !GetWorld()) { return false; }

	FVector ViewLocation;
	FRotator ViewRotation;
	Observer->GetActorEyesViewPoint(ViewLocation, ViewRotation);

	/*
	 * 후보 지점의 발 위치가 아니라 Enemy 몸통 높이를 검사한다.
	 */
	const FVector CandidateViewLocation = Location + FVector(0.f, 0.f, 80.f);

	FHitResult HitResult;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(EnemyHideVisibility),
		false
	);

	QueryParams.AddIgnoredActor(Observer);

	if (Enemy)
	{
		QueryParams.AddIgnoredActor(Enemy);
	}

	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		ViewLocation,
		CandidateViewLocation,
		ECC_Visibility,
		QueryParams
	);

#if ENABLE_DRAW_DEBUG
	DrawDebugLine(
		GetWorld(),
		ViewLocation,
		CandidateViewLocation,
		bBlocked ? FColor::Green : FColor::Red,
		false,
		10.f,
		0,
		1.f
	);
#endif

	/*
	 * 중간에 벽이나 장애물이 맞았다면 해당 위치는 가려진 위치다.
	 */
	return bBlocked;
}

bool UEnemyStateMachineComponent::IsAtHideLocation() const
{
	if (!Enemy || !bHasValidHideLocation)
	{
		return false;
	}

	return FVector::Dist2D(
		Enemy->GetActorLocation(),
		HideLocation
	) <= HideAcceptanceRadius;
}

bool UEnemyStateMachineComponent::IsRecoveryComplete() const
{
	if (!HealthComponent)
	{
		return true;
	}

	return HealthComponent->GetCurrentHealth()
		>= RecoveryHealthCap - KINDA_SMALL_NUMBER;
}

bool UEnemyStateMachineComponent::HasMinimumHideTimeElapsed() const
{
	if (!GetWorld())
	{
		return false;
	}

	return GetWorld()->GetTimeSeconds() - HideStartTime
		>= MinimumHideTime;
}

bool UEnemyStateMachineComponent::HasMaximumHideTimeElapsed() const
{
	if (!GetWorld())
	{
		return false;
	}

	return GetWorld()->GetTimeSeconds() - HideStartTime
		>= MaximumHideTime;
}

void UEnemyStateMachineComponent::BeginFlee(AActor* ThreatActor)
{
	if (!Enemy || !HealthComponent) { return; }
	if (CurrentState == EEnemyStateType::Dead) { return; }

	if (ThreatActor) { TargetActor = ThreatActor; }

	if (!IsDefensiveState()) { StateBeforeFlee = CurrentState; }

	/*
	 * 최대 회복 가능 체력은 MaxHealth의 일정 비율이다.
	 * 예: 최대 체력 100, 비율 0.65 → 65까지만 회복 가능.
	 */
	RecoveryHealthCap = HealthComponent->GetMaxHealth() * RecoveryHealthCapPercent;

	/*
	 * 현재 체력이 이미 제한치보다 높다면 회복은 하지 않고
	 * 은신 시간만 가진 뒤 복귀한다.
	 */
	RecoveryHealthCap = FMath::Max(RecoveryHealthCap, HealthComponent->GetCurrentHealth());

	bHasValidHideLocation = FindHideLocation_Upgrade(TargetActor, HideLocation);

	SetState(EEnemyStateType::Flee);
}

void UEnemyStateMachineComponent::ReturnToPreviousBehavior()
{
	bHasValidHideLocation = false;
	AccumulatedBurstDamage = 0.f;

	/*
	 * 공격자가 아직 유효하고 시야에 있을 수 있지만,
	 * 회복 직후 바로 Attack으로 순간 전환하는 것보다는
	 * Chase부터 다시 시작하는 편이 안전하다.
	 */
	if (IsValid(TargetActor))
	{
		SetState(EEnemyStateType::Chase);
		return;
	}

	switch (StateBeforeFlee)
	{
	case EEnemyStateType::Patrol:
	case EEnemyStateType::Idle:
	case EEnemyStateType::Investigate:
		SetState(StateBeforeFlee);
		break;

	default:
		SetState(EEnemyStateType::Patrol);
		break;
	}
}

void UEnemyStateMachineComponent::StopChase()
{
	if (CurrentState == EEnemyStateType::BreachDoor)
	{
		// Sight memory timed out; stop treating the actor as a live target,
		// but allow the already-committed door strike to finish.
		ClearTarget();
		return;
	}
	ClearTarget();

	// The sight-loss timer must not cancel an ongoing impact dodge or flee.
	if (CurrentState != EEnemyStateType::Chase &&
		CurrentState != EEnemyStateType::Attack)
	{
		return;
	}

	if (HasRecentGunshotMemory())
	{
		SetLastHeardLocation(EstimatedGunshotLocation);
		SetState(EEnemyStateType::Investigate);
	}
	else if (bHasHeardLocation)
	{
		SetState(EEnemyStateType::Investigate);
	}
	else
	{
		SetState(PatrolPoints.Num() > 0 ? EEnemyStateType::Patrol : EEnemyStateType::Idle);
	}
}

void UEnemyStateMachineComponent::Task_MoveToTarget()
{
	if (!AIController || !HasTarget()) return;

	AIController->MoveToTarget(TargetActor);
}

void UEnemyStateMachineComponent::Task_AttackTarget()
{
	if (!Enemy || !HasTarget()) return;

	LastAttackTime = GetWorld()->GetTimeSeconds();

	//TODO: AnimNotify에서 해결하기
	if (EnemyAudioComponent)
	{
		EnemyAudioComponent->NotifyAttack();
	}

	Enemy->AttackTarget(TargetActor);
}

void UEnemyStateMachineComponent::Task_MoveToHideLocation()
{
	if (!AIController || !bHasValidHideLocation)
	{
		return;
	}

	AIController->MoveToLocationPoint(
		HideLocation,
		HideAcceptanceRadius
	);
}

void UEnemyStateMachineComponent::Task_StopMovement()
{
	if (!AIController) return;

	AIController->StopAIMovement();
}

void UEnemyStateMachineComponent::MoveToCurrentPatrolPoint()
{
	if (!AIController) return;
	if (PatrolPoints.Num() <= 0) return;
	if (!PatrolPoints.IsValidIndex(CurrentPatrolIndex)) return;

	AEnemyPatrolPoint* PatrolPoint = PatrolPoints[CurrentPatrolIndex];
	if (!PatrolPoint) return;

	AIController->MoveToLocationPoint(PatrolPoint->GetActorLocation());
}

void UEnemyStateMachineComponent::SelectNextPatrolPoint()
{
	UE_LOG(LogTemp, Warning, TEXT("UEnemyStateMachineComponent::SelectNextPatrolPoint()"));

	if (PatrolPoints.Num() <= 0) return;

	CurrentPatrolIndex++;

	if (CurrentPatrolIndex >= PatrolPoints.Num())
	{
		CurrentPatrolIndex = 0;
	}
}

void UEnemyStateMachineComponent::OnPatrolWaitFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("UEnemyStateMachineComponent::OnPatrolWaitFinished()"));

	bIsWaitingAtPatrolPoint = false;

	if (CurrentState != EEnemyStateType::Patrol)
	{
		return;
	}

	SelectNextPatrolPoint();
	MoveToCurrentPatrolPoint();
}

bool UEnemyStateMachineComponent::IsAtCurrentPatrolPoint() const
{
	if (!Enemy) return false;
	if (PatrolPoints.Num() <= 0) return false;
	if (!PatrolPoints.IsValidIndex(CurrentPatrolIndex)) return false;

	const AEnemyPatrolPoint* PatrolPoint = PatrolPoints[CurrentPatrolIndex];
	if (!PatrolPoint) return false;

	const float Distance = FVector::Dist(
		Enemy->GetActorLocation(),
		PatrolPoint->GetActorLocation()
	);

	return Distance <= PatrolAcceptanceRadius;
}

FVector UEnemyStateMachineComponent::GetCurrPatrolPointLocation() const
{
	if (PatrolPoints.Num() <= 0) return FVector();
	if (!PatrolPoints.IsValidIndex(CurrentPatrolIndex)) return FVector();

	const AEnemyPatrolPoint* PatrolPoint = PatrolPoints[CurrentPatrolIndex];
	if (!PatrolPoint) return FVector();
	return PatrolPoint->GetActorLocation();
}

void UEnemyStateMachineComponent::SetLastHeardLocation(const FVector& NewLocation)
{
	LastHeardLocation = NewLocation;
	bHasHeardLocation = true;
}

void UEnemyStateMachineComponent::UpdateInvestigate(float DeltaTime)
{
	if (HasTarget())
	{
		SetState(EEnemyStateType::Chase);
		return;
	}

	if (CanSearchForBlood())
	{
		LastBloodSearchTime = GetWorld()->GetTimeSeconds();
		if (TryAcquireBloodTrail()) { return; }
	}

	if (!bHasHeardLocation)
	{
		bHasGunshotMemory = false;
		SetState(PatrolPoints.Num() > 0 ? EEnemyStateType::Patrol : EEnemyStateType::Idle);
		return;
	}

	// An estimated gunshot location may be behind a visibly blocking weak door.
	// Never open/breach one for an arbitrary footstep or stale memory.
	if (bAllowGunshotInvestigationDoorBreach && HasRecentGunshotMemory() &&
		TryBeginDoorBreach(LastHeardLocation, EEnemyStateType::Investigate)) return;

	// Fail-safe: don't keep requesting a route forever if a location is unreachable.
	if (GetWorld() && GetWorld()->GetTimeSeconds() - InvestigateStartTime >
		InvestigateMaximumTravelTime)
	{
		bHasGunshotMemory = false;
		bHasHeardLocation = false;
		SetState(PatrolPoints.Num() > 0 ? EEnemyStateType::Patrol : EEnemyStateType::Idle);
		return;
	}

	if (bIsWaitingAtInvestigateLocation)
	{
		return;
	}

	if (IsAtLastHeardLocation())
	{
		Task_StopMovement();

		bIsWaitingAtInvestigateLocation = true;

		GetWorld()->GetTimerManager().SetTimer(
			InvestigateWaitTimerHandle,
			this,
			&UEnemyStateMachineComponent::OnInvestigateWaitFinished,
			InvestigateWaitTime,
			false
		);

		return;
	}

	FVector TargetLocation = Enemy->GetActorLocation() + Enemy->GetVelocity();
	RotateToTargetDirection(DeltaTime, TargetLocation);

	// Avoid issuing a new pathfinding request every single Tick.
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastInvestigateMoveTime >= 0.85f ||
		FVector::DistSquared2D(LastRequestedInvestigateLocation, LastHeardLocation) >=
		FMath::Square(GunshotRetargetDistance))
	{
		MoveToLastHeardLocation();
	}
}

void UEnemyStateMachineComponent::MoveToLastHeardLocation()
{
	if (!AIController || !bHasHeardLocation) return;

	AIController->MoveToLocationPoint_Upgrade(
		LastHeardLocation,
		InvestigateAcceptanceRadius
	);
	LastRequestedInvestigateLocation = LastHeardLocation;
	LastInvestigateMoveTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

bool UEnemyStateMachineComponent::IsAtLastHeardLocation() const
{
	if (!Enemy || !bHasHeardLocation) return false;

	const float Distance = FVector::Dist(
		Enemy->GetActorLocation(),
		LastHeardLocation
	);

	return Distance <= InvestigateAcceptanceRadius;
}

void UEnemyStateMachineComponent::OnInvestigateWaitFinished()
{
	bIsWaitingAtInvestigateLocation = false;
	bHasHeardLocation = false;

	if (CurrentState == EEnemyStateType::Investigate)
	{
		// This search is complete; do not restart it on a stale gunshot memory.
		bHasGunshotMemory = false;
		SetState(PatrolPoints.Num() > 0 ? EEnemyStateType::Patrol : EEnemyStateType::Idle);
	}
}

void UEnemyStateMachineComponent::NotifyDamageReceived(float DamageAmount, AActor* DamageInstigator)
{
	if (!Enemy || !HealthComponent || HealthComponent->IsDead())
	{
		return;
	}

	if (DamageAmount <= 0.f) { return; }
	if (DamageInstigator) { TargetActor = DamageInstigator; }

	if (EnemyAudioComponent)
	{
		EnemyAudioComponent->NotifyDamage(
			DamageAmount
		);
	}


	const float CurrentTime = GetWorld()->GetTimeSeconds();

	/*
	 * 기존 피해 누적 시간창이 끝났다면 새 시간창을 시작한다.
	 */
	if (CurrentTime > BurstDamageWindowEndTime)
	{
		AccumulatedBurstDamage = 0.f;
		BurstDamageWindowEndTime = CurrentTime + BurstDamageWindow;
	}

	AccumulatedBurstDamage += DamageAmount;

	if (ShouldFlee())
	{
		BeginFlee(DamageInstigator);
		return;
	}

	/*
	 * 도망 조건을 만족하지 않으면 기존처럼 공격자를 추적한다.
	 */
	if (!IsDefensiveState() && DamageInstigator)
	{
		UE_LOG(LogTemp, Error, TEXT("Flee Failed!!!"));

		SetTarget(DamageInstigator);
		SetState(EEnemyStateType::Chase);
	}
}

void UEnemyStateMachineComponent::RotateToTarget(float DeltaTime)
{
	if (!Enemy || !HasTarget()) return;

	FVector Direction = TargetActor->GetActorLocation() - Enemy->GetActorLocation();
	Direction.Z = 0.f;

	if (Direction.IsNearlyZero()) return;

	const FRotator TargetRotation = Direction.Rotation();

	const FRotator NewRotation = FMath::RInterpTo(
		Enemy->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		Enemy->AttackRotationSpeed
	);

	Enemy->SetActorRotation(NewRotation);
}

void UEnemyStateMachineComponent::RotateToTargetDirection(float DeltaTime, FVector TargetLocation)
{
	FVector Direction = TargetLocation - Enemy->GetActorLocation();
	Direction.Z = 0.f;

	if (Direction.IsNearlyZero()) return;

	const FRotator TargetRotation = Direction.Rotation();

	const FRotator NewRotation = FMath::RInterpTo(
		Enemy->GetActorRotation(),
		TargetRotation,
		DeltaTime,
		Enemy->AttackRotationSpeed
	);

	Enemy->SetActorRotation(NewRotation);
}

// -----------------------------------------------------------------------------
// Perceived combat noise behavior (range first gated by UAISense_Hearing).
// -----------------------------------------------------------------------------

bool UEnemyStateMachineComponent::TryHearCombatNoise(
	const FVector& NoiseLocation, float Loudness, float MaxRange,
	float& OutAudibility) const
{
	OutAudibility = 0.f;
	if (!IsValid(Enemy) || !GetWorld() || Enemy->IsDead() ||
		CurrentState == EEnemyStateType::Dead ||
		Loudness <= 0.f || MaxRange <= 0.f || CombatHearingRange <= 0.f)
	{
		return false;
	}

	// The enemy cannot hear infinitely far away, even if an event is very loud.
	const float EffectiveRange = FMath::Min(CombatHearingRange, MaxRange * Loudness);
	const FVector ListenerLocation = Enemy->GetActorLocation() + FVector(0.f, 0.f, 75.f);
	const float Distance = FVector::Dist(ListenerLocation, NoiseLocation);
	if (Distance > EffectiveRange)
	{
		return false;
	}

	OutAudibility = FMath::Clamp(1.f - Distance / FMath::Max(EffectiveRange, 1.f), 0.f, 1.f);

	// A wall attenuates the perceived strength; the exact propagation system can
	// later be replaced with portal/acoustic-propagation based attenuation.
	FHitResult BlockingHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyCombatSoundOcclusion), false);
	QueryParams.AddIgnoredActor(Enemy);

	if (GetWorld()->LineTraceSingleByChannel(
		BlockingHit, ListenerLocation, NoiseLocation, ECC_Visibility, QueryParams))
	{
		// Ignore a blocking hit at the source itself (e.g. a bullet hits the wall).
		if (FVector::DistSquared(BlockingHit.ImpactPoint, NoiseLocation) > FMath::Square(35.f))
		{
			OutAudibility *= FMath::Clamp(OccludedSoundMultiplier, 0.f, 1.f);
		}
	}

	return OutAudibility >= MinimumAudibility;
}

FVector UEnemyStateMachineComponent::EstimateGunshotLocation(
	const FVector& NoiseLocation, float Audibility) const
{
	if (!Enemy) { return NoiseLocation; }

	// Hearing provides bearing + a rough distance, never a perfect actor location.
	const FVector ListenerLocation = Enemy->GetActorLocation();
	const FVector ToSource = NoiseLocation - ListenerLocation;
	const float ApproxDistance = FMath::Max(ToSource.Size2D(), 1.f);
	const float ErrorRadius = FMath::Lerp(
		GunshotMaximumEstimateError,
		GunshotMinimumEstimateError,
		FMath::Clamp(Audibility, 0.f, 1.f));

	const FVector Direction = FVector(ToSource.X, ToSource.Y, 0.f).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		const FVector2D Offset = FMath::RandPointInCircle(ErrorRadius);
		return NoiseLocation + FVector(Offset.X, Offset.Y, 0.f);
	}

	// Both distance and bearing are perturbed. Gunshots occluded by walls will
	// naturally have a larger error because their audibility is lower.
	const float BearingErrorRadians = FMath::Atan2(ErrorRadius, ApproxDistance);
	const float Angle = FMath::FRandRange(-BearingErrorRadians, BearingErrorRadians);
	const FVector NoisyDirection = Direction.RotateAngleAxis(
		FMath::RadiansToDegrees(Angle), FVector::UpVector);
	const float NoisyDistance = FMath::Max(80.f,
		ApproxDistance + FMath::FRandRange(-ErrorRadius, ErrorRadius));

	FVector ApproxLocation = ListenerLocation + NoisyDirection * NoisyDistance;
	ApproxLocation.Z = NoiseLocation.Z;

	// Guns fire above the floor. Project the estimate onto NavMesh for MoveTo.
	if (UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation Projected;
		if (NavSystem->ProjectPointToNavigation(
			ApproxLocation, Projected, FVector(300.f, 300.f, 450.f)))
		{
			return Projected.Location;
		}
	}
	// MoveToLocationPoint_Upgrade handles fallback projection if available.
	return ApproxLocation;
}

bool UEnemyStateMachineComponent::HasRecentGunshotMemory() const
{
	return bHasGunshotMemory && GetWorld() &&
		(GetWorld()->GetTimeSeconds() - LastGunshotTime <= GunshotMemoryDuration);
}

void UEnemyStateMachineComponent::NotifyGunshotHeard(
	const FVector& SoundLocation, float Loudness, float MaxRange,
	const AActor* NoiseInstigator)
{
	float Audibility;
	if (!TryHearCombatNoise(SoundLocation, Loudness, MaxRange, Audibility))
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const FVector NewEstimate = EstimateGunshotLocation(SoundLocation, Audibility);

	// Consecutive shots refine the current approximate hypothesis. They do not
	// grant knowledge of Shooter or set TargetActor.
	if (HasRecentGunshotMemory())
	{
		const float BlendAlpha = 0.35f;
		EstimatedGunshotLocation = FMath::Lerp(EstimatedGunshotLocation, NewEstimate, BlendAlpha);
	}
	else
	{
		EstimatedGunshotLocation = NewEstimate;
	}
	bHasGunshotMemory = true;
	LastGunshotTime = CurrentTime;

	// Hearing alone does not reveal player position. But if sight already
	// identified the same actor, the heard gunshot signals incoming fire.
	if (bTargetCurrentlyVisible && HasTarget() && NoiseInstigator == TargetActor.Get())
	{
		RegisterCombatThreat(false);
	}

	if (IsDefensiveState() || CurrentState == EEnemyStateType::EvadeImpact ||
		CurrentState == EEnemyStateType::BreachDoor ||
		CurrentState == EEnemyStateType::Dead ||
		(HasTarget() && (CurrentState == EEnemyStateType::Chase ||
			CurrentState == EEnemyStateType::Attack)))
	{
		return;
	}

	const FVector PreviousInvestigation = LastHeardLocation;
	SetLastHeardLocation(EstimatedGunshotLocation);

	if (CurrentState != EEnemyStateType::Investigate)
	{
		LastGunshotMoveTime = CurrentTime;
		SetState(EEnemyStateType::Investigate);
		return;
	}

	const bool bDestinationChanged = FVector::DistSquared2D(
		PreviousInvestigation, LastHeardLocation) >= FMath::Square(GunshotRetargetDistance);
	const bool bMayRetarget =
		CurrentTime - LastGunshotMoveTime >= GunshotMoveRetargetInterval;

	if (bIsWaitingAtInvestigateLocation || (bDestinationChanged && bMayRetarget))
	{
		GetWorld()->GetTimerManager().ClearTimer(InvestigateWaitTimerHandle);
		bIsWaitingAtInvestigateLocation = false;
		LastGunshotMoveTime = CurrentTime;
		InvestigateStartTime = CurrentTime;
		MoveToLastHeardLocation();
	}
}

void UEnemyStateMachineComponent::RefineGunshotEstimateFromImpact(
	const FVector& ImpactLocation, const FVector& ProjectileTravelDirection)
{
	if (!HasRecentGunshotMemory() ||
		ProjectileTravelDirection.IsNearlyZero() ||
		GunshotImpactFusionWeight <= 0.f)
	{
		return;
	}

	// A bullet arriving at the impact point had approximately traveled along
	// +FlightDirection. This is optional: do NOT supply a bogus direction for
	// a grenade that has bounced several times, or for an inaudible bullet.
	const FVector ReverseFlight = -ProjectileTravelDirection.GetSafeNormal2D();
	const FVector ImpactToShot = EstimatedGunshotLocation - ImpactLocation;
	const float ProjectionLength = FVector::DotProduct(ImpactToShot, ReverseFlight);
	if (ProjectionLength <= 0.f) { return; }

	FVector Candidate = ImpactLocation + ReverseFlight * ProjectionLength;
	Candidate.Z = EstimatedGunshotLocation.Z;
	const float Correction = FVector::Dist2D(Candidate, EstimatedGunshotLocation);
	if (Correction > GunshotImpactFusionMaxCorrection) { return; }

	// Only correct the lateral estimate; keep distance uncertainty. Never
	// assign TargetActor or otherwise imply direct visual confirmation.
	EstimatedGunshotLocation = FMath::Lerp(
		EstimatedGunshotLocation, Candidate,
		FMath::Clamp(GunshotImpactFusionWeight, 0.f, 1.f));
}

void UEnemyStateMachineComponent::NotifyProjectileImpactHeard(
	const FVector& ImpactLocation,
	const FVector& ProjectileTravelDirection,
	float Loudness,
	float MaxRange,
	bool bMayInferShooterFromDirection,
	const AActor* NoiseInstigator)
{
	float Audibility = 0.f;
	if (!TryHearCombatNoise(ImpactLocation, Loudness, MaxRange, Audibility) ||
		!Enemy || IsDefensiveState())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastImpactTime < ImpactNoiseCooldown &&
		FVector::DistSquared2D(ImpactLocation, LastImpactLocation) < FMath::Square(300.f))
	{
		return; // Ignore repeated collision callbacks / same surface impact.
	}

	LastImpactLocation = ImpactLocation;
	LastImpactTravelDirection = ProjectileTravelDirection.GetSafeNormal2D();
	LastImpactTime = CurrentTime;

	// Travel direction is useful ONLY for an unbounced bullet. In particular,
	// grenade bounce direction is never used as the original thrower's bearing.
	if (bMayInferShooterFromDirection && !LastImpactTravelDirection.IsNearlyZero())
	{
		if (HasRecentGunshotMemory())
		{
			RefineGunshotEstimateFromImpact(ImpactLocation, LastImpactTravelDirection);
		}
		else
		{
			// Hearing the hit alone cannot reveal distance to the shooter:
			// choose an uncertain point along the reverse trajectory, not the
			// exact owner actor position (which the stimulus might expose).
			const float MinEstimate = FMath::Max(100.f, ImpactOnlyMinimumSourceEstimateDistance);
			const float MaxEstimate = FMath::Max(MinEstimate, ImpactOnlyMaximumSourceEstimateDistance);
			const float GuessDistance = FMath::FRandRange(MinEstimate, MaxEstimate);
			const FVector GuessedSource = ImpactLocation - LastImpactTravelDirection * GuessDistance;
			EstimatedGunshotLocation = EstimateGunshotLocation(GuessedSource, Audibility);
			bHasGunshotMemory = true;
		}
		LastGunshotTime = CurrentTime;

		if (CurrentState == EEnemyStateType::Investigate)
		{
			SetLastHeardLocation(EstimatedGunshotLocation);
		}
		else if (CurrentState == EEnemyStateType::Idle ||
			CurrentState == EEnemyStateType::Patrol ||
			CurrentState == EEnemyStateType::TrackBlood)
		{
			SetLastHeardLocation(EstimatedGunshotLocation);
		}
	}

	// A distant impact may refine an investigation but is NOT a reason to dodge.
	const float Distance = FVector::Dist2D(Enemy->GetActorLocation(), ImpactLocation);

	// Protect door breaching from frequent noise interruptions. Exception:
	// a dangerous nearby impact still requires immediate evasion.
	if (CurrentState == EEnemyStateType::BreachDoor)
	{
		if (Distance <= FMath::Min(ImpactReactionRadius, CombatDodgeHazardRadius))
		{
			StateBeforeImpactEvade = EEnemyStateType::BreachDoor;
			SetState(EEnemyStateType::EvadeImpact);
		}
		return;
	}

	// Sight-confirmed target: stay in Chase, dodge AWAY from the fresh impact,
	// and approach the player in a zigzag. Health-based Flee remains higher
	// priority and is already filtered at the start of this function.
	if (bTargetCurrentlyVisible && HasTarget() &&
		NoiseInstigator != Enemy.Get() && Distance <= ImpactReactionRadius &&
		(CurrentState == EEnemyStateType::Chase ||
			CurrentState == EEnemyStateType::Attack ||
			CurrentState == EEnemyStateType::Investigate ||
			CurrentState == EEnemyStateType::EvadeImpact))
	{
		// When the source is identifiable, do not treat a different player's
		// non-threatening gunshot as the current target's aim. A nearby impact
		// is nevertheless an immediate positional hazard, whoever fired it.
		RegisterCombatThreat(true);
		if (CurrentState != EEnemyStateType::Chase &&
			(!IsTargetInAttackRange() || Distance <= CombatDodgeHazardRadius))
		{
			SetState(EEnemyStateType::Chase);
		}
		return;
	}

	if (Distance > ImpactReactionRadius)
	{
		if (bHasGunshotMemory &&
			(CurrentState == EEnemyStateType::Idle ||
				CurrentState == EEnemyStateType::Patrol ||
				CurrentState == EEnemyStateType::TrackBlood))
		{
			SetState(EEnemyStateType::Investigate);
		}
		return;
	}

	if (CurrentState == EEnemyStateType::EvadeImpact)
	{
		ImpactEvadeStartTime = CurrentTime;
		bHasImpactEvadeLocation = FindImpactEvadeLocation(ImpactEvadeLocation);
		if (bHasImpactEvadeLocation) MoveToImpactEvadeLocation();
		return;
	}

	StateBeforeImpactEvade = CurrentState;
	SetState(EEnemyStateType::EvadeImpact);
}

bool UEnemyStateMachineComponent::FindImpactEvadeLocation(FVector& OutLocation) const
{
	if (!Enemy || !GetWorld()) { return false; }
	UNavigationSystemV1* Navigation = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Navigation) { return false; }

	const FVector EnemyLocation = Enemy->GetActorLocation();
	const float CurrentDistanceFromImpact = FVector::Dist2D(EnemyLocation, LastImpactLocation);
	FVector Away = EnemyLocation - LastImpactLocation;
	Away.Z = 0.f;

	if (!Away.Normalize())
	{
		// Enemy is virtually on the impact: dodge perpendicular to the
		// incoming projectile direction if available.
		Away = FVector::CrossProduct(LastImpactTravelDirection, FVector::UpVector);
		if (Away.IsNearlyZero())
		{
			Away = Enemy->GetActorRightVector();
		}
		Away.Z = 0.f;
		Away.Normalize();
	}

	bool bFound = false;
	float BestScore = -BIG_NUMBER;
	FVector BestCandidate = FVector::ZeroVector;

	// Random reachable points are generated from the ENEMY's navmesh island,
	// rather than merely projecting a potentially unreachable desired point.
	for (int32 Index = 0; Index < FMath::Max(1, ImpactEvadeSampleCount); ++Index)
	{
		FNavLocation Candidate;
		if (!Navigation->GetRandomReachablePointInRadius(
			EnemyLocation, ImpactEvadeSearchRadius, Candidate))
		{
			continue;
		}

		const FVector Position = Candidate.Location;
		const float DistanceFromImpact = FVector::Dist2D(Position, LastImpactLocation);
		if (DistanceFromImpact <= CurrentDistanceFromImpact + 80.f)
		{
			continue;
		}

		const FVector ToCandidate = (Position - EnemyLocation).GetSafeNormal2D();
		const float DirectionAlignment = FVector::DotProduct(ToCandidate, Away);
		const float TravelDistance = FVector::Dist2D(Position, EnemyLocation);
		const float SafetyBonus = DistanceFromImpact >= ImpactSafeDistance ? 500.f : 0.f;

		const float Score = DistanceFromImpact * 1.5f
			+ DirectionAlignment * 300.f
			- TravelDistance * 0.2f + SafetyBonus;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestCandidate = Position;
			bFound = true;
		}
	}

	if (bFound)
	{
		OutLocation = BestCandidate;
	}
	return bFound;
}

void UEnemyStateMachineComponent::MoveToImpactEvadeLocation()
{
	if (!Enemy || !bHasImpactEvadeLocation) { return; }
	if (!IsValid(AIController))
	{
		AIController = Cast<AEnemyBaseAIController>(Enemy->GetController());
	}
	if (!AIController) { return; }

	// Reuse the project's existing ground-adjusting movement API.
	AIController->MoveToLocationPoint_Upgrade(
		ImpactEvadeLocation, ImpactEvadeAcceptanceRadius);
	LastImpactMoveTime = GetWorld()->GetTimeSeconds();
}

void UEnemyStateMachineComponent::UpdateEvadeImpact(float DeltaTime)
{
	if (!Enemy || !GetWorld()) { return; }
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float ElapsedTime = CurrentTime - ImpactEvadeStartTime;
	const float DistanceFromImpact = FVector::Dist2D(
		Enemy->GetActorLocation(), LastImpactLocation);

	if (ElapsedTime >= ImpactEvadeMaximumTime ||
		(ElapsedTime >= ImpactEvadeMinimumTime &&
			DistanceFromImpact >= ImpactSafeDistance))
	{
		FinishImpactEvasion();
		return;
	}

	if (bHasImpactEvadeLocation)
	{
		const bool bAtDestination = FVector::DistSquared2D(
			Enemy->GetActorLocation(), ImpactEvadeLocation) <=
			FMath::Square(ImpactEvadeAcceptanceRadius);

		if (bAtDestination && CurrentTime - LastImpactRepathTime >= 0.65f)
		{
			LastImpactRepathTime = CurrentTime;
			bHasImpactEvadeLocation = FindImpactEvadeLocation(ImpactEvadeLocation);
			if (bHasImpactEvadeLocation)
			{
				MoveToImpactEvadeLocation();
			}
		}
		else if (!bAtDestination && CurrentTime - LastImpactMoveTime >= 1.0f)
		{
			MoveToImpactEvadeLocation();
		}
	}
	else if (CurrentTime - LastImpactRepathTime >= 0.65f)
	{
		LastImpactRepathTime = CurrentTime;
		bHasImpactEvadeLocation = FindImpactEvadeLocation(ImpactEvadeLocation);
		if (bHasImpactEvadeLocation)
		{
			MoveToImpactEvadeLocation();
		}
	}

	if (Enemy->GetVelocity().SizeSquared2D() > FMath::Square(15.f))
	{
		RotateToTargetDirection(DeltaTime,
			Enemy->GetActorLocation() + Enemy->GetVelocity());
	}
}

void UEnemyStateMachineComponent::FinishImpactEvasion()
{
	Task_StopMovement();
	bHasImpactEvadeLocation = false;

	if (HasTarget())
	{
		SetState(EEnemyStateType::Chase);
		return;
	}

	// Prefer a previously HEARD gunshot over the impact point; going to the
	// impact point would directly contradict the requested evasive behavior.
	if (HasRecentGunshotMemory() &&
		FVector::Dist2D(EstimatedGunshotLocation, LastImpactLocation) >=
		ImpactSafeDistance * 0.7f)
	{
		SetLastHeardLocation(EstimatedGunshotLocation);
		SetState(EEnemyStateType::Investigate);
		return;
	}

	if (StateBeforeImpactEvade == EEnemyStateType::TrackBlood &&
		IsBloodStainValid(CurrentBloodStain))
	{
		SetState(EEnemyStateType::TrackBlood);
		return;
	}

	if (StateBeforeImpactEvade == EEnemyStateType::Investigate &&
		bHasHeardLocation &&
		FVector::Dist2D(LastHeardLocation, LastImpactLocation) >=
		ImpactSafeDistance * 0.7f)
	{
		SetState(EEnemyStateType::Investigate);
		return;
	}

	bHasGunshotMemory = false;
	bHasHeardLocation = false;
	SetState(PatrolPoints.Num() > 0 ? EEnemyStateType::Patrol : EEnemyStateType::Idle);
}

// -----------------------------------------------------------------------------
// Combat movement: zigzag while CHASING a visually confirmed Player.
// All expensive navigation work happens on a bounded, time-based cadence.
// -----------------------------------------------------------------------------

void UEnemyStateMachineComponent::RegisterCombatThreat(bool bUrgent)
{
	if (!GetWorld() || IsDefensiveState() || CurrentState == EEnemyStateType::Dead)
	{
		return;
	}

	const bool bWasActive = IsCombatDodgeActive();
	LastCombatThreatTime = GetWorld()->GetTimeSeconds();

	// New attack or nearby impact gets a fresh waypoint; automatic rifle fire
	// only refreshes memory, it does not force navigation 10+ times per second.
	if (bUrgent || !bWasActive)
	{
		bForceCombatDodgeRepath = true;
	}

	if (bUrgent && Enemy && HasTarget() && IsRecentImpactForDodge())
	{
		const FVector ToTarget = (TargetActor->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
		const FVector Right(-ToTarget.Y, ToTarget.X, 0.f);
		const FVector Away = (Enemy->GetActorLocation() - LastImpactLocation).GetSafeNormal2D();
		const float SideDot = FVector::DotProduct(Away, Right);
		if (FMath::Abs(SideDot) > 0.15f)
		{
			CombatDodgeSide = SideDot >= 0.f ? +1 : -1;
		}
		else if (!LastImpactTravelDirection.IsNearlyZero())
		{
			// For a near-center impact, move perpendicular to incoming bullets.
			const FVector Perpendicular(-LastImpactTravelDirection.Y, LastImpactTravelDirection.X, 0.f);
			CombatDodgeSide = FVector::DotProduct(Perpendicular, Right) >= 0.f ? +1 : -1;
		}
		LastCombatDodgeSideChangeTime = GetWorld()->GetTimeSeconds();
	}
}

bool UEnemyStateMachineComponent::IsRecentImpactForDodge() const
{
	return GetWorld() && (GetWorld()->GetTimeSeconds() - LastImpactTime <= CombatDodgeImpactMemory);
}

bool UEnemyStateMachineComponent::IsCombatDodgeActive() const
{
	return bEnableCombatZigzag && GetWorld() && Enemy && HasTarget() &&
		bTargetCurrentlyVisible && !IsDefensiveState() &&
		GetWorld()->GetTimeSeconds() - LastCombatThreatTime <= CombatDodgeThreatMemory;
}

void UEnemyStateMachineComponent::ResetCombatDodgeMovement()
{
	bHasCombatDodgeWaypoint = false;
	bForceCombatDodgeRepath = false;
	CombatDodgeWaypoint = FVector::ZeroVector;
}

bool UEnemyStateMachineComponent::TryFindCombatDodgeWaypoint(FVector& OutWaypoint) const
{
	if (!IsValid(Enemy) || !HasTarget() || !GetWorld()) return false;
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Nav) return false;

	const FVector Start = Enemy->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector Forward = (TargetLocation - Start).GetSafeNormal2D();
	if (Forward.IsNearlyZero()) return false;
	const FVector Right(-Forward.Y, Forward.X, 0.f);

	const float DistanceToTarget = FVector::Dist2D(Start, TargetLocation);
	const float Advance = FMath::Clamp(
		DistanceToTarget - Enemy->AttackRange * 0.8f,
		100.f, CombatDodgeForwardStep);
	const bool bImpactRecent = IsRecentImpactForDodge();
	const float ImpactDistance = bImpactRecent
		? FVector::Dist2D(Start, LastImpactLocation) : BIG_NUMBER;
	const bool bInsideDanger = bImpactRecent && ImpactDistance < CombatDodgeHazardRadius;
	const FVector AwayFromImpact = bImpactRecent
		? (Start - LastImpactLocation).GetSafeNormal2D() : FVector::ZeroVector;

	// Two lateral distances per side; prefer the alternating side, but permit
	// the other one if the preferred side is blocked or close to an impact.
	constexpr float WidthScales[] = { 1.0f, 0.65f, 1.30f };
	float BestScore = -BIG_NUMBER;
	bool bFound = false;

	for (int32 SideAttempt = 0; SideAttempt < 2; ++SideAttempt)
	{
		const int32 Side = SideAttempt == 0 ? CombatDodgeSide : -CombatDodgeSide;
		for (float WidthScale : WidthScales)
		{
			FVector Desired = Start + Forward * Advance +
				Right * (Side * CombatDodgeLateralStep * WidthScale);

			if (bImpactRecent)
			{
				const float ImpactAlpha = FMath::Clamp(
					1.f - ImpactDistance / FMath::Max(ImpactReactionRadius, 1.f), 0.f, 1.f);
				Desired += AwayFromImpact * CombatDodgeImpactAwayBias * ImpactAlpha;
			}

			FNavLocation NavLocation;
			if (!Nav->ProjectPointToNavigation(Desired, NavLocation, FVector(220.f, 220.f, 350.f)))
			{
				continue;
			}

			const FVector Candidate = NavLocation.Location;
			const float TravelDistance = FVector::Dist2D(Start, Candidate);
			if (TravelDistance < CombatDodgeWaypointAcceptance * 1.3f) continue;

			const float CandidateTargetDistance = FVector::Dist2D(Candidate, TargetLocation);
			const float Progress = DistanceToTarget - CandidateTargetDistance;
			// Keep approaching the player. The exception is an emergency
			// dodge when the Enemy is already within the impact danger radius.
			if (!bInsideDanger && Progress < 55.f) continue;
			if (bInsideDanger && Progress < -220.f) continue;

			// Projection alone does not guarantee reachable destinations.
			// Check an actual non-partial NavMesh route before issuing MoveTo.
			UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
				GetWorld(), Start, Candidate, Enemy);
			if (!IsValid(Path) || !Path->IsValid() || Path->IsPartial()) continue;

			float Score = Progress * 1.5f + (Side == CombatDodgeSide ? 180.f : 0.f)
				- TravelDistance * 0.1f;

			if (bImpactRecent)
			{
				const float CandidateDistanceFromImpact = FVector::Dist2D(Candidate, LastImpactLocation);
				Score += (CandidateDistanceFromImpact - ImpactDistance) * 1.45f;

				// Inspect all actual nav path segments; a destination that is
				// safe but whose path walks through impact is NOT desirable.
				float ClosestPathDistance = BIG_NUMBER;
				const TArray<FVector>& Points = Path->PathPoints;
				for (int32 Index = 1; Index < Points.Num(); ++Index)
				{
					const FVector Segment = Points[Index] - Points[Index - 1];
					const FVector ImpactOffset = LastImpactLocation - Points[Index - 1];
					const FVector2D SegmentXY(Segment.X, Segment.Y);
					const FVector2D OffsetXY(ImpactOffset.X, ImpactOffset.Y);
					const float SegLengthSquared = SegmentXY.SizeSquared();
					const float T = SegLengthSquared > KINDA_SMALL_NUMBER
						? FMath::Clamp(static_cast<float>(FVector2D::DotProduct(OffsetXY, SegmentXY) / SegLengthSquared), 0.f, 1.f)
						: 0.f;
					const FVector Closest = Points[Index - 1] + Segment * T;
					ClosestPathDistance = FMath::Min(ClosestPathDistance,
						static_cast<float>(FVector::Dist2D(Closest, LastImpactLocation)));
				}
				if (ClosestPathDistance < CombatDodgeHazardRadius)
				{
					// When starting inside the hazard every route includes a
					// dangerous initial segment; penalize instead of rejecting.
					Score -= (CombatDodgeHazardRadius - ClosestPathDistance) *
						(bInsideDanger ? 0.45f : 2.0f);
				}

				// An unbounced bullet's line suggests a likely firing lane.
				// It is a soft cue, not a perfect/clairvoyant bullet predictor.
				if (!LastImpactTravelDirection.IsNearlyZero())
				{
					const FVector LaneDirection = LastImpactTravelDirection.GetSafeNormal2D();
					const FVector ImpactToCandidate = Candidate - LastImpactLocation;
					const float LateralDist = FMath::Abs(
						FVector::CrossProduct(LaneDirection, ImpactToCandidate).Z);
					Score += FMath::Min(LateralDist, 700.f) * 0.15f;
				}
			}

			if (!bFound || Score > BestScore)
			{
				BestScore = Score;
				OutWaypoint = Candidate;
				bFound = true;
			}
		}
	}
	return bFound;
}

bool UEnemyStateMachineComponent::UpdateCombatDodgeChase()
{
	if (!IsCombatDodgeActive() || !IsValid(AIController))
	{
		// If the threat expired, let normal Chase refresh the MoveToActor path.
		ResetCombatDodgeMovement();
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const FVector CurrentLocation = Enemy->GetActorLocation();
	const bool bReachedWaypoint = bHasCombatDodgeWaypoint &&
		FVector::DistSquared2D(CurrentLocation, CombatDodgeWaypoint) <=
		FMath::Square(CombatDodgeWaypointAcceptance);
	const bool bPlayerMoved = bHasCombatDodgeWaypoint &&
		FVector::DistSquared2D(TargetActor->GetActorLocation(), CombatDodgeTargetAtPathTime) >=
		FMath::Square(CombatDodgeTargetMoveRepathDistance);
	const bool bSideDue = (Now - LastCombatDodgeSideChangeTime >= CombatDodgeSideSwitchInterval);
	const bool bPathTooOld = Now - LastCombatDodgePathTime >= CombatDodgeMaximumRepathInterval;
	const bool bNeedWaypoint = !bHasCombatDodgeWaypoint || bReachedWaypoint ||
		bPlayerMoved || bSideDue || bPathTooOld || bForceCombatDodgeRepath;

	if (bNeedWaypoint &&
		(Now - LastCombatDodgePathTime >= CombatDodgeMinimumRepathInterval))
	{
		if (LastCombatDodgeSideChangeTime < -100000.f)
		{
			LastCombatDodgeSideChangeTime = Now; // Keep preferred side on first move.
		}
		else if (bSideDue || bReachedWaypoint)
		{
			CombatDodgeSide *= -1;
			LastCombatDodgeSideChangeTime = Now;
		}

		LastCombatDodgePathTime = Now;
		bForceCombatDodgeRepath = false;

		FVector NewWaypoint;
		if (TryFindCombatDodgeWaypoint(NewWaypoint))
		{
			CombatDodgeWaypoint = NewWaypoint;
			CombatDodgeTargetAtPathTime = TargetActor->GetActorLocation();
			bHasCombatDodgeWaypoint = AIController->MoveToCombatDodgeLocation(
				CombatDodgeWaypoint, CombatDodgeWaypointAcceptance);
		}
		else
		{
			bHasCombatDodgeWaypoint = false;
		}
	}

	// When we have a valid route, do NOT call MoveToActor every frame;
	// it would overwrite our lateral waypoint path.
	return bHasCombatDodgeWaypoint;
}

void UEnemyStateMachineComponent::DrawDebug() const
{
	if (!Enemy)
	{
		return;
	}

	const UEnum* Enum = StaticEnum<EEnemyStateType>();

	const FString StateString = Enum
		? Enum->GetNameStringByValue(static_cast<int64>(CurrentState))
		: TEXT("Unknown");

	const FVector BaseLocation = Enemy->GetActorLocation() + FVector(0.f, 0.f, 140.f);

	DrawDebugString(
		GetWorld(),
		BaseLocation,
		FString::Printf(TEXT("State : %s"), *StateString),
		nullptr,
		FColor::Cyan,
		0.f,
		true
	);

	if (HealthComponent)
	{
		DrawDebugString(
			GetWorld(),
			BaseLocation + FVector(0.f, 0.f, -15.f),
			FString::Printf(
				TEXT("HP : %.0f / %.0f"),
				HealthComponent->GetCurrentHealth(),
				HealthComponent->GetMaxHealth()
			),
			nullptr,
			FColor::Green,
			0.f,
			true
		);
	}

	if (StaminaComponent)
	{
		DrawDebugString(
			GetWorld(),
			BaseLocation + FVector(0.f, 0.f, -30.f),
			FString::Printf(
				TEXT("ST : %.0f / %.0f"),
				StaminaComponent->GetCurrentStamina(),
				StaminaComponent->GetMaxStamina()
			),
			nullptr,
			FColor::Yellow,
			0.f,
			true
		);
	}

	if (HasRecentGunshotMemory())
	{
		DrawDebugSphere(GetWorld(), EstimatedGunshotLocation, 35.f, 10,
			FColor::Cyan, false, 0.f);
	}
	if (CurrentState == EEnemyStateType::EvadeImpact)
	{
		DrawDebugSphere(GetWorld(), LastImpactLocation, 55.f, 12,
			FColor::Red, false, 0.f);
		if (bHasImpactEvadeLocation)
		{
			DrawDebugSphere(GetWorld(), ImpactEvadeLocation, 45.f, 10,
				FColor::Green, false, 0.f);
			DrawDebugLine(GetWorld(), Enemy->GetActorLocation(),
				ImpactEvadeLocation, FColor::Green, false, 0.f);
		}
	}
#if ENABLE_DRAW_DEBUG
	if (bDrawCombatDodgeDebug && GetWorld() && bHasCombatDodgeWaypoint &&
		CurrentState == EEnemyStateType::Chase && IsCombatDodgeActive())
	{
		const FVector From = Enemy->GetActorLocation() + FVector(0.f, 0.f, 35.f);
		const FVector To = CombatDodgeWaypoint + FVector(0.f, 0.f, 35.f);
		DrawDebugLine(GetWorld(), From, To, FColor::Cyan, false, 0.f, 0, 3.f);
		DrawDebugSphere(GetWorld(), To, 32.f, 8, FColor::Cyan, false, 0.f);
		if (IsRecentImpactForDodge())
		{
			DrawDebugSphere(GetWorld(), LastImpactLocation, CombatDodgeHazardRadius,
				16, FColor::Orange, false, 0.f, 0, 1.f);
		}
	}
#endif

}

void UEnemyStateMachineComponent::UpdateTrackBlood(float DeltaTime)
{
	if (!Enemy || !AIController) { return; }

	/*
	 * 실제 Player를 시야로 발견하면 혈흔 추적보다
	 * 직접 추격을 우선한다.
	 */
	if (HasTarget())
	{
		ClearBloodTrail();
		SetState(EEnemyStateType::Chase);
		return;
	}

	if (!IsBloodStainValid(CurrentBloodStain))
	{
		ReturnFromBloodTracking();
		return;
	}

	FVector TargetLocation = Enemy->GetActorLocation() + Enemy->GetVelocity();
	RotateToTargetDirection(DeltaTime, TargetLocation);

	if (!IsAtCurrentBloodStain())
	{
		AIController->MoveToLocationPoint_Upgrade(CurrentBloodStain->GetActorLocation(), BloodTrackAcceptanceRadius - 50.f);
		return;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Is at Curr Blood Stain!!!"));
	}

	ABloodStainActor* NextBloodStain = nullptr;

	if (FindNextBloodStain(NextBloodStain))
	{
		UE_LOG(LogTemp, Error, TEXT("Next Blood Stain is Found!!!"));

		SetCurrentBloodStain(NextBloodStain);

		AIController->MoveToLocationPoint_Upgrade(CurrentBloodStain->GetActorLocation(), BloodTrackAcceptanceRadius);

		return;
	}

	/*
	 * 아직 다음 혈흔이 생성되지 않았을 수 있으므로
	 * 마지막 혈흔에서 잠시 기다린다.
	 */
	if (BloodTrailLostStartTime <= 0.f)
	{
		BloodTrailLostStartTime = GetWorld()->GetTimeSeconds();
		Task_StopMovement();
		return;
	}

	const float LostDuration = GetWorld()->GetTimeSeconds() - BloodTrailLostStartTime;

	if (LostDuration >= BloodTrailLostWaitTime)
	{
		ReturnFromBloodTracking();
	}
}

bool UEnemyStateMachineComponent::CanSearchForBlood() const
{
	if (!GetWorld()) { return false; }

	const bool bPassiveState =
		CurrentState == EEnemyStateType::Idle ||
		CurrentState == EEnemyStateType::Patrol ||
		CurrentState == EEnemyStateType::Investigate;

	if (!bPassiveState) { return false; }

	return GetWorld()->GetTimeSeconds() - LastBloodSearchTime >= BloodSearchInterval;
}

bool UEnemyStateMachineComponent::TryAcquireBloodTrail()
{
	ABloodStainActor* FoundBloodStain = nullptr;
	if (!FindInitialBloodStain(FoundBloodStain)) { return false; }
	SetCurrentBloodStain(FoundBloodStain);
	SetState(EEnemyStateType::TrackBlood);
	return true;
}

bool UEnemyStateMachineComponent::FindInitialBloodStain(ABloodStainActor*& OutBloodStain) const
{
	OutBloodStain = nullptr;
	if (!Enemy || !GetWorld()) { return false; }

	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (TActorIterator<ABloodStainActor> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		ABloodStainActor* BloodStain = *Iterator;

		if (!IsBloodStainValid(BloodStain)) { continue; }

		AActor* TrailOwner = BloodStain->GetTrailOwner();

		/*
		 * Enemy 자신 또는 다른 Enemy가 남긴 혈흔은
		 * 필요에 따라 제외한다.
		 *
		 * 여기서는 Player 혈흔 여부를 프로젝트의
		 * Tag 또는 Class로 판별한다고 가정한다.
		 */
		if (!TrailOwner || !TrailOwner->ActorHasTag(TEXT("Player")))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(Enemy->GetActorLocation(), BloodStain->GetActorLocation());

		if (DistanceSquared > FMath::Square(BloodDetectionRadius))
		{
			continue;
		}

		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			OutBloodStain = BloodStain;
		}
	}

	return OutBloodStain != nullptr;
}

bool UEnemyStateMachineComponent::FindNextBloodStain(ABloodStainActor*& OutBloodStain) const
{
	UE_LOG(LogTemp, Error, TEXT("bool UEnemyStateMachineComponent::FindNextBloodStain(ABloodStainActor*& OutBloodStain) const"));

	OutBloodStain = nullptr;
	if (!CurrentBloodStain || !TrackedBloodOwner.IsValid() || !GetWorld())
	{
		return false;
	}

	int32 BestSequenceIndex = MAX_int32;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (TActorIterator<ABloodStainActor> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		UE_LOG(LogTemp, Error, TEXT("for (TActorIterator<ABloodStainActor> Iterator(GetWorld()); Iterator; ++Iterator)"));

		ABloodStainActor* Candidate = *Iterator;

		if (!IsBloodStainValid(Candidate)) { continue; }

		if (Candidate == CurrentBloodStain) { continue; }

		if (Candidate->GetTrailOwner() != TrackedBloodOwner.Get())
		{
			continue;
		}

		const int32 CandidateSequence = Candidate->GetSequenceIndex();

		if (CandidateSequence <= CurrentBloodSequenceIndex)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(CurrentBloodStain->GetActorLocation(), Candidate->GetActorLocation());

		if (DistanceSquared > FMath::Square(BloodTrailLinkRadius))
		{
			continue;
		}

		/*
		 * 순서가 가장 가까운 다음 혈흔을 우선한다.
		 * 같은 Sequence라면 거리가 가까운 것을 선택한다.
		 */
		const bool bBetterSequence = CandidateSequence < BestSequenceIndex;

		const bool bSameSequenceButCloser =
			CandidateSequence == BestSequenceIndex &&
			DistanceSquared < BestDistanceSquared;

		if (bBetterSequence || bSameSequenceButCloser)
		{
			BestSequenceIndex = CandidateSequence;
			BestDistanceSquared = DistanceSquared;
			OutBloodStain = Candidate;
		}
	}

	return OutBloodStain != nullptr;
}

bool UEnemyStateMachineComponent::IsBloodStainValid(const ABloodStainActor* BloodStain) const
{
	if (!IsValid(BloodStain) || !GetWorld()) { return false; }
	const float BloodAge = GetWorld()->GetTimeSeconds() - BloodStain->GetBloodSpawnTime();
	return BloodAge <= MaximumTrackableBloodAge;
}

bool UEnemyStateMachineComponent::IsAtCurrentBloodStain() const
{
	if (!Enemy || !CurrentBloodStain)
	{
		return false;
	}

	return FVector::DistSquared2D(Enemy->GetActorLocation(), CurrentBloodStain->GetActorLocation()) <= FMath::Square(BloodTrackAcceptanceRadius);
}

void UEnemyStateMachineComponent::SetCurrentBloodStain(ABloodStainActor* NewBloodStain)
{
	CurrentBloodStain = NewBloodStain;
	if (!CurrentBloodStain) { return; }

	TrackedBloodOwner = CurrentBloodStain->GetTrailOwner();
	CurrentBloodSequenceIndex = CurrentBloodStain->GetSequenceIndex();
	BloodTrailLostStartTime = 0.f;
}

void UEnemyStateMachineComponent::ClearBloodTrail()
{
	CurrentBloodStain = nullptr;
	TrackedBloodOwner.Reset();
	CurrentBloodSequenceIndex = INDEX_NONE;
	BloodTrailLostStartTime = 0.f;
}

void UEnemyStateMachineComponent::ReturnFromBloodTracking()
{
	FVector LastBloodLocation = FVector::ZeroVector;

	if (CurrentBloodStain)
	{
		LastBloodLocation = CurrentBloodStain->GetActorLocation();
	}

	ClearBloodTrail();

	if (!LastBloodLocation.IsNearlyZero())
	{
		SetLastHeardLocation(LastBloodLocation);
		SetState(EEnemyStateType::Investigate);
	}
	else
	{
		SetState(EEnemyStateType::Patrol);
	}
}
