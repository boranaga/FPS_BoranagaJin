#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/EnemyStateMachineComponent.h"
#include "Characters/Enemies/EnemyCombatNoiseSubsystem.h"
#include "Characters/Enemies/EnemyLightSourceSubsystem.h"
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
#include "Components/SpotLightComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

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
    StartLightObservation();
}

void AEnemyBaseAIController::OnUnPossess()
{
    ResetChaseMoveCache();
    StopLightObservation();
    Super::OnUnPossess();
}

void AEnemyBaseAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ResetChaseMoveCache();
    StopLightObservation();
    Super::EndPlay(EndPlayReason);
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

        // Health-based Flee/Hide/Recover still takes priority. A recent
        // impact is handled by zigzag Chase when the player is visible.
        if (SM->IsInDefensiveState())
        {
            SM->NotifyThreatSeen(Actor);
            return;
        }
        // A visually reacquired target does not interrupt a door breach;
        // the door state itself checks whether the obstacle is still relevant.
        if (SM->GetCurrentState() != EEnemyStateType::BreachDoor)
        {
            SM->SetState(EEnemyStateType::Chase);
        }
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
            SM->NotifyGunshotHeard(Event.Location, Event.Loudness, Event.MaxRange,
                Event.Instigator.Get());
            return;

        case EEnemyCombatNoiseType::ProjectileImpact:
            SM->NotifyProjectileImpactHeard(
                Event.Location, Event.TravelDirection, Event.Loudness, Event.MaxRange,
                Event.bMayInferSourceFromTravel, Event.Instigator.Get());
            return;

        case EEnemyCombatNoiseType::PayloadActivation:
            // Grenade/flash/smoke activation is a danger cue without a known trajectory.
            SM->NotifyProjectileImpactHeard(
                Event.Location, FVector::ZeroVector, Event.Loudness, Event.MaxRange,
                false, Event.Instigator.Get());
            return;
        }
        return;
    }

    // Backward compatibility for noise sources not yet migrated to CombatNoise.
    // Still NEVER set TargetActor based on a hearing-only stimulus.
    if (Stimulus.Tag == TEXT("Firing"))
    {
        SM->NotifyGunshotHeard(HeardLocation, Stimulus.Strength, HearingConfig->HearingRange, Actor);
        return;
    }
    if (Stimulus.Tag == TEXT("ProjectileHit"))
    {
        SM->NotifyProjectileImpactHeard(
            HeardLocation, FVector::ZeroVector, Stimulus.Strength, HearingConfig->HearingRange,
            false, Actor);
        return;
    }

    const EEnemyStateType State = SM->GetCurrentState();
    if (SM->IsInDefensiveState() || State == EEnemyStateType::Dead ||
        State == EEnemyStateType::EvadeImpact ||
        State == EEnemyStateType::BreachDoor ||
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

void AEnemyBaseAIController::ResetChaseMoveCache()
{
    ChaseMoveTarget.Reset();
    ChaseMoveRequestID = FAIRequestID::InvalidRequest;
    bChaseMoveUsesFallback = false;
    bChaseMoveReachedGoal = false;
    bPreferChaseNavFallback = false;
    ChaseTargetLocationAtRequest = FVector::ZeroVector;
    ChasePawnLocationAtProgress = FVector::ZeroVector;
    LastChaseMoveRequestTime = -BIG_NUMBER;
    LastChaseProgressTime = -BIG_NUMBER;
}

void AEnemyBaseAIController::LogChaseMoveFailure(
    const TCHAR* Reason, AActor* Target) const
{
    if (!bLogChaseMoveDiagnostics || !GetWorld()) return;
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastChaseFailureLogTime < 1.5f) return;
    LastChaseFailureLogTime = Now;

    const APawn* pawn = GetPawn();
    const ACharacter* CharacterPawn = Cast<ACharacter>(pawn);
    const UCharacterMovementComponent* Movement =
        CharacterPawn ? CharacterPawn->GetCharacterMovement() : nullptr;
    UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());

    bool bAgentOnNav = false;
    bool bTargetOnNav = false;
    if (Nav && pawn && IsValid(Target))
    {
        FNavLocation Projected;
        bAgentOnNav = Nav->ProjectPointToNavigation(
            pawn->GetActorLocation(), Projected, ChaseNavProjectionExtent);
        bTargetOnNav = Nav->ProjectPointToNavigation(
            Target->GetActorLocation(), Projected, ChaseNavProjectionExtent);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[EnemyChase][%s] %s Target=%s Pawn=%s TargetLoc=%s "
            "Nav=%d PawnOnNav=%d TargetOnNav=%d Following=%d "
            "MovementMode=%d MaxSpeed=%.1f Velocity=%.1f"),
        *GetName(), Reason, *GetNameSafe(Target),
        pawn ? *pawn->GetActorLocation().ToString() : TEXT("None"),
        IsValid(Target) ? *Target->GetActorLocation().ToString() : TEXT("None"),
        Nav != nullptr, bAgentOnNav, bTargetOnNav, IsFollowingAPath(),
        Movement ? static_cast<int32>(Movement->MovementMode) : -1,
        Movement ? Movement->MaxWalkSpeed : -1.f,
        pawn ? pawn->GetVelocity().Size2D() : -1.f);
}

