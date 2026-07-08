#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyStateType.h"
#include "EnemyStateMachineComponent.generated.h"

class AEnemyBase;
class AEnemyBaseAIController;
class AEnemyPatrolPoint;

class UHealthComponent;
class UStaminaComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPS_BORANAGAJIN_API UEnemyStateMachineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyStateMachineComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

private:
	UPROPERTY()
	AEnemyBase* Enemy;

	UPROPERTY()
	AEnemyBaseAIController* AIController;

	UPROPERTY()
	AActor* TargetActor;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|State")
	EEnemyStateType CurrentState = EEnemyStateType::Idle;

	float LastAttackTime = -999.f;

// <Stamina and Health>
private:
	UPROPERTY()
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY()
	TObjectPtr<UStaminaComponent> StaminaComponent;

	UPROPERTY(EditAnywhere, Category = "Enemy|Stamina")
	float AttackStaminaCost = 20.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Stamina")
	float ChaseStaminaCostPerSecond = 5.f;

// <Idle>
private:
	FTimerHandle IdleToPatrolTimerHandle;
	float IdleToPatrolTime = 4.f;
private:
	void OnIdleFinished();

// <Patrol Point>
private:
	UPROPERTY(EditInstanceOnly, Category = "Enemy|Patrol")
	TArray<AEnemyPatrolPoint*> PatrolPoints;

	UPROPERTY(EditAnywhere, Category = "Enemy|Patrol")
	float PatrolWaitTime = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Patrol")
	float PatrolAcceptanceRadius = 100.f;

	int32 CurrentPatrolIndex = 0;

	bool bIsWaitingAtPatrolPoint = false;

	FTimerHandle PatrolWaitTimerHandle;

public:
	void SetTarget(AActor* NewTarget);
	void ClearTarget();

	void SetState(EEnemyStateType NewState);
	EEnemyStateType GetCurrentState() const;

private:
	void EnterState(EEnemyStateType NewState);
	void ExitState(EEnemyStateType OldState);

	void UpdateState(float DeltaTime);

	void UpdateIdle(float DeltaTime);
	void UpdatePatrol(float DeltaTime);
	void UpdateChase(float DeltaTime);
	void UpdateAttack(float DeltaTime);
	void UpdateDead(float DeltaTime);

	bool CanSeeTarget() const;
	bool HasTarget() const;
	bool IsTargetInAttackRange() const;
	bool CanAttack() const;
	bool ShouldLoseTarget() const;

	void Task_MoveToTarget();
	void Task_AttackTarget();
	void Task_StopMovement();

// <Patrol Point>
private:
	void MoveToCurrentPatrolPoint();
	void SelectNextPatrolPoint();
	void OnPatrolWaitFinished();
	bool IsAtCurrentPatrolPoint() const;

// <Attack>
private:
	void RotateToTarget(float DeltaTime);

#pragma region Debuging
public:
	void DrawDebug() const;
#pragma endregion
};