// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BTD_IsWithinIdealRadius.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API UBTD_IsWithinIdealRadius : public UBTDecorator
{
	GENERATED_BODY()

protected:
	virtual auto CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const -> bool override;
	
public:
	UBTD_IsWithinIdealRadius();

	UPROPERTY(EditAnywhere, Category = "Condition")
	FBlackboardKeySelector IdealRadiusKey;
};
