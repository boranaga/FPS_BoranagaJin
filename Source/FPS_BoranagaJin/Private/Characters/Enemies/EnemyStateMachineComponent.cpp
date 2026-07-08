#include "Characters/Enemies/EnemyStateMachineComponent.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyPatrolPoint.h"
#include "Characters/HealthComponent.h"
#include "Characters/StaminaComponent.h"

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
