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
	TObjectPtr<AEnemyBase> Enemy;

	UPROPERTY()
	TObjectPtr<AEnemyBaseAIController> AIController;

	UPROPERTY()
	TObjectPtr<AActor> TargetActor;

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

	bool IsInDefensiveState() const;
	void NotifyThreatSeen(AActor* ThreatActor);


private:
	void EnterState(EEnemyStateType NewState);
	void ExitState(EEnemyStateType OldState);

	void UpdateState(float DeltaTime);

	void UpdateIdle(float DeltaTime);
	void UpdatePatrol(float DeltaTime);
	void UpdateChase(float DeltaTime);
	void UpdateAttack(float DeltaTime);
	void UpdateFlee(float DeltaTime);
	void UpdateHide(float DeltaTime);
	void UpdateRecover(float DeltaTime);
	void UpdateDead(float DeltaTime);

	bool CanSeeTarget() const;
	bool HasTarget() const;
	bool IsTargetInAttackRange() const;
	bool CanAttack() const;
	bool ShouldLoseTarget() const;

	bool ShouldFlee() const;
	bool IsDefensiveState() const;

	bool FindHideLocation(AActor* ThreatActor, FVector& OutLocation) const;
	bool IsLocationHiddenFromActor(const FVector& Location, AActor* Observer) const;

	bool IsAtHideLocation() const;
	bool IsRecoveryComplete() const;
	bool HasMinimumHideTimeElapsed() const;
	bool HasMaximumHideTimeElapsed() const;

	void BeginFlee(AActor* ThreatActor);
	void ReturnToPreviousBehavior();


	void Task_MoveToTarget();
	void Task_AttackTarget();
	void Task_MoveToHideLocation();
	void Task_StopMovement();

// <Patrol Point>
private:
	void MoveToCurrentPatrolPoint();
	void SelectNextPatrolPoint();
	void OnPatrolWaitFinished();
	bool IsAtCurrentPatrolPoint() const;

// <Investigate>
private:
	FVector LastHeardLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Enemy|Investigate")
	float InvestigateAcceptanceRadius = 100.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Investigate")
	float InvestigateWaitTime = 2.f;

	bool bHasHeardLocation = false;
	bool bIsWaitingAtInvestigateLocation = false;

	FTimerHandle InvestigateWaitTimerHandle;

public:
	void SetLastHeardLocation(const FVector& NewLocation);

private:
	void UpdateInvestigate(float DeltaTime);
	void MoveToLastHeardLocation();
	bool IsAtLastHeardLocation() const;
	void OnInvestigateWaitFinished();

// <Damaged>
public:
	void NotifyDamageReceived(float DamageAmount, AActor* DamageInstigator);

// <Flee>
private:
	// 이 비율 이하로 체력이 내려가면 도망
	UPROPERTY(EditAnywhere, Category = "Enemy|Flee")
	float FleeHealthThresholdPercent = 0.3f;

	// 짧은 시간 안에 이 피해량 이상 받으면 도망
	UPROPERTY(EditAnywhere, Category = "Enemy|Flee")
	float BurstDamageThreshold = 35.f;

	// 급격한 피해를 누적해서 검사할 시간
	UPROPERTY(EditAnywhere, Category = "Enemy|Flee")
	float BurstDamageWindow = 1.5f;

	float AccumulatedBurstDamage = 0.f;
	float BurstDamageWindowEndTime = 0.f;

// <Hide and Heal>
	UPROPERTY(EditAnywhere, Category = "Enemy|Flee")
	float FleeSearchRadius = 1800.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Flee")
	float MinimumFleeDistanceFromPlayer = 700.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Flee")
	float HideAcceptanceRadius = 120.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Flee")
	int32 HideLocationSampleCount = 20;

	FVector HideLocation = FVector::ZeroVector;
	bool bHasValidHideLocation = false;

	UPROPERTY(EditAnywhere, Category = "Enemy|Hide")
	float MinimumHideTime = 3.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Hide")
	float MaximumHideTime = 8.f;

	float HideStartTime = 0.f;

	// 초당 회복량
	UPROPERTY(EditAnywhere, Category = "Enemy|Recovery")
	float RecoveryHealthPerSecond = 8.f;

	// 최대 체력의 65%까지만 회복 가능
	UPROPERTY(EditAnywhere, Category = "Enemy|Recovery",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecoveryHealthCapPercent = 0.65f;

	float RecoveryHealthCap = 0.f;

	// 도망치기 전 상태
	EEnemyStateType StateBeforeFlee = EEnemyStateType::Patrol;



// <Attack>
private:
	void RotateToTarget(float DeltaTime);

#pragma region Debuging
public:
	void DrawDebug() const;
#pragma endregion
};