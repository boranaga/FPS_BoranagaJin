#include "Characters/Enemies/EnemyStateMachineComponent.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyPatrolPoint.h"
#include "Characters/HealthComponent.h"
#include "Characters/StaminaComponent.h"

#include "NavigationSystem.h"
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


		if (HealthComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("HealthComponent"));
		}
		if (StaminaComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("StaminaComponent"));
		}
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

	UpdateState(DeltaTime);
}

void UEnemyStateMachineComponent::OnIdleFinished()
{
	// TODO: 일단 무조건 Patrol인데, 조건 추가해야할듯
	SetState(EEnemyStateType::Patrol);
}

void UEnemyStateMachineComponent::SetTarget(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void UEnemyStateMachineComponent::ClearTarget()
{
	TargetActor = nullptr;
}

void UEnemyStateMachineComponent::SetState(EEnemyStateType NewState)
{
	if (CurrentState == NewState) return;

	ExitState(CurrentState);
	CurrentState = NewState;
	EnterState(CurrentState);
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
		break;

	case EEnemyStateType::Patrol:
		UE_LOG(LogTemp, Warning, TEXT("Enter Patrol"));
		bIsWaitingAtPatrolPoint = false;
		MoveToCurrentPatrolPoint();
		break;

	case EEnemyStateType::Chase:
		UE_LOG(LogTemp, Warning, TEXT("Enter Chase"));
		break;

	case EEnemyStateType::Investigate:
		UE_LOG(LogTemp, Warning, TEXT("Enter Investigate"));
		bIsWaitingAtInvestigateLocation = false;
		MoveToLastHeardLocation();
		break;

	case EEnemyStateType::Flee:
	{
		AccumulatedBurstDamage = 0.f;

		if (!bHasValidHideLocation)
		{
			bHasValidHideLocation = FindHideLocation(
				TargetActor,
				HideLocation
			);
		}

		if (bHasValidHideLocation)
		{
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
				FVector AwayDirection =
					Enemy->GetActorLocation() -
					TargetActor->GetActorLocation();

				AwayDirection.Z = 0.f;
				AwayDirection.Normalize();

				HideLocation =
					Enemy->GetActorLocation() +
					AwayDirection * MinimumFleeDistanceFromPlayer;

				bHasValidHideLocation = true;
				Task_MoveToHideLocation();
			}
		}

		break;
	}

	case EEnemyStateType::Hide:
		Task_StopMovement();
		HideStartTime = GetWorld()->GetTimeSeconds();
		break;

	case EEnemyStateType::Recover:
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
		break;

	case EEnemyStateType::Investigate:
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(InvestigateWaitTimerHandle);
		}
		bIsWaitingAtInvestigateLocation = false;
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

	case EEnemyStateType::Investigate:
		UpdateInvestigate(DeltaTime);
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

	// Idle To Patrol 전환은 일단 EnterState에서 타이머로 해결했음

}

void UEnemyStateMachineComponent::UpdatePatrol(float DeltaTime)
{
	if (HasTarget())
	{
		SetState(EEnemyStateType::Chase);
		return;
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

void UEnemyStateMachineComponent::UpdateChase(float DeltaTime)
{
	if (!HasTarget() || ShouldLoseTarget())
	{
		ClearTarget();
		SetState(EEnemyStateType::Patrol);
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
			SetState(EEnemyStateType::Patrol);
			return;
		}
	}

	if (IsTargetInAttackRange())
	{
		SetState(EEnemyStateType::Attack);
		return;
	}

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
		if (!FindHideLocation(TargetActor, HideLocation))
		{
			return;
		}

		bHasValidHideLocation = true;
		Task_MoveToHideLocation();
	}

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
	bHasValidHideLocation = FindHideLocation(
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
		bHasValidHideLocation = FindHideLocation(
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
		bHasValidHideLocation = FindHideLocation(
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

	const float Distance = FVector::Dist(
		Enemy->GetActorLocation(),
		TargetActor->GetActorLocation()
	);

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

	const bool bLowHealth =
		HealthComponent->GetHealthPercent() <= FleeHealthThresholdPercent;

	const bool bReceivedBurstDamage =
		AccumulatedBurstDamage >= BurstDamageThreshold;

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
		return false;
	}

	UNavigationSystemV1* NavigationSystem =
		UNavigationSystemV1::GetCurrent(GetWorld());

	if (!NavigationSystem)
	{
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

bool UEnemyStateMachineComponent::IsLocationHiddenFromActor(const FVector& Location, AActor* Observer) const
{
	if (!Observer || !GetWorld())
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Observer->GetActorEyesViewPoint(ViewLocation, ViewRotation);

	/*
	 * 후보 지점의 발 위치가 아니라 Enemy 몸통 높이를 검사한다.
	 */
	const FVector CandidateViewLocation =
		Location + FVector(0.f, 0.f, 80.f);

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
		1.f,
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
	if (!Enemy || !HealthComponent)
	{
		return;
	}

	if (CurrentState == EEnemyStateType::Dead)
	{
		return;
	}

	if (ThreatActor)
	{
		TargetActor = ThreatActor;
	}

	if (!IsDefensiveState())
	{
		StateBeforeFlee = CurrentState;
	}

	/*
	 * 최대 회복 가능 체력은 MaxHealth의 일정 비율이다.
	 * 예: 최대 체력 100, 비율 0.65 → 65까지만 회복 가능.
	 */
	RecoveryHealthCap =
		HealthComponent->GetMaxHealth() * RecoveryHealthCapPercent;

	/*
	 * 현재 체력이 이미 제한치보다 높다면 회복은 하지 않고
	 * 은신 시간만 가진 뒤 복귀한다.
	 */
	RecoveryHealthCap = FMath::Max(
		RecoveryHealthCap,
		HealthComponent->GetCurrentHealth()
	);

	bHasValidHideLocation = FindHideLocation(
		TargetActor,
		HideLocation
	);

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

void UEnemyStateMachineComponent::Task_MoveToTarget()
{
	if (!AIController || !HasTarget()) return;

	AIController->MoveToTarget(TargetActor);
}

void UEnemyStateMachineComponent::Task_AttackTarget()
{
	if (!Enemy || !HasTarget()) return;

	LastAttackTime = GetWorld()->GetTimeSeconds();

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

	if (!bHasHeardLocation)
	{
		SetState(EEnemyStateType::Patrol);
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

	MoveToLastHeardLocation();
}

void UEnemyStateMachineComponent::MoveToLastHeardLocation()
{
	if (!AIController || !bHasHeardLocation) return;

	AIController->MoveToLocationPoint(
		LastHeardLocation,
		InvestigateAcceptanceRadius
	);
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
		SetState(EEnemyStateType::Patrol);
	}
}

void UEnemyStateMachineComponent::NotifyDamageReceived(float DamageAmount, AActor* DamageInstigator)
{
	if (!Enemy || !HealthComponent || HealthComponent->IsDead())
	{
		return;
	}

	if (DamageAmount <= 0.f)
	{
		return;
	}

	if (DamageInstigator)
	{
		TargetActor = DamageInstigator;
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
}
