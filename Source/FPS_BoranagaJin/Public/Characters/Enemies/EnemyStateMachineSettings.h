#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyStateMachineSettings.generated.h"

/**
 * Tunable values used by UEnemyStateMachineComponent.
 * One DataTable row can be shared by many enemy instances.
 * Runtime memory/state and level Actor pointers are intentionally excluded.
 */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FEnemyStateMachineSettings : public FTableRowBase
{
    GENERATED_BODY()
public:

    // Common - Target
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common - Target")
    float TargetMemoryDuration = 3.f;

    // Combat - Stamina
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack - Stamina")
    float AttackStaminaCost = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Stamina")
    float ChaseStaminaCostPerSecond = 5.f;

    // Idle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle", meta = (ClampMin = "0.0"))
    float IdleToPatrolTime = 4.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
    TArray<float> IdleLookYawOffsets =
    {
        -60.f,
        0.f,
        60.f,
        0.f
    };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
    float IdleLookRotationRate = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
    float IdleLookAcceptanceAngle = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
    float IdleLookWaitTimeMin = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
    float IdleLookWaitTimeMax = 1.8f;

    // Patrol
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    float PatrolWaitTime = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    float PatrolAcceptanceRadius = 100.f;

    // BreachDoor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor")
    bool bEnableDoorBreach = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor")
    bool bAllowGunshotInvestigationDoorBreach = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor")
    bool bAllowLightInvestigationDoorBreach = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "500.0"))
    float DoorBreachDetectionRadius = 750.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "0.1"))
    float DoorBreachSearchInterval = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "0.1"))
    float DoorBreachStallDuration = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "10.0"))
    float DoorBreachStallProgressDistance = 45.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "1.0"))
    float DoorBreachDetourRatio = 1.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "20.0"))
    float DoorBreachAttackReach = 170.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "10.0"))
    float DoorBreachApproachOffset = 105.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "10.0"))
    float DoorBreachApproachAcceptance = 65.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "0.1"))
    float DoorBreachMoveRequestInterval = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "1.0"))
    float DoorBreachMaximumDuration = 12.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor", meta = (ClampMin = "0.0"))
    float DoorBreachFailureCooldown = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BreachDoor")
    bool bDrawDoorBreachDebug = false;

    // Investigate
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigate")
    float InvestigateAcceptanceRadius = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigate")
    float InvestigateWaitTime = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Investigate", meta = (ClampMin = "1.0"))
    float InvestigateMaximumTravelTime = 12.f;

    // Perception - Hearing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0"))
    float CombatHearingRange = 3500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float OccludedSoundMultiplier = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinimumAudibility = 0.06f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0"))
    float GunshotMinimumEstimateError = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0"))
    float GunshotMaximumEstimateError = 800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0"))
    float GunshotMemoryDuration = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0"))
    float GunshotMoveRetargetInterval = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0"))
    float GunshotRetargetDistance = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GunshotImpactFusionWeight = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "0.0"))
    float GunshotImpactFusionMaxCorrection = 700.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "100.0"))
    float ImpactOnlyMinimumSourceEstimateDistance = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Hearing", meta = (ClampMin = "100.0"))
    float ImpactOnlyMaximumSourceEstimateDistance = 2200.f;

    // Perception - Light
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "0.0"))
    float LightCueMemoryDuration = 7.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "0.1"))
    float LightCueRetargetInterval = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "20.0"))
    float LightCueRetargetDistance = 160.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception - Light", meta = (ClampMin = "0.0"))
    float LightCueMinimumMoveDistance = 80.f;

    // EvadeImpact
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EvadeImpact", meta = (ClampMin = "0.0"))
    float ImpactReactionRadius = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EvadeImpact", meta = (ClampMin = "0.0"))
    float ImpactSafeDistance = 1650.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EvadeImpact", meta = (ClampMin = "100.0"))
    float ImpactEvadeSearchRadius = 1300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EvadeImpact", meta = (ClampMin = "1", UIMin = "1"))
    int32 ImpactEvadeSampleCount = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EvadeImpact", meta = (ClampMin = "10.0"))
    float ImpactEvadeAcceptanceRadius = 110.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EvadeImpact", meta = (ClampMin = "0.0"))
    float ImpactEvadeMinimumTime = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EvadeImpact", meta = (ClampMin = "0.1"))
    float ImpactEvadeMaximumTime = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EvadeImpact", meta = (ClampMin = "0.0"))
    float ImpactNoiseCooldown = 0.35f;

    // Chase - Combat Dodge
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge")
    bool bEnableCombatZigzag = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "0.2"))
    float CombatDodgeThreatMemory = 2.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "100.0"))
    float CombatDodgeForwardStep = 430.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "50.0"))
    float CombatDodgeLateralStep = 370.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "0.25"))
    float CombatDodgeSideSwitchInterval = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "0.2"))
    float CombatDodgeMinimumRepathInterval = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "0.2"))
    float CombatDodgeMaximumRepathInterval = 1.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "50.0"))
    float CombatDodgeTargetMoveRepathDistance = 420.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "20.0"))
    float CombatDodgeWaypointAcceptance = 95.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "50.0"))
    float CombatDodgeHazardRadius = 620.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "0.0"))
    float CombatDodgeImpactAwayBias = 480.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge", meta = (ClampMin = "0.2"))
    float CombatDodgeImpactMemory = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase - Combat Dodge")
    bool bDrawCombatDodgeDebug = true;

    // Flee
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
    float FleeHealthThresholdPercent = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
    float BurstDamageThreshold = 35.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
    float BurstDamageWindow = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
    float FleeSearchRadius = 1800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
    TArray<float> FleeSearchRadii =
    {
        600.f,
        1000.f,
        1500.f
    };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
    float MinimumFleeDistanceFromPlayer = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
    float HideAcceptanceRadius = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flee")
    int32 HideLocationSampleCount = 12;

    // Hide
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hide")
    float MinimumHideTime = 3.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hide")
    float MaximumHideTime = 8.f;

    // Recover
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recover")
    float RecoveryHealthPerSecond = 8.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recover", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RecoveryHealthCapPercent = 0.65f;

    // TrackBlood
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackBlood")
    float BloodDetectionRadius = 700.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackBlood")
    float BloodTrackAcceptanceRadius = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackBlood")
    float BloodTrailLinkRadius = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackBlood")
    float MaximumTrackableBloodAge = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackBlood")
    float BloodSearchInterval = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrackBlood")
    float BloodTrailLostWaitTime = 2.f;

    // Patrol point selection: place matching Actor Tags on AEnemyPatrolPoint instances.
    // If no tags are provided, component instance PatrolPoints are used as before.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
    TArray<FName> PatrolPointTags;

};
