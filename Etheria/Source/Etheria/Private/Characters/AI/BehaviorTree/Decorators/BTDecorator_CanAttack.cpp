/*
* Etheria's End Project, 2025
* BTDecorator_CanAttack - Source
*/

#include "Characters/AI/BehaviorTree/Decorators/BTDecorator_CanAttack.h"

#include "AIController.h"
#include "Characters/AI/BehaviorTree/Blackboards/BossBlackboardData.h"
#include "Characters/AI/Enemies/Enemies_Type/BaseBoss.h"
#include "Components/Combat/CombatComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"

bool UBTDecorator_CanAttack::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(BossBlackboardKeys::TargetActor));
	if (!IsValid(Target)) return false;

	APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	ABaseBoss* Boss = Cast<ABaseBoss>(Pawn);
	if (!Boss) return false;

	UCombatComponent* Combat = Boss->FindComponentByClass<UCombatComponent>();
	if (!Combat || Combat->IsAttackActive()) return false;

	const float Dist = FVector::Dist2D(Pawn->GetActorLocation(), Target->GetActorLocation());
	return Dist <= Combat->GetCurrentAttackRange();
}
