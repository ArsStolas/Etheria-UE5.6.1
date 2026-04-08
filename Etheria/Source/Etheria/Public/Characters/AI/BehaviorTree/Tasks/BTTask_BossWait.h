/*
* Etheria's End Project, 2025
* BTTask_BossWait - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_BossWait.generated.h"

/** Attend un temps configurable puis réussit. */
UCLASS()
class ETHERIA_API UBTTask_BossWait : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_BossWait();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(float); }

	UPROPERTY(EditAnywhere, Category = "Task", meta = (ClampMin = "0.0"))
	float WaitTime = 1.f;
};
