/*
* Etheria's End Project, 2025
* BTTask_BossAttack - Source
*/

#include "Characters/AI/BehaviorTree/Tasks/BTTask_BossAttack.h"
#include "Characters/AI/BehaviorTree/Blackboards/BossBlackboardData.h"
#include "Characters/AI/Enemies/Enemies_Type/BaseBoss.h"
#include "Components/Combat/CombatComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_BossAttack::UBTTask_BossAttack()
{
	NodeName = TEXT("BossAttack");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_BossAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!BB || !Pawn) return EBTNodeResult::Failed;

	ABaseBoss* Boss = Cast<ABaseBoss>(Pawn);
	UCombatComponent* Combat = Boss ? Boss->FindComponentByClass<UCombatComponent>() : nullptr;
	AActor* Target = Cast<AActor>(BB->GetValueAsObject(BossBlackboardKeys::TargetActor));
	if (!Combat || !Target)
	{
		return EBTNodeResult::Failed;
	}

	Combat->SetExternalTarget(Target);
	ACharacter* Char = Cast<ACharacter>(Pawn);
	if (Char && Target)
	{
		FRotator Desired = UKismetMathLibrary::FindLookAtRotation(Char->GetActorLocation(), Target->GetActorLocation());
		Char->SetActorRotation(FRotator(0.f, Desired.Yaw, 0.f));
	}
	if (Combat->TryAttackPrimary())
	{
		return EBTNodeResult::InProgress;
	}
	return EBTNodeResult::Failed;
}

void UBTTask_BossAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	if (!Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	UCombatComponent* Combat = Pawn->FindComponentByClass<UCombatComponent>();
	if (!Combat || !Combat->IsAttackActive())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
