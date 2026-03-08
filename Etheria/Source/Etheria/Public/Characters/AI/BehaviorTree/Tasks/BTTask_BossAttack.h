/*
* Etheria's End Project, 2025
* BTTask_BossAttack - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_BossAttack.generated.h"

/** Lance une attaque primaire vers la cible du Blackboard, termine quand l'attaque est finie. */
UCLASS()
class ETHERIA_API UBTTask_BossAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_BossAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
