/*
* Etheria's End Project, 2025
* BTDecorator_HasValidTarget - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasValidTarget.generated.h"

/** Succès si le Blackboard a une cible (TargetActor) valide et vivante. */
UCLASS()
class ETHERIA_API UBTDecorator_HasValidTarget : public UBTDecorator
{
	GENERATED_BODY()

public:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
