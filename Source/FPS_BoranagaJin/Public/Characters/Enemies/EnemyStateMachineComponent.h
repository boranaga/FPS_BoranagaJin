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
class UEnemyAudioComponent;
class ABloodStainActor;
class ADestructibleDoor;

enum class EEnemyIdleLookPhase : uint8
{
	Waiting,
	Rotating
};

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

	// Sight is authoritative. Heard sound never marks a target as visible.
	bool bTargetCurrentlyVisible = false;
	FVector LastSeenTargetLocation = FVector::ZeroVector;
	bool bHasLastSeenTargetLocation = false;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|State")
	EEnemyStateType CurrentState = EEnemyStateType::Idle;

	UPROPERTY(EditAnywhere, Category = "Target")
	float TargetMemoryDuration = 3.f;

	FTimerHandle TargetDurationTimerHandle;

	float LastAttackTime = -999.f;

	// <AudioComponent>
private:
	UPROPERTY()
	TObjectPtr<UEnemyAudioComponent> EnemyAudioComponent;
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
#pragma region Idle
private:
	FTimerHandle IdleToPatrolTimerHandle;
	float IdleToPatrolTime = 4.f;

	FRotator IdleBaseRotation = FRotator::ZeroRotator;
	FRotator IdleTargetRotation = FRotator::ZeroRotator;

	// 다음에 바라볼 방향 인덱스
	int32 IdleLookDirectionIndex = 0;

	EEnemyIdleLookPhase IdleLookPhase = EEnemyIdleLookPhase::Waiting;

	float IdlePhaseStartTime = 0.f;

	//TODO: Rotation 시작 방향(좌인지 우인지)는 Random으로 결정하도록 함
	UPROPERTY(EditAnywhere, Category = "Enemy|Idle")
	TArray<float> IdleLookYawOffsets =
	{
		-60.f,
		0.f,
		60.f,
		0.f
	};

	UPROPERTY(EditAnywhere, Category = "Enemy|Idle")
	float IdleLookRotationRate = 120.f;
	UPROPERTY(EditAnywhere, Category = "Enemy|Idle")
	float IdleLookAcceptanceAngle = 2.f;
	UPROPERTY(EditAnywhere, Category = "Enemy|Idle")
	float IdleLookWaitTimeMin = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Enemy|Idle")
	float IdleLookWaitTimeMax = 1.8f;

	float CurrentIdleLookWaitTime = 1.f;
private:
	void OnIdleFinished();

	void BeginIdleLook();
	void UpdateIdleLook(float DeltaTime);
	void SelectNextIdleLookDirection();
	bool HasReachedIdleLookRotation() const;

	//void SetOrientRotationToMovement(bool bEnable);
#pragma endregion
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
	AActor* GetTrackedTarget() const { return TargetActor.Get(); }

	void OnTargetFound();
	void OnTargetMissed();

	void SetState(EEnemyStateType NewState);
	EEnemyStateType GetCurrentState() const;

	bool IsInDefensiveState() const;
	void NotifyThreatSeen(AActor* ThreatActor);

	// Called by AIController after UAISense_Hearing detects the sound. Never assigns TargetActor.
	void NotifyGunshotHeard(const FVector& SoundLocation, float Loudness, float MaxRange,
		const AActor* NoiseInstigator = nullptr);
	void NotifyProjectileImpactHeard(
		const FVector& ImpactLocation,
		const FVector& ProjectileTravelDirection,
		float Loudness,
		float MaxRange,
		bool bMayInferShooterFromDirection = true,
		const AActor* NoiseInstigator = nullptr);


