/*
* Etheria's End Project, 2025
* BTTask_BossMoveToTarget - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_BossMoveToTarget.generated.h"

/** Démarre un déplacement vers la cible (Blackboard), rafraîchit la destination régulièrement pour suivre le joueur. */
UCLASS()
class ETHERIA_API UBTTask_BossMoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_BossMoveToTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(float); }

	UPROPERTY(EditAnywhere, Category = "Task", meta = (ClampMin = "50", ClampMax = "300"))
	float AcceptRadius = 120.f;

	/** Intervalle (secondes) entre chaque mise à jour de la destination vers la cible. Plus bas = suit mieux le joueur. */
	UPROPERTY(EditAnywhere, Category = "Task", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PathRefreshInterval = 0.2f;
};
