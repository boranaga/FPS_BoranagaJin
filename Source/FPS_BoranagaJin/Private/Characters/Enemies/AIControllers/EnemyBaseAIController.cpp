#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/EnemyStateMachineComponent.h"
#include "Characters/Enemies/EnemyCombatNoiseSubsystem.h"
#include "Characters/Player/CharacterPlayer.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Engine/World.h"

AEnemyBaseAIController::AEnemyBaseAIController()
{
    PrimaryActorTick.bCanEverTick = false;
    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 2000.f;
    SightConfig->LoseSightRadius = 5000.f;
    SightConfig->PeripheralVisionAngleDegrees = 120.f;
    SightConfig->SetMaxAge(10.f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    // Listener limit. Weapon MaxRange * Loudness may be larger, but this still caps it.
    HearingConfig->HearingRange = 3500.f;
    HearingConfig->LoSHearingRange = 3500.f;
    HearingConfig->SetMaxAge(5.f);
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

    AIPerceptionComponent->ConfigureSense(*SightConfig);
    AIPerceptionComponent->ConfigureSense(*HearingConfig);
    AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
    SetPerceptionComponent(*AIPerceptionComponent);
}

void AEnemyBaseAIController::BeginPlay()
{
    Super::BeginPlay();
    if (AIPerceptionComponent)
    {
        AIPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(
            this, &AEnemyBaseAIController::OnTargetPerceptionUpdated);
    }
}

void AEnemyBaseAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
}

void AEnemyBaseAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        HandleSightStimulus(Actor, Stimulus);
    }
    else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
    {
        HandleHearingStimulus(Actor, Stimulus);
    }
}

void AEnemyBaseAIController::HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
    AEnemyBase* Enemy = GetEnemyCharacter();
    if (!IsValid(Enemy) || Enemy->IsDead() || !IsValid(Actor)) return;

    UEnemyStateMachineComponent* SM = Enemy->GetStateMachineComponent();
    if (!SM) return;

    // A projectile/weapon must never become the chase target just because sight sensed it.
    if (!Actor->IsA(ACharacterPlayer::StaticClass()) &&
        !Actor->ActorHasTag(TEXT("Player")))
    {
        return;
    }

    if (Stimulus.WasSuccessfullySensed())
    {
        SM->SetTarget(Actor);
        SM->OnTargetFound(); // Cancels any previously scheduled loss-of-sight timer.

        // While dodging a nearby impact, remember the newly seen threat but
        // finish the immediate safety movement before starting direct chase.
        if (SM->IsInDefensiveState() ||
            SM->GetCurrentState() == EEnemyStateType::EvadeImpact)
        {
            SM->NotifyThreatSeen(Actor);
            return;
        }
        SM->SetState(EEnemyStateType::Chase);
    }
    else if (SM->GetTrackedTarget() == Actor)
    {
        // Only the currently tracked player's lost sight should start the timer.
        SM->OnTargetMissed();
    }
}

void AEnemyBaseAIController::HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
    if (!Stimulus.WasSuccessfullySensed()) return; // Ignore expired/forgotten stimuli.

    AEnemyBase* Enemy = GetEnemyCharacter();
    if (!IsValid(Enemy) || Enemy->IsDead()) return;

    UEnemyStateMachineComponent* SM = Enemy->GetStateMachineComponent();
    if (!SM || !GetWorld()) return;
    if (!IsValid(Actor) || Actor == Enemy) return;

    if (bIgnoreEnemyGeneratedNoise && Actor->ActorHasTag(TEXT("Enemy")))
    {
        return;
    }

    const FVector HeardLocation = Stimulus.StimulusLocation;
    UEnemyCombatNoiseSubsystem* Store = GetWorld()->GetSubsystem<UEnemyCombatNoiseSubsystem>();

    if (UEnemyCombatNoiseSubsystem::IsCombatNoiseTag(Stimulus.Tag))
    {
        // A combat tag with missing/expired metadata must NOT become a generic
        // 'investigate the precise stimulus location' order.
        FEnemyCombatNoiseEvent Event;
        if (!Store || !Store->ResolveNoise(Stimulus.Tag, HeardLocation, Actor, Event))
        {
            return;
        }

        switch (Event.Type)
        {
        case EEnemyCombatNoiseType::Gunshot:
            // No SetTarget/Chase. We know only an approximate bearing.
            SM->NotifyGunshotHeard(Event.Location, Event.Loudness, Event.MaxRange);
            return;

        case EEnemyCombatNoiseType::ProjectileImpact:
            SM->NotifyProjectileImpactHeard(
                Event.Location, Event.TravelDirection, Event.Loudness, Event.MaxRange,
                Event.bMayInferSourceFromTravel);
            return;

        case EEnemyCombatNoiseType::PayloadActivation:
            // Grenade/flash/smoke activation is a danger cue without a known trajectory.
            SM->NotifyProjectileImpactHeard(
                Event.Location, FVector::ZeroVector, Event.Loudness, Event.MaxRange, false);
            return;
        }
        return;
    }

    // Backward compatibility for noise sources not yet migrated to CombatNoise.
    // Still NEVER set TargetActor based on a hearing-only stimulus.
    if (Stimulus.Tag == TEXT("Firing"))
    {
        SM->NotifyGunshotHeard(HeardLocation, Stimulus.Strength, HearingConfig->HearingRange);
        return;
    }
    if (Stimulus.Tag == TEXT("ProjectileHit"))
    {
        SM->NotifyProjectileImpactHeard(
            HeardLocation, FVector::ZeroVector, Stimulus.Strength, HearingConfig->HearingRange, false);
        return;
    }

    const EEnemyStateType State = SM->GetCurrentState();
    if (SM->IsInDefensiveState() || State == EEnemyStateType::Dead ||
        State == EEnemyStateType::EvadeImpact ||
        State == EEnemyStateType::Chase || State == EEnemyStateType::Attack)
    {
        return;
    }

    // Footsteps, Reloading and other ordinary gameplay cues.
    SM->SetLastHeardLocation(HeardLocation);
    if (State != EEnemyStateType::Investigate)
    {
        SM->SetState(EEnemyStateType::Investigate);
    }
}

