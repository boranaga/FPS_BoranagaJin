
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/EnemyStateMachineComponent.h"


#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"

//#include "Perception/AISense_Sight.h"
//#include "Perception/AISense_Hearing.h"

AEnemyBaseAIController::AEnemyBaseAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(
		TEXT("AIPerceptionComponent")
	);

	// <Sight>
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SightConfig->SightRadius = 2000.f;
	SightConfig->LoseSightRadius = 5000.f;
	SightConfig->PeripheralVisionAngleDegrees = 120.f;
	SightConfig->SetMaxAge(10.f);

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	// <Hearing>
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 2000.f;
	HearingConfig->LoSHearingRange = 3000.f;
	HearingConfig->SetMaxAge(5.f);

	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

	// <PerceptionComponent>
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
		AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
			this,
			&AEnemyBaseAIController::OnTargetPerceptionUpdated
		);
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
	if (!Enemy || Enemy->IsDead()) return;

	UEnemyStateMachineComponent* StateMachine = Enemy->GetStateMachineComponent();
	if (!StateMachine) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		StateMachine->SetTarget(Actor);

		if (StateMachine->IsInDefensiveState())
		{
			StateMachine->NotifyThreatSeen(Actor);
			return;
		}

		StateMachine->SetState(EEnemyStateType::Chase);
	}
	else
	{
		///*
		// * 도망/회복 중에는 시야를 잃는 것이 성공적인 상황이므로
		// * Target을 즉시 제거하면 안 된다.
		// * 은신 판정에 플레이어 위치가 계속 필요하다.
		// */
		//if (!StateMachine->IsInDefensiveState())
		//{
		//	StateMachine->ClearTarget();
		//	StateMachine->SetState(EEnemyStateType::Patrol);
		//}

		StateMachine->OnTargetMissed();
	}
}

void AEnemyBaseAIController::HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	AEnemyBase* Enemy = GetEnemyCharacter();
	if (!Enemy || Enemy->IsDead()) return;

	UEnemyStateMachineComponent* StateMachine = Enemy->GetStateMachineComponent();
	if (!StateMachine) return;

	if (StateMachine->GetCurrentState() == EEnemyStateType::Chase
		|| StateMachine->GetCurrentState() == EEnemyStateType::Attack)
	{
		return;
	}

	if (!StateMachine->IsInDefensiveState())
	{
		const FVector HeardLocation = Stimulus.StimulusLocation;

		StateMachine->SetLastHeardLocation(HeardLocation);
		StateMachine->SetState(EEnemyStateType::Investigate);
	}
}

bool AEnemyBaseAIController::FindGroundNavLocation(const FVector& InputLocation, FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	if (!World) { return false; }

	const FVector TraceStart = InputLocation + FVector::UpVector * GroundTraceHeight;
	const FVector TraceEnd = InputLocation - FVector::UpVector * GroundTraceDepth;

	//FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AIMoveGroundTrace), false);
	FCollisionQueryParams QueryParams;

	if (const APawn* ControlledPawn = GetPawn())
	{
		QueryParams.AddIgnoredActor(ControlledPawn);
	}

	FHitResult GroundHit;

	const bool bGroundHit = World->LineTraceSingleByChannel(
		GroundHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	if (!bGroundHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindGroundNavLocation: Ground not found. Input: %s"), *InputLocation.ToString());
		return false;
	}

	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(World);

	if (!NavigationSystem)
	{
		UE_LOG(LogTemp, Error,TEXT("FindGroundNavLocation: NavigationSystem is null"));
		return false;
	}

	FNavLocation ProjectedLocation;

	const bool bProjected = NavigationSystem->ProjectPointToNavigation(
			GroundHit.ImpactPoint,
			ProjectedLocation,
			NavProjectionExtent
		);

	if (!bProjected)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindGroundNavLocation: Failed to project ground point " "to NavMesh. Ground: %s"),*GroundHit.ImpactPoint.ToString());

		return false;
	}

	OutLocation = ProjectedLocation.Location;
	return true;
}

AEnemyBase* AEnemyBaseAIController::GetEnemyCharacter() const
{
	return Cast<AEnemyBase>(GetPawn());
}

void AEnemyBaseAIController::MoveToTarget(AActor* Target)
{
	if (!Target) return;

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalActor(Target);
	MoveRequest.SetAcceptanceRadius(50.f);
	MoveRequest.SetUsePathfinding(true);

	FNavPathSharedPtr NavPath;
	MoveTo(MoveRequest, &NavPath);
}

void AEnemyBaseAIController::MoveToLocationPoint(const FVector& Location, float AcceptanceRadius)
{
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(Location);
	MoveRequest.SetProjectGoalLocation(true);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
	MoveRequest.SetUsePathfinding(true);

	FNavPathSharedPtr NavPath;
	FPathFollowingRequestResult PathFollowingRequestResult = MoveTo(MoveRequest, &NavPath);

	if (!NavPath)
	{
		//UE_LOG(LogTemp, Error, TEXT("No Path!!!!!!!!!"));
	}
}

void AEnemyBaseAIController::MoveToLocationPoint_Upgrade(const FVector& Location, float AcceptanceRadius)
{
	FVector CorrectedLocation;

	if (!FindGroundNavLocation(Location, CorrectedLocation))
	{
		UE_LOG(LogTemp, Error, TEXT("MoveToLocationPoint: Failed to correct destination. ""Input: %s"), *Location.ToString());
		return;
	}

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(CorrectedLocation);
	MoveRequest.SetAcceptanceRadius(FMath::Max(0.f, AcceptanceRadius));
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetProjectGoalLocation(false);
	MoveRequest.SetAllowPartialPath(false);

	FNavPathSharedPtr NavPath;
	const FPathFollowingRequestResult MoveResult = MoveTo(MoveRequest, &NavPath);

	//switch (MoveResult.Code)
	//{
	//case EPathFollowingRequestResult::RequestSuccessful:
	//	UE_LOG(LogTemp, Error, TEXT("Move request succeeded. Input: %s, Corrected: %s"), *Location.ToString(), *CorrectedLocation.ToString());
	//	break;

	//case EPathFollowingRequestResult::AlreadyAtGoal:
	//	UE_LOG(LogTemp, Error, TEXT("Already at goal: %s"), *CorrectedLocation.ToString());
	//	break;

	//case EPathFollowingRequestResult::Failed:
	//default:
	//	UE_LOG(LogTemp, Error, TEXT("Move request failed. Input: %s, Corrected: %s"), *Location.ToString(), *CorrectedLocation.ToString());
	//	break;
	//}

	if (!NavPath.IsValid() && MoveResult.Code == EPathFollowingRequestResult::RequestSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("Move request succeeded but NavPath is invalid"));
	}
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