bool AEnemyBaseAIController::TryChaseNavmeshFallback(
    AActor* Target, const FVector& TargetLocation)
{
    UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!Nav)
    {
        LogChaseMoveFailure(TEXT("NavigationSystem missing"), Target);
        return false;
    }

    FNavLocation ProjectedGoal;
    if (!Nav->ProjectPointToNavigation(
        TargetLocation, ProjectedGoal, ChaseNavProjectionExtent))
    {
        LogChaseMoveFailure(TEXT("Target cannot be projected onto NavMesh"), Target);
        return false;
    }

    // Reject a floor on the other side of a large gap or very distant island.
    if (FVector::Dist2D(TargetLocation, ProjectedGoal.Location) >
        FMath::Max(ChaseNavProjectionExtent.X, ChaseNavProjectionExtent.Y))
    {
        LogChaseMoveFailure(TEXT("Projected goal is too far from player"), Target);
        return false;
    }

    FAIMoveRequest Request;
    Request.SetGoalLocation(ProjectedGoal.Location);
    Request.SetUsePathfinding(true);
    Request.SetProjectGoalLocation(false); // Already projected by NavSystem.
    Request.SetAllowPartialPath(false);
    Request.SetAcceptanceRadius(ChaseMoveAcceptanceRadius);

    FNavPathSharedPtr Path;
    const FPathFollowingRequestResult MoveResult = MoveTo(Request, &Path);
    if (MoveResult.Code == EPathFollowingRequestResult::Failed)
    {
        LogChaseMoveFailure(TEXT("Projected-position MoveTo failed"), Target);
        return false;
    }

    ChaseMoveRequestID = MoveResult.MoveId;
    bChaseMoveUsesFallback = true;
    bChaseMoveReachedGoal = (MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal);

    if (bLogChaseMoveDiagnostics)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[EnemyChase][%s] NavMesh fallback accepted: %s Result=%d"),
            *GetName(), *ProjectedGoal.Location.ToString(),
            static_cast<int32>(MoveResult.Code));
    }
    return true;
}

