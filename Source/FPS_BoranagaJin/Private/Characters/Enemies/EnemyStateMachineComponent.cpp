#include "Characters/Enemies/EnemyStateMachineComponent.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Kismet/GameplayStatics.h"

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
	}

	TargetActor = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	SetState(EEnemyStateType::Idle);
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
		break;

	case EEnemyStateType::Patrol:
		UE_LOG(LogTemp, Warning, TEXT("Enter Patrol"));
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
		break;
	}
}

void UEnemyStateMachineComponent::ExitState(EEnemyStateType OldState)
{
	switch (OldState)
	{
	case EEnemyStateType::Idle:
		break;

	case EEnemyStateType::Patrol:
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

	// 나중에 Idle 시간이 지나면 Patrol로 전환 가능
}

void UEnemyStateMachineComponent::UpdatePatrol(float DeltaTime)
{
	if (HasTarget())
	{
		SetState(EEnemyStateType::Chase);
		return;
	}

	// 나중에 Patrol Point 이동 구현
}

void UEnemyStateMachineComponent::UpdateChase(float DeltaTime)
{
	if (!HasTarget() || ShouldLoseTarget())
	{
		ClearTarget();
		SetState(EEnemyStateType::Idle);
		return;
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
		SetState(EEnemyStateType::Idle);
		return;
	}

	if (ShouldLoseTarget())
	{
		ClearTarget();
		SetState(EEnemyStateType::Idle);
		return;
	}

	if (!IsTargetInAttackRange())
	{
		SetState(EEnemyStateType::Chase);
		return;
	}

	if (CanAttack())
	{
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