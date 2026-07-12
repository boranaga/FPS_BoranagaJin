
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/EnemyStateMachineComponent.h"


#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Navigation/PathFollowingComponent.h"

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

	SightConfig->SightRadius = 1700.f;
	SightConfig->LoseSightRadius = 2000.f;
	SightConfig->PeripheralVisionAngleDegrees = 140.f;
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
		/*
		 * 도망/회복 중에는 시야를 잃는 것이 성공적인 상황이므로
		 * Target을 즉시 제거하면 안 된다.
		 * 은신 판정에 플레이어 위치가 계속 필요하다.
		 */
		if (!StateMachine->IsInDefensiveState())
		{
			StateMachine->ClearTarget();
			StateMachine->SetState(EEnemyStateType::Patrol);
		}
	}
}

void AEnemyBaseAIController::HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	AEnemyBase* Enemy = GetEnemyCharacter();
	if (!Enemy || Enemy->IsDead()) return;

	UEnemyStateMachineComponent* StateMachine = Enemy->GetStateMachineComponent();
	if (!StateMachine) return;

	const FVector HeardLocation = Stimulus.StimulusLocation;

	StateMachine->SetLastHeardLocation(HeardLocation);
	StateMachine->SetState(EEnemyStateType::Investigate);
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
	MoveRequest.SetAcceptanceRadius(120.f);
	MoveRequest.SetUsePathfinding(true);

	FNavPathSharedPtr NavPath;
	MoveTo(MoveRequest, &NavPath);
}

void AEnemyBaseAIController::MoveToLocationPoint(const FVector& Location, float AcceptanceRadius)
{
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(Location);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
	MoveRequest.SetUsePathfinding(true);

	FNavPathSharedPtr NavPath;
	MoveTo(MoveRequest, &NavPath);
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