private:
	void EnterState(EEnemyStateType NewState);
	void ExitState(EEnemyStateType OldState);

	void UpdateState(float DeltaTime);

	void UpdateIdle(float DeltaTime);
	void UpdatePatrol(float DeltaTime);
	void UpdateChase(float DeltaTime);
	void UpdateAttack(float DeltaTime);
	void UpdateBreachDoor(float DeltaTime);
	void UpdateFlee(float DeltaTime);
	void UpdateHide(float DeltaTime);
	void UpdateRecover(float DeltaTime);
	void UpdateInvestigate(float DeltaTime);
	void UpdateEvadeImpact(float DeltaTime);
	void UpdateDead(float DeltaTime);

	bool CanSeeTarget() const;
	bool HasTarget() const;
	bool IsTargetInAttackRange() const;
	bool CanAttack() const;
	bool ShouldLoseTarget() const;

	bool ShouldFlee() const;
	bool IsDefensiveState() const;

	bool FindHideLocation(AActor* ThreatActor, FVector& OutLocation) const;
	bool FindHideLocation_Upgrade(AActor* ThreatActor, FVector& OutLocation) const;
	bool IsLocationHiddenFromActor(const FVector& Location, AActor* Observer) const;

	bool IsAtHideLocation() const;
	bool IsRecoveryComplete() const;
	bool HasMinimumHideTimeElapsed() const;
	bool HasMaximumHideTimeElapsed() const;

	void BeginFlee(AActor* ThreatActor);
	void ReturnToPreviousBehavior();
	void StopChase();

	void Task_MoveToTarget();
	void Task_AttackTarget();
	void Task_MoveToHideLocation();
	void Task_StopMovement();

	// <Door Breach: player path obstruction>
private:
	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach")
	bool bEnableDoorBreach = true;

	// Optional: only when a recent gunshot points to the destination.
	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach")
	bool bAllowGunshotInvestigationDoorBreach = true;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "100.0"))
	float DoorBreachDetectionRadius = 750.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "0.1"))
	float DoorBreachSearchInterval = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "0.1"))
	float DoorBreachStallDuration = 0.85f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "10.0"))
	float DoorBreachStallProgressDistance = 45.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "1.0"))
	float DoorBreachDetourRatio = 1.85f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "20.0"))
	float DoorBreachAttackReach = 170.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "10.0"))
	float DoorBreachApproachOffset = 105.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "10.0"))
	float DoorBreachApproachAcceptance = 65.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "0.1"))
	float DoorBreachMoveRequestInterval = 0.85f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "1.0"))
	float DoorBreachMaximumDuration = 12.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach", meta = (ClampMin = "0.0"))
	float DoorBreachFailureCooldown = 5.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Door Breach")
	bool bDrawDoorBreachDebug = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<ADestructibleDoor> BreachDoorTarget;

	EEnemyStateType DoorBreachReturnState = EEnemyStateType::Chase;
	float DoorBreachStartedAt = -BIG_NUMBER;
	float LastDoorBreachCheckAt = -BIG_NUMBER;
	float LastDoorApproachMoveAt = -BIG_NUMBER;
	float LastDoorBreachAbortAt = -BIG_NUMBER;
	float LastDoorChaseProgressAt = -BIG_NUMBER;
	FVector LastDoorChaseProgressLocation = FVector::ZeroVector;
	FVector DoorBreachApproachPosition = FVector::ZeroVector;
	bool bHasDoorBreachApproachPosition = false;

	void UpdateDoorChaseMovementProgress();
	bool IsDoorChaseMovementStalled() const;
	bool IsDoorBlockingLineTo(const ADestructibleDoor* Door, const FVector& Goal) const;
	bool FindDoorBlockingDestination(const FVector& Goal, ADestructibleDoor*& OutDoor) const;
	bool IsDoorSignificantObstacle(const FVector& Goal) const;
	bool TryBeginDoorBreach(const FVector& Goal, EEnemyStateType ReturnState);
	bool FindDoorBreachApproachPosition(const ADestructibleDoor* Door, FVector& OutLocation) const;
	void FinishDoorBreach(bool bFailed);
	void ResetDoorBreachProgress();

	// <Patrol Point>
private:
	void MoveToCurrentPatrolPoint();
	void SelectNextPatrolPoint();
	void OnPatrolWaitFinished();
	bool IsAtCurrentPatrolPoint() const;
	FVector GetCurrPatrolPointLocation() const;

	// <Investigate>
private:
	FVector LastHeardLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Enemy|Investigate")
	float InvestigateAcceptanceRadius = 100.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Investigate")
	float InvestigateWaitTime = 2.f;

	// Fail-safe for unreachable/invalid investigation destinations.
	UPROPERTY(EditAnywhere, Category = "Enemy|Investigate", meta = (ClampMin = "1.0"))
	float InvestigateMaximumTravelTime = 12.f;

	float InvestigateStartTime = 0.f;
	float LastInvestigateMoveTime = -BIG_NUMBER;
	FVector LastRequestedInvestigateLocation = FVector::ZeroVector;

	bool bHasHeardLocation = false;
	bool bIsWaitingAtInvestigateLocation = false;

	FTimerHandle InvestigateWaitTimerHandle;