void AEnemyBaseAIController::MoveToTarget(AActor* Target)
{
    if (!IsValid(Target) || !IsValid(GetPawn()) || !GetWorld()) return;

    const float Now = GetWorld()->GetTimeSeconds();
    const FVector TargetLocation = Target->GetActorLocation();
    const FVector PawnLocation = GetPawn()->GetActorLocation();
    const bool bTargetChanged = ChaseMoveTarget.Get() != Target;

    if (bTargetChanged)
    {
        ResetChaseMoveCache();
        ChaseMoveTarget = Target;
    }

    const bool bTargetMoved = FVector::DistSquared2D(
        TargetLocation, ChaseTargetLocationAtRequest) >=
        FMath::Square(ChaseFallbackRetargetDistance);

    if (!bTargetChanged && IsFollowingAPath())
    {
        // An Actor goal is followed by the engine as it moves; do not
        // cancel the current chase path by reissuing MoveTo every Tick.
        if (!bChaseMoveUsesFallback || !bTargetMoved)
        {
            // Detect a path that is technically running but produces no motion.
            if (FVector::DistSquared2D(PawnLocation, ChasePawnLocationAtProgress) >=
                FMath::Square(ChaseProgressThreshold))
            {
                ChasePawnLocationAtProgress = PawnLocation;
                LastChaseProgressTime = Now;
            }
            else if (Now - LastChaseProgressTime >= ChaseStuckTime &&
                FVector::Dist2D(PawnLocation, TargetLocation) >
                ChaseMoveAcceptanceRadius + 100.f)
            {
                LogChaseMoveFailure(TEXT("Stalled while following chase path"), Target);
                // Abandon only the stuck path. Next attempt uses projected Nav goal.
                ChaseMoveRequestID = FAIRequestID::InvalidRequest;
                StopMovement();
                bPreferChaseNavFallback = true;
                LastChaseMoveRequestTime = Now - ChaseMoveRetryInterval;
                LastChaseProgressTime = Now;
            }
            else
            {
                return;
            }

            if (IsFollowingAPath()) return;
        }
    }

    // No per-Tick path requests, even when navigation fails.
    if (Now - LastChaseMoveRequestTime < ChaseMoveRetryInterval) return;

    // At goal: wait until player moves away; otherwise unnecessary MoveTo
    // requests repeat while we stand in melee distance.
    if (bChaseMoveReachedGoal && !bTargetMoved &&
        FVector::Dist2D(PawnLocation, TargetLocation) <=
        ChaseMoveAcceptanceRadius + 100.f)
    {
        return;
    }

    LastChaseMoveRequestTime = Now;
    ChaseTargetLocationAtRequest = TargetLocation;
    ChasePawnLocationAtProgress = PawnLocation;
    LastChaseProgressTime = Now;
    ChaseMoveRequestID = FAIRequestID::InvalidRequest;
    bChaseMoveReachedGoal = false;

    // If the actor-target request failed previously, try the on-NavMesh goal.
    // After player movement during projected fallback, retry Actor mode.
    // Do not immediately retry Actor if an Actor path just stalled/failed.
    if (bTargetMoved && bChaseMoveUsesFallback) bPreferChaseNavFallback = false;

    if (!bPreferChaseNavFallback)
    {
        FAIMoveRequest Request;
        Request.SetGoalActor(Target);
        Request.SetUsePathfinding(true);
        Request.SetAllowPartialPath(false);
        Request.SetAcceptanceRadius(ChaseMoveAcceptanceRadius);

        FNavPathSharedPtr Path;
        const FPathFollowingRequestResult MoveResult = MoveTo(Request, &Path);
        if (MoveResult.Code != EPathFollowingRequestResult::Failed)
        {
            ChaseMoveRequestID = MoveResult.MoveId;
            bChaseMoveUsesFallback = false;
            bChaseMoveReachedGoal =
                MoveResult.Code == EPathFollowingRequestResult::AlreadyAtGoal;
            return;
        }

        LogChaseMoveFailure(TEXT("Actor MoveTo FAILED; trying NavMesh fallback"), Target);
        bPreferChaseNavFallback = true;
    }

    TryChaseNavmeshFallback(Target, TargetLocation);
}

void AEnemyBaseAIController::OnMoveCompleted(
    FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    if (RequestID != ChaseMoveRequestID) return;

    ChaseMoveRequestID = FAIRequestID::InvalidRequest;
    bChaseMoveReachedGoal = Result.Code == EPathFollowingResult::Success;

    if (Result.Code != EPathFollowingResult::Success)
    {
        bPreferChaseNavFallback = true;
        LogChaseMoveFailure(TEXT("Chase path finished unsuccessfully"), ChaseMoveTarget.Get());
    }
}

bool AEnemyBaseAIController::MoveToCombatDodgeLocation(
    const FVector& Location, float AcceptanceRadius)
{
    ResetChaseMoveCache();
    if (!IsValid(GetPawn()) || !GetWorld()) return false;

    FAIMoveRequest Request;
    Request.SetGoalLocation(Location);
    Request.SetAcceptanceRadius(FMath::Max(20.f, AcceptanceRadius));
    Request.SetUsePathfinding(true);
    Request.SetProjectGoalLocation(false); // StateMachine already projected it.
    Request.SetAllowPartialPath(false);

    FNavPathSharedPtr Path;
    const FPathFollowingRequestResult Result = MoveTo(Request, &Path);
    return Result.Code != EPathFollowingRequestResult::Failed;
}