bool AEnemyBaseAIController::FindGroundNavLocation(
    const FVector& InputLocation, FVector& OutLocation) const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    const FVector TraceStart = InputLocation + FVector::UpVector * GroundTraceHeight;
    const FVector TraceEnd = InputLocation - FVector::UpVector * GroundTraceDepth;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AIMoveGroundTrace), false);
    if (const APawn* pawn = GetPawn()) Params.AddIgnoredActor(pawn);

    FHitResult GroundHit;
    if (!World->LineTraceSingleByChannel(
        GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params))
    {
        return false;
    }

    UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(World);
    if (!Nav) return false;

    FNavLocation Projected;
    if (!Nav->ProjectPointToNavigation(
        GroundHit.ImpactPoint, Projected, NavProjectionExtent))
    {
        return false;
    }

    OutLocation = Projected.Location;
    return true;
}

AEnemyBase* AEnemyBaseAIController::GetEnemyCharacter() const
{
    return Cast<AEnemyBase>(GetPawn());
}

void AEnemyBaseAIController::MoveToTarget(AActor* Target)
{
    if (!IsValid(Target)) return;
    FAIMoveRequest Request;
    Request.SetGoalActor(Target);
    Request.SetAcceptanceRadius(50.f);
    Request.SetUsePathfinding(true);
    FNavPathSharedPtr Path;
    MoveTo(Request, &Path);
}

void AEnemyBaseAIController::MoveToLocationPoint(const FVector& Location, float AcceptanceRadius)
{
    FAIMoveRequest Request;
    Request.SetGoalLocation(Location);
    Request.SetProjectGoalLocation(true);
    Request.SetAcceptanceRadius(FMath::Max(0.f, AcceptanceRadius));
    Request.SetUsePathfinding(true);
    FNavPathSharedPtr Path;
    MoveTo(Request, &Path);
}

void AEnemyBaseAIController::MoveToLocationPoint_Upgrade(
    const FVector& Location, float AcceptanceRadius)
{
    FVector GroundLocation;
    if (!FindGroundNavLocation(Location, GroundLocation))
    {
        // Keep legacy behavior: no new path when correction fails.
        UE_LOG(LogTemp, Verbose, TEXT("MoveToLocationPoint_Upgrade: no ground-nav point for %s"),
            *Location.ToString());
        return;
    }

    FAIMoveRequest Request;
    Request.SetGoalLocation(GroundLocation);
    Request.SetAcceptanceRadius(FMath::Max(0.f, AcceptanceRadius));
    Request.SetUsePathfinding(true);
    Request.SetProjectGoalLocation(false);
    Request.SetAllowPartialPath(false);
    FNavPathSharedPtr Path;
    MoveTo(Request, &Path);
}

void AEnemyBaseAIController::StopAIMovement()
{
    StopMovement();
}

void AEnemyBaseAIController::DisablePerception()
{
    if (AIPerceptionComponent)
    {
        AIPerceptionComponent->Deactivate();
        AIPerceptionComponent->SetComponentTickEnabled(false);
    }
}
