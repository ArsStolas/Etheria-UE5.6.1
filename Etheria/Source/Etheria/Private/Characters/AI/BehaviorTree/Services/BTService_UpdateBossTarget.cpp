/*
* Etheria's End Project, 2025
* BTService_UpdateBossTarget - Source
*/

#include "Characters/AI/BehaviorTree/Services/BTService_UpdateBossTarget.h"
#include "Characters/AI/BehaviorTree/Blackboards/BossBlackboardData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

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

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	BB->SetValueAsObject(BossBlackboardKeys::TargetActor, Player);
}
