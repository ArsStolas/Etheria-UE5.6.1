/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BTTask_PerformAttack - Header
*/

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PerformAttack.generated.h"

UCLASS()
class ETHERIA_API UBTTask_PerformAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PerformAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
