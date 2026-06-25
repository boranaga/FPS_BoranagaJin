#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyBaseAIController.generated.h"

class AEnemyBase;

UCLASS()
class FPS_BORANAGAJIN_API AEnemyBaseAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyBaseAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

public:
	AEnemyBase* GetEnemyCharacter() const;

	void MoveToTarget(AActor* Target);
	void MoveToLocationPoint(const FVector& Location);
	void StopAIMovement();
};