bool AEnemyBaseAIController::MoveToDoorBreachPosition(const FVector& Location, float AcceptanceRadius)
{
    ResetChaseMoveCache();
    if (!IsValid(GetPawn()) || !GetWorld()) return false;
    FAIMoveRequest Request;
    Request.SetGoalLocation(Location);
    Request.SetUsePathfinding(true);
    Request.SetProjectGoalLocation(false); // projected by state machine
    Request.SetAllowPartialPath(false);
    Request.SetAcceptanceRadius(FMath::Max(20.f, AcceptanceRadius));

    FNavPathSharedPtr Path;
    const FPathFollowingRequestResult Result = MoveTo(Request, &Path);
    return Result.Code != EPathFollowingRequestResult::Failed;
}

void AEnemyBaseAIController::MoveToLocationPoint(const FVector& Location, float AcceptanceRadius)
{
    ResetChaseMoveCache();
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
    ResetChaseMoveCache();
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
    ResetChaseMoveCache();
    StopMovement();
}

void AEnemyBaseAIController::DisablePerception()
{
    StopLightObservation();
    if (AIPerceptionComponent)
    {
        AIPerceptionComponent->Deactivate();
        AIPerceptionComponent->SetComponentTickEnabled(false);
    }
}

// -----------------------------------------------------------------------------
// Light is a separate VISUAL cue. It is never reported as UAISense_Hearing.
// Registered player lights only; no TActorIterator on AI Tick.
// -----------------------------------------------------------------------------
void AEnemyBaseAIController::StartLightObservation()
{
    if (!GetWorld() || !HasAuthority() || !GetPawn()) return;
    const float Interval = FMath::Max(0.1f, LightObservationInterval);
    GetWorld()->GetTimerManager().SetTimer(
        LightObservationTimerHandle, this,
        &AEnemyBaseAIController::PollVisiblePlayerLights,
        Interval, true, FMath::FRandRange(0.05f, Interval));
}

void AEnemyBaseAIController::StopLightObservation()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(LightObservationTimerHandle);
    LastObservedSpotLight.Reset();
}

bool AEnemyBaseAIController::CanObserveLightPoint(
    const FVector& EyeLocation, const FVector& Point,
    const AActor* Emitter, const AActor* Player, const AActor* SurfaceActor,
    const FVector& LitSurfaceNormal) const
{
    const AEnemyBase* Enemy = GetEnemyCharacter();
    if (!IsValid(Enemy) || !GetWorld()) return false;

    const FVector ToPoint = Point - EyeLocation;
    if (ToPoint.IsNearlyZero()) return true;

    const FVector Facing = Enemy->GetActorForwardVector().GetSafeNormal();
    const float FacingDot = FVector::DotProduct(Facing, ToPoint.GetSafeNormal());
    if (FacingDot < FMath::Cos(FMath::DegreesToRadians(
        FMath::Clamp(LightObservationHalfFOV, 0.f, 179.f)))) return false;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyLightObservationLOS), false);
    Params.AddIgnoredActor(Enemy);
    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByChannel(
        Hit, EyeLocation, Point, ECC_Visibility, Params)) return true;

    // The player can occlude a lamp worn on their body; the first actor is
    // still physically the player, not an invisible wall.
    if (!SurfaceActor && (Hit.GetActor() == Emitter || Hit.GetActor() == Player))
    {
        return true; // Looking at emitter, possibly with carrier in front.
    }
    if (SurfaceActor && Hit.GetActor() == SurfaceActor &&
        FVector::DistSquared(Hit.ImpactPoint, Point) < FMath::Square(16.f) &&
        FVector::DotProduct(Hit.ImpactNormal, LitSurfaceNormal) > 0.7f)
    {
        return true; // The same ILLUMINATED face, not the back of a thin wall.
    }
    return false;
}

