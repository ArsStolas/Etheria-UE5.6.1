/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BTTask_PerformAttack - Source
*/

#include "Core/AIConfig/EliteEnemy/BTTask_PerformAttack.h"
#include "AIController.h"
#include "Characters/AI/Enemies/Enemies_Type/BaseElite.h"

UBTTask_PerformAttack::UBTTask_PerformAttack()
{
	NodeName = "Perform Attack";
}

EBTNodeResult::Type UBTTask_PerformAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	if (!AICon) return EBTNodeResult::Failed;

	ABaseElite* Enemy = Cast<ABaseElite>(AICon->GetPawn());
	if (!Enemy) return EBTNodeResult::Failed;

	if (!Enemy->TargetActor)
		return EBTNodeResult::Failed;

	AICon->StopMovement();

	if (Enemy->CurrentState != EEnemyState::Fighting)
		Enemy->CurrentState = EEnemyState::Fighting;

	if (!Enemy->bIsAttacking)
		Enemy->StartAttackCycle();

	return EBTNodeResult::Succeeded;
}