public:
	void SetLastHeardLocation(const FVector& NewLocation);

private:
	void MoveToLastHeardLocation();
	bool IsAtLastHeardLocation() const;
	void OnInvestigateWaitFinished();

	// <Combat Hearing> -- all distances are in centimeters.
private:
	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "0.0"))
	float CombatHearingRange = 3500.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OccludedSoundMultiplier = 0.55f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimumAudibility = 0.06f;

	// At low audibility a larger position error is applied to gunshots.
	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "0.0"))
	float GunshotMinimumEstimateError = 120.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "0.0"))
	float GunshotMaximumEstimateError = 800.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "0.0"))
	float GunshotMemoryDuration = 10.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "0.0"))
	float GunshotMoveRetargetInterval = 0.65f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "0.0"))
	float GunshotRetargetDistance = 150.f;

	// Optional projectile flight bearing can refine (not reveal) a recent shot.
	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GunshotImpactFusionWeight = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "0.0"))
	float GunshotImpactFusionMaxCorrection = 700.f;

	/** With only a bullet bearing, hypothesize a point somewhere behind the impact. */
	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "100.0"))
	float ImpactOnlyMinimumSourceEstimateDistance = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Hearing", meta = (ClampMin = "100.0"))
	float ImpactOnlyMaximumSourceEstimateDistance = 2200.f;

	FVector EstimatedGunshotLocation = FVector::ZeroVector;
	bool bHasGunshotMemory = false;
	float LastGunshotTime = -BIG_NUMBER;
	float LastGunshotMoveTime = -BIG_NUMBER;

	bool TryHearCombatNoise(const FVector& NoiseLocation, float Loudness,
		float MaxRange, float& OutAudibility) const;
	FVector EstimateGunshotLocation(const FVector& NoiseLocation, float Audibility) const;
	bool HasRecentGunshotMemory() const;
	void RefineGunshotEstimateFromImpact(const FVector& ImpactLocation,
		const FVector& ProjectileTravelDirection);

	// <Impact Evasion> -- independent of health-based Flee/Hide/Recover.
private:
	UPROPERTY(EditAnywhere, Category = "Enemy|Impact Avoidance", meta = (ClampMin = "0.0"))
	float ImpactReactionRadius = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Impact Avoidance", meta = (ClampMin = "0.0"))
	float ImpactSafeDistance = 1650.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Impact Avoidance", meta = (ClampMin = "100.0"))
	float ImpactEvadeSearchRadius = 1300.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Impact Avoidance", meta = (ClampMin = "1", UIMin = "1"))
	int32 ImpactEvadeSampleCount = 20;

	UPROPERTY(EditAnywhere, Category = "Enemy|Impact Avoidance", meta = (ClampMin = "10.0"))
	float ImpactEvadeAcceptanceRadius = 110.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Impact Avoidance", meta = (ClampMin = "0.0"))
	float ImpactEvadeMinimumTime = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Impact Avoidance", meta = (ClampMin = "0.1"))
	float ImpactEvadeMaximumTime = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Impact Avoidance", meta = (ClampMin = "0.0"))
	float ImpactNoiseCooldown = 0.35f;

	FVector LastImpactLocation = FVector::ZeroVector;
	FVector LastImpactTravelDirection = FVector::ZeroVector;
	FVector ImpactEvadeLocation = FVector::ZeroVector;
	bool bHasImpactEvadeLocation = false;
	float ImpactEvadeStartTime = 0.f;
	float LastImpactTime = -BIG_NUMBER;
	float LastImpactMoveTime = -BIG_NUMBER;
	float LastImpactRepathTime = -BIG_NUMBER;
	EEnemyStateType StateBeforeImpactEvade = EEnemyStateType::Patrol;

	bool FindImpactEvadeLocation(FVector& OutLocation) const;
	void MoveToImpactEvadeLocation();
	void FinishImpactEvasion();

	// <Combat Chase / Zigzag>
