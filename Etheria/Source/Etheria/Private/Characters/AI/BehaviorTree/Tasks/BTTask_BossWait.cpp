/*
* Etheria's End Project, 2025
* BTTask_BossWait - Source
*/

#include "Characters/AI/BehaviorTree/Tasks/BTTask_BossWait.h"

UBTTask_BossWait::UBTTask_BossWait()
{
	NodeName = TEXT("BossWait");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_BossWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	float* Remaining = reinterpret_cast<float*>(NodeMemory);
	*Remaining = WaitTime;
	return EBTNodeResult::InProgress;
}

void UBTTask_BossWait::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	float* Remaining = reinterpret_cast<float*>(NodeMemory);
	*Remaining -= DeltaSeconds;
	if (*Remaining <= 0.f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
