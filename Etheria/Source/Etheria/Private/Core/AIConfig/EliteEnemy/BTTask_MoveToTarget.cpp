/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BTTask_MoveToTarget - Source
*/

#include "Core/AIConfig/EliteEnemy/BTTask_MoveToTarget.h"
#include "AIController.h"
#include "Characters/AI/Enemies/BaseEnemy.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_MoveToTarget::UBTTask_MoveToTarget()
{
	NodeName = "Move To Target";
}

EBTNodeResult::Type UBTTask_MoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon) return EBTNodeResult::Failed;

	ABaseEnemy* Enemy = Cast<ABaseEnemy>(AICon->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	Enemy->StartChasePlayer();

	return EBTNodeResult::Succeeded;
}