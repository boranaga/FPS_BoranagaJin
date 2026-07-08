
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Enemies/EnemyStateMachineComponent.h"


#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

#include "Navigation/PathFollowingComponent.h"

//#include "Navigation/CrowdFollowingComponent.h"

//#include "Characters/Enemies/CharacterEnemyTurret.h"
//#include "Structures/Enemies/EnemyAttributesData.h"



AEnemyBaseAIController::AEnemyBaseAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(
		TEXT("AIPerceptionComponent")
	);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(
		TEXT("SightConfig")
	);

	SightConfig->SightRadius = 1700.f;
	SightConfig->LoseSightRadius = 2000.f;
	SightConfig->PeripheralVisionAngleDegrees = 140.f;
	SightConfig->SetMaxAge(10.f);

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
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
	AEnemyBase* Enemy = GetEnemyCharacter();
	if (!Enemy) return;

	UEnemyStateMachineComponent* StateMachine = Enemy->GetStateMachineComponent();
	if (!StateMachine) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		StateMachine->SetTarget(Actor);
		StateMachine->SetState(EEnemyStateType::Chase);
	}
	else
	{
		StateMachine->ClearTarget();
		StateMachine->SetState(EEnemyStateType::Idle);
	}
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

void AEnemyBaseAIController::MoveToLocationPoint(const FVector& Location)
{
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(Location);
	MoveRequest.SetAcceptanceRadius(50.f);
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
