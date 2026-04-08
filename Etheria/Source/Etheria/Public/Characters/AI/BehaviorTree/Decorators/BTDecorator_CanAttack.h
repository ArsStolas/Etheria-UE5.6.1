/*
* Etheria's End Project, 2025
* BTDecorator_CanAttack - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CanAttack.generated.h"

/** Succès si cible valide, boss a un CombatComponent, pas en attaque/cooldown, et à portée. */
UCLASS()
class ETHERIA_API UBTDecorator_CanAttack : public UBTDecorator
{
	GENERATED_BODY()

public:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
