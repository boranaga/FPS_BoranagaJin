#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyBaseAIController.generated.h"

class AEnemyBase;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

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
	UAIPerceptionComponent* AIPerceptionComponent;

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus);
	void HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus);
public:
	AEnemyBase* GetEnemyCharacter() const;

	void MoveToTarget(AActor* Target);
	void MoveToLocationPoint(const FVector& Location, float AcceptanceRadius = 50.f);
	void StopAIMovement();

	void DisablePerception();
};