// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Enemies/Decorators/BTD_IsWithinIdealRadius.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Enemies/EnemyBase.h"
#include "Characters/Player/PlayerMovementComponent.h"

UBTD_IsWithinIdealRadius::UBTD_IsWithinIdealRadius()
{
	NodeName = "IsWithinIdealRadius";

	IdealRadiusKey.AddFloatFilter(this, "");
}

bool UBTD_IsWithinIdealRadius::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (AEnemyBase* const Enemy = Cast<AEnemyBase>(OwnerComp.GetAIOwner()->GetCharacter()))
	{
		if (OwnerComp.GetBlackboardComponent()->GetValueAsFloat("DistanceToAttackTarget") <= OwnerComp.GetBlackboardComponent()->GetValueAsFloat(IdealRadiusKey.SelectedKeyName))
		{
			// UE_LOG(LogTemp, Error, TEXT("Distance within ideal radius: %f"), OwnerComp.GetBlackboardComponent()->GetValueAsFloat(IdealRadiusKey.SelectedKeyName));
			return true;
		}
	}

	return false;
}