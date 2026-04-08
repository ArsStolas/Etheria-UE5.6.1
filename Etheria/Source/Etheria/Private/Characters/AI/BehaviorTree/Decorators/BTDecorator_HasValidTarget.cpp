/*
* Etheria's End Project, 2025
* BTDecorator_HasValidTarget - Source
*/

#include "Characters/AI/BehaviorTree/Decorators/BTDecorator_HasValidTarget.h"
#include "Characters/AI/BehaviorTree/Blackboards/BossBlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

bool UBTDecorator_HasValidTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(BossBlackboardKeys::TargetActor));
	return IsValid(Target);
}
