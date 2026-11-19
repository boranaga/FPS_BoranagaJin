#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyBaseAIController.generated.h"

class AEnemyBase;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

/**
 * Perception receives ALL sound stimuli. Gameplay semantics live in
 * EnemyStateMachineComponent, not in the sense or the weapon.
 */
UCLASS()
class FPS_BORANAGAJIN_API AEnemyBaseAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyBaseAIController();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

private:
    UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
    TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    void HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus);
    void HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus);

    bool FindGroundNavLocation(const FVector& InputLocation, FVector& OutLocation) const;

protected:
    UPROPERTY(EditAnywhere, Category = "AI|Movement")
    float GroundTraceHeight = 300.f;

    UPROPERTY(EditAnywhere, Category = "AI|Movement")
    float GroundTraceDepth = 500.f;

    UPROPERTY(EditAnywhere, Category = "AI|Movement")
    FVector NavProjectionExtent = FVector(100.f, 100.f, 300.f);

    /** Skip AI-owned sources (enemy firearms / enemy thrown projectiles). */
    UPROPERTY(EditAnywhere, Category = "AI|Perception")
    bool bIgnoreEnemyGeneratedNoise = true;

public:
    AEnemyBase* GetEnemyCharacter() const;
    void MoveToTarget(AActor* Target);
    /** Navigation on the current side of a closed door (no partial routes). */
    bool MoveToDoorBreachPosition(const FVector& Location, float AcceptanceRadius = 65.f);
    /** Single path request to a validated lateral waypoint (never per Tick). */
    bool MoveToCombatDodgeLocation(const FVector& Location, float AcceptanceRadius = 95.f);
    void MoveToLocationPoint(const FVector& Location, float AcceptanceRadius = 50.f);
    void MoveToLocationPoint_Upgrade(const FVector& Location, float AcceptanceRadius = 50.f);
    void StopAIMovement();
    void DisablePerception();
};