bool AEnemyBaseAIController::TryObserveSpotLight(
    const USpotLightComponent* Light, const AActor* Emitter, const AActor* Player,
    FVector& OutEstimatedPosition, bool& bOutEmitterDirectlyVisible,
    float& OutScore) const
{
    const AEnemyBase* Enemy = GetEnemyCharacter();
    if (!IsValid(Light) || !IsValid(Enemy) || !GetWorld() ||
        !IsValid(Emitter) || !IsValid(Player) || !Light->IsVisible() ||
        Light->Intensity < MinimumVisibleLightIntensity) return false;

    FVector EyeLocation;
    FRotator EyeRotation;
    Enemy->GetActorEyesViewPoint(EyeLocation, EyeRotation);

    const FVector LightOrigin = Light->GetComponentLocation();
    const FVector BeamForward = Light->GetForwardVector().GetSafeNormal();
    const float EffectiveBeamRadius = FMath::Min(
        Light->AttenuationRadius, LightMaxBeamTraceLength);
    if (EffectiveBeamRadius <= 50.f) return false;
    if (FVector::DistSquared(EyeLocation, LightOrigin) >
        FMath::Square(EffectiveBeamRadius + LightPatchDetectionRange)) return false;

    // Light illuminating the enemy: source position is visually discernible.
    // No LOS -> no direct identification through walls.
    const FVector LightToEnemy = EyeLocation - LightOrigin;
    const float DirectDistance = LightToEnemy.Size();
    const float ConeThreshold = FMath::Cos(FMath::DegreesToRadians(
        FMath::Clamp(Light->OuterConeAngle, 0.f, 89.f)));
    if (DirectDistance > 5.f &&
        DirectDistance <= FMath::Min(LightSourceDetectionRange, EffectiveBeamRadius) &&
        FVector::DotProduct(BeamForward, LightToEnemy / DirectDistance) >= ConeThreshold &&
        CanObserveLightPoint(EyeLocation, LightOrigin, Emitter, Player))
    {
        const FVector2D Error = FMath::RandPointInCircle(LightEstimateLocationError);
        OutEstimatedPosition = LightOrigin + FVector(Error.X, Error.Y, 0.f);
        bOutEmitterDirectlyVisible = true;
        OutScore = 2.0f - (DirectDistance / FMath::Max(100.f, LightSourceDetectionRange));
        return true;
    }

    if (!bDetectLightOnSurfaces) return false;

    // Approximate the visible pool of spotlight illumination. This is NOT a
    // readback of the rendered screen, GI, bounced light or Niagara emission.
    const float HalfConeRadians = FMath::DegreesToRadians(
        FMath::Clamp(Light->OuterConeAngle * 0.67f, 1.f, 70.f));
    const float Tangent = FMath::Tan(HalfConeRadians);
    const FVector Right = Light->GetRightVector();
    const FVector Up = Light->GetUpVector();
    const FVector2D Offsets[] = {
        FVector2D(0.f, 0.f), FVector2D(-0.7f, 0.f),
        FVector2D(0.7f, 0.f), FVector2D(0.f, -0.7f), FVector2D(0.f, 0.7f)
    };

    float BestScore = -BIG_NUMBER;
    FVector BestEstimate = FVector::ZeroVector;
    const int32 Count = FMath::Clamp(LightBeamTraceCount, 1, 5);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FVector2D& Offset = Offsets[Index];
        const FVector Direction = (BeamForward +
            Right * (Offset.X * Tangent) + Up * (Offset.Y * Tangent)).GetSafeNormal();

        // Ignore source and its carrier in the forward beam cast only.
        FCollisionQueryParams BeamParams(SCENE_QUERY_STAT(EnemySpotLightSurface), false);
        BeamParams.AddIgnoredActor(Emitter);
        BeamParams.AddIgnoredActor(Player);
        FHitResult SurfaceHit;
        if (!GetWorld()->LineTraceSingleByChannel(
            SurfaceHit, LightOrigin + Direction * 10.f,
            LightOrigin + Direction * EffectiveBeamRadius,
            ECC_Visibility, BeamParams)) continue;

        const AActor* SurfaceActor = SurfaceHit.GetActor();
        if (!IsValid(SurfaceActor) || SurfaceActor == Enemy ||
            SurfaceActor->IsA(APawn::StaticClass())) continue;

        // Strongly grazing hits do not form a reliable visible light patch.
        if (FVector::DotProduct(SurfaceHit.ImpactNormal, -Direction) < 0.15f) continue;

        const FVector Patch = SurfaceHit.ImpactPoint + SurfaceHit.ImpactNormal * 8.f;
        const float ObserverDist = FVector::Dist(EyeLocation, Patch);
        if (ObserverDist > LightPatchDetectionRange ||
            !CanObserveLightPoint(EyeLocation, Patch, Emitter, Player,
                SurfaceActor, SurfaceHit.ImpactNormal))
        {
            continue;
        }

        // If only the pool of light is visible, the source direction/distance
        // is uncertain. Investigate an estimated point on the illuminated side
        // of the surface; NEVER use Player->GetActorLocation().
        const float PatchScore = 1.f - ObserverDist / FMath::Max(100.f, LightPatchDetectionRange);
        if (PatchScore > BestScore)
        {
            BestScore = PatchScore;
            BestEstimate = SurfaceHit.ImpactPoint + SurfaceHit.ImpactNormal * 260.f;
        }
    }

    if (BestScore <= -BIG_NUMBER / 2.f) return false;
    const FVector2D Error = FMath::RandPointInCircle(LightEstimateLocationError);
    OutEstimatedPosition = BestEstimate + FVector(Error.X, Error.Y, 0.f);
    bOutEmitterDirectlyVisible = false;
    OutScore = BestScore;
    return true;
}

