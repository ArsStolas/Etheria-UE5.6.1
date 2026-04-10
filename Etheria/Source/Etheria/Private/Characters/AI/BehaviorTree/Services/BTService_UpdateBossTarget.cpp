/*
* Etheria's End Project, 2025
* BTService_UpdateBossTarget - Source
*/

#include "Characters/AI/BehaviorTree/Services/BTService_UpdateBossTarget.h"
#include "Characters/AI/BehaviorTree/Blackboards/BossBlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "Characters/AI/Controllers/AIController_Boss.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"

UBTService_UpdateBossTarget::UBTService_UpdateBossTarget()
{
	Interval = 0.25f;
	RandomDeviation = 0.f;
	NodeName = TEXT("UpdateBossTarget");
}

void UBTService_UpdateBossTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	AAIController_Boss* AIController = Cast<AAIController_Boss>(OwnerComp.GetAIOwner());
	if (!AIController) return;

	UAIPerceptionComponent* PerceptionComp = AIController->GetPerceptionComponent();
	if (!PerceptionComp) return;

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return;

	TArray<AActor*> PerceivedActors;
	PerceptionComp->GetKnownPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	bool bSeesPlayer = PerceivedActors.Contains(Player);
	if (bSeesPlayer)
	{
		BB->SetValueAsObject(BossBlackboardKeys::TargetActor, Player);
	}
	else
	{
		BB->ClearValue(BossBlackboardKeys::TargetActor);
	}
}
