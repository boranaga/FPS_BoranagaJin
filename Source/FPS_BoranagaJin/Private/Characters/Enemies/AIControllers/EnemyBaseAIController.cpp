
#include "Characters/Enemies/AIControllers/EnemyBaseAIController.h"
#include "Characters/Enemies/EnemyBase.h"


//#include "BehaviorTree/BehaviorTree.h"

//#include "Perception/AIPerceptionComponent.h"
//#include "Perception/AISenseConfig_Sight.h"
//#include "BehaviorTree/BlackboardComponent.h"
//#include "Characters/Enemies/CharacterEnemyTurret.h"
#include "Navigation/CrowdFollowingComponent.h"
//#include "Structures/Enemies/EnemyAttributesData.h"

AEnemyBaseAIController::AEnemyBaseAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEnemyBaseAIController::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyBaseAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
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