void AEnemyBaseAIController::PollVisiblePlayerLights()
{
    AEnemyBase* Enemy = GetEnemyCharacter();
    if (!IsValid(Enemy) || Enemy->IsDead() || !GetWorld()) return;
    UEnemyStateMachineComponent* SM = Enemy->GetStateMachineComponent();
    if (!IsValid(SM) || SM->IsTargetCurrentlyVisible() || SM->IsInDefensiveState() ||
        SM->GetCurrentState() == EEnemyStateType::Dead) return;

    UEnemyLightSourceSubsystem* Registry = GetWorld()->GetSubsystem<UEnemyLightSourceSubsystem>();
    if (!Registry) return;

    TArray<FEnemyRegisteredSpotLight> Sources;
    Registry->GetActivePlayerSpotLights(Sources);

    bool bFound = false;
    float BestScore = -BIG_NUMBER;
    FVector BestEstimate = FVector::ZeroVector;
    bool bBestDirect = false;
    USpotLightComponent* BestLight = nullptr;

    for (const FEnemyRegisteredSpotLight& Entry : Sources)
    {
        const AActor* Player = Entry.PlayerActor.Get();
        const AActor* Emitter = Entry.EmitterActor.Get();
        USpotLightComponent* Light = Entry.Light.Get();
        if (!IsValid(Player) || !IsValid(Emitter) || !IsValid(Light)) continue;

        // Limit event origin to the player's own light sources, never an
        // ambient/world spotlight or a hostile enemy's equipment.
        if (!Player->IsA(ACharacterPlayer::StaticClass()) &&
            !Player->ActorHasTag(TEXT("Player"))) continue;

        FVector Candidate;
        bool bDirect = false;
        float Score = -BIG_NUMBER;
        if (TryObserveSpotLight(Light, Emitter, Player, Candidate, bDirect, Score) &&
            (!bFound || Score > BestScore))
        {
            bFound = true;
            BestScore = Score;
            BestEstimate = Candidate;
            bBestDirect = bDirect;
            BestLight = Light;
        }
    }
    if (!bFound || !BestLight) return;

    const float Now = GetWorld()->GetTimeSeconds();
    const bool bNewLight = LastObservedSpotLight.Get() != BestLight;
    const bool bMovedSignificantly = FVector::DistSquared2D(
        LastSubmittedLightEstimate, BestEstimate) >=
        FMath::Square(LightObservationRetargetDistance);
    if (!bNewLight && !bMovedSignificantly &&
        Now - LastLightSubmissionTime < LightObservationReportCooldown) return;

    LastObservedSpotLight = BestLight;
    LastSubmittedLightEstimate = BestEstimate;
    LastLightSubmissionTime = Now;
    SM->NotifyPlayerLightObserved(BestEstimate, bBestDirect, BestLight->GetOwner());

#if ENABLE_DRAW_DEBUG
    if (bDrawLightObservationDebug)
    {
        DrawDebugSphere(GetWorld(), BestEstimate, 24.f, 8,
            bBestDirect ? FColor::Yellow : FColor::Cyan, false,
            LightObservationInterval + 0.2f);
    }
#endif
}
