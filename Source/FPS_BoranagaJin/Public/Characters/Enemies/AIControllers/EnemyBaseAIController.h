

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DetourCrowdAIController.h"
#include "Perception/AIPerceptionTypes.h"
//#include "Enumerations/Enemies/EnemyEnums.h"
#include "EnemyBaseAIController.generated.h"

class UAISenseConfig_Sight;
class AEnemyBase;
class ACharacterPlayer;
/**
 *
 */
UCLASS()
class FPS_BORANAGAJIN_API AEnemyBaseAIController : public AAIController
{
	GENERATED_BODY()

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig = nullptr;

	TWeakObjectPtr<AEnemyBase> CachedPossessedPawn;

	void SetupPerceptionSystem();

	//EEnemyStates _CurrentState;

protected:
	virtual void OnPossess(APawn* PossessedPawn) override;

	UFUNCTION()
	virtual void OnTargetSighted(AActor* SeenTarget, FAIStimulus const Stimulus);

public:
	AEnemyBaseAIController(FObjectInitializer const& ObjectInitializer);

	void InitializeBlackBoard(float StrafeRadius, float ChaseStrafeRadius, float AttackRadius, float AttackRate);

	//FORCEINLINE EEnemyStates GetCurrentState() const { return _CurrentState; }

	//void UpdateCurrentState(EEnemyStates NewState);

	void SetStateToChaseOrPursue(AActor* TargetActor);

	void SetStateToCoopAttack(AActor* Ally, bool bIsThrowing);

	void EndPursueState();

	UFUNCTION(BlueprintCallable)
	ACharacterPlayer* GetAttackTarget();

	bool IsTraversing = false;
};
