/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: EliteAIController - Source
*/

#include "Core/AIConfig/EliteEnemy/EliteAiController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/AI/Enemies/Enemies_Type/BaseElite.h"

void AEliteAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABaseElite* ElitePawn = Cast<ABaseElite>(InPawn);
	if (!ElitePawn) return;

	if (BehaviorTreeAsset)
	{
		if (BehaviorTreeAsset->BlackboardAsset)
		{
			BlackboardComponent = NewObject<UBlackboardComponent>(this);
			BlackboardComponent->RegisterComponent();
			BlackboardComponent->InitializeBlackboard(*BehaviorTreeAsset->BlackboardAsset);
		}
		RunBehaviorTree(BehaviorTreeAsset);
	}
}
