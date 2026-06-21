// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Enemies/Tasks/BTT_MoveToTarget.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTT_MoveToTarget::UBTT_MoveToTarget()
{
	NodeName = "Move To Target";
	TargetLocationKey.AddVectorFilter(this, "TargetLocation");
}

EBTNodeResult::Type UBTT_MoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
		return EBTNodeResult::Failed;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
		return EBTNodeResult::Failed;

	FVector TargetLocation = BlackboardComp->GetValueAsVector(TargetLocationKey.SelectedKeyName);

	FAIRequestID RequestID = AIController->MoveToLocation(TargetLocation);

	return EBTNodeResult::Succeeded;
}

void UBTT_MoveToTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
		TargetLocationKey.ResolveSelectedKey(*BBAsset);
}
