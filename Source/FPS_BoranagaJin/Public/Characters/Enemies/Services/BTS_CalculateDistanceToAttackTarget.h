// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTS_CalculateDistanceToAttackTarget.generated.h"

class AEnemyBase;
class ACharacterPlayer;
/**
 * 
 */
UCLASS()
class FPS_BORANAGAJIN_API UBTS_CalculateDistanceToAttackTarget : public UBTService_BlackboardBase
{
	GENERATED_BODY()

	UPROPERTY()
	AEnemyBase* OwnerEnemy = nullptr;

	UPROPERTY()
	ACharacterPlayer* TargetPlayer = nullptr;
	
protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override; // called only once when the node is entered by the BT
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

public:
	UBTS_CalculateDistanceToAttackTarget();
};
