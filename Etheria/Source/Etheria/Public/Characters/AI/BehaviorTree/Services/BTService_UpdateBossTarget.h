/*
* Etheria's End Project, 2025
* BTService_UpdateBossTarget - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateBossTarget.generated.h"

/** Met à jour la clé TargetActor du Blackboard avec le joueur (index 0). Interval défini dans le constructeur (0.25s). */
UCLASS()
class ETHERIA_API UBTService_UpdateBossTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateBossTarget();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