private:
	// Only used while a player is confirmed by sight AND recent combat sound exists.
	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge")
	bool bEnableCombatZigzag = true;

	// How long a gunshot or dangerous impact keeps the enemy maneuvering (seconds).
	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "0.2"))
	float CombatDodgeThreatMemory = 2.8f;

	// Each sideways waypoint also advances toward the player (centimeters).
	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "100.0"))
	float CombatDodgeForwardStep = 430.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "50.0"))
	float CombatDodgeLateralStep = 370.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "0.25"))
	float CombatDodgeSideSwitchInterval = 0.95f;

	// Prevent rebuilding paths on every Tick / every individual rifle shot.
	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "0.2"))
	float CombatDodgeMinimumRepathInterval = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "0.2"))
	float CombatDodgeMaximumRepathInterval = 1.15f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "50.0"))
	float CombatDodgeTargetMoveRepathDistance = 420.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "20.0"))
	float CombatDodgeWaypointAcceptance = 95.f;

	// At this distance from the last impact, survival has priority over progress.
	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "50.0"))
	float CombatDodgeHazardRadius = 620.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "0.0"))
	float CombatDodgeImpactAwayBias = 480.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge", meta = (ClampMin = "0.2"))
	float CombatDodgeImpactMemory = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Combat Dodge")
	bool bDrawCombatDodgeDebug = true;

	float LastCombatThreatTime = -BIG_NUMBER;
	float LastCombatDodgePathTime = -BIG_NUMBER;
	float LastCombatDodgeSideChangeTime = -BIG_NUMBER;
	int32 CombatDodgeSide = 1;
	bool bHasCombatDodgeWaypoint = false;
	bool bForceCombatDodgeRepath = false;
	FVector CombatDodgeWaypoint = FVector::ZeroVector;
	FVector CombatDodgeTargetAtPathTime = FVector::ZeroVector;

	void RegisterCombatThreat(bool bUrgent);
	bool IsCombatDodgeActive() const;
	bool IsRecentImpactForDodge() const;
	bool TryFindCombatDodgeWaypoint(FVector& OutWaypoint) const;
	bool UpdateCombatDodgeChase();
	void ResetCombatDodgeMovement();

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
	TArray<float> FleeSearchRadii =
	{
		600.f,
		1000.f,
		1500.f
	};

	UPROPERTY(EditAnywhere, Category = "Enemy|Flee")
	float MinimumFleeDistanceFromPlayer = 200.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Flee")
	float HideAcceptanceRadius = 120.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Flee")
	int32 HideLocationSampleCount = 12;

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
	UPROPERTY(EditAnywhere, Category = "Enemy|Recovery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecoveryHealthCapPercent = 0.65f;

	float RecoveryHealthCap = 0.f;

	// 도망치기 전 상태
	EEnemyStateType StateBeforeFlee = EEnemyStateType::Patrol;

	// <Attack>
private:
	void RotateToTarget(float DeltaTime);
	void RotateToTargetDirection(float DeltaTime, FVector TargetLocation);

#pragma region BloodStain
private:
	UPROPERTY(EditAnywhere, Category = "Enemy|Blood Tracking")
	float BloodDetectionRadius = 700.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Blood Tracking")
	float BloodTrackAcceptanceRadius = 150.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Blood Tracking")
	float BloodTrailLinkRadius = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Blood Tracking")
	float MaximumTrackableBloodAge = 25.f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Blood Tracking")
	float BloodSearchInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Enemy|Blood Tracking")
	float BloodTrailLostWaitTime = 2.f;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|Blood Tracking")
	float LastBloodSearchTime = -BIG_NUMBER;
	UPROPERTY(VisibleAnywhere, Category = "Enemy|Blood Tracking")
	float BloodTrailLostStartTime = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|Blood Tracking")
	TObjectPtr<ABloodStainActor> CurrentBloodStain;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|Blood Tracking")
	TWeakObjectPtr<AActor> TrackedBloodOwner;

	UPROPERTY(VisibleAnywhere, Category = "Enemy|Blood Tracking")
	int32 CurrentBloodSequenceIndex = INDEX_NONE;

private:
	void UpdateTrackBlood(float DeltaTime);

	bool CanSearchForBlood() const;
	bool TryAcquireBloodTrail();
	bool FindInitialBloodStain(ABloodStainActor*& OutBloodStain) const;

	bool FindNextBloodStain(ABloodStainActor*& OutBloodStain) const;

	bool IsBloodStainValid(const ABloodStainActor* BloodStain) const;

	bool IsAtCurrentBloodStain() const;

	void SetCurrentBloodStain(ABloodStainActor* NewBloodStain);

	void ClearBloodTrail();
	void ReturnFromBloodTracking();

#pragma endregion

#pragma region Debuging
public:
	void DrawDebug() const;
#pragma endregion
};