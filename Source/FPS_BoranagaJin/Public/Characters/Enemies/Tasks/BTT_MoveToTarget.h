// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTT_MoveToTarget.generated.h"

/**
 * 
 */
USTRUCT()
struct FMoveToTargetCache
{
	GENERATED_BODY()

	FTimerHandle TimerHandle;
	
	TWeakObjectPtr<UBehaviorTreeComponent> OwnerComp;
	
	TWeakObjectPtr<AAIController> AIController;
	
	float RemainingWaitTime = 0.f;
	
	FDelegateHandle MoveCompleteDelegateHandle;
	
	bool bIsWaitingForPath = false;
};

UCLASS()
class FPS_BORANAGAJIN_API UBTT_MoveToTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTT_MoveToTarget();

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float WaitTime = 5.0f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.1"))
	float CheckInterval = 0.5f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float AcceptableRadius = 50.f;
	
protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetLocationKey;
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
