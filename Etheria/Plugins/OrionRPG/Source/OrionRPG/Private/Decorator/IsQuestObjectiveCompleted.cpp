// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Decorator/IsQuestObjectiveCompleted.h"
#include "QuestBuilderNode_Objective.h"
#include "QuestComponent.h"
#include "QuestBuilderFunctionLibrary.h"
#include "Quest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "QuestBuilderGraph.h"
#include "Engine/World.h"

UIsQuestObjectiveCompleted::UIsQuestObjectiveCompleted()
{
    bUseQuestTag = true;
	bUseNodeTag = true;
}

bool UIsQuestObjectiveCompleted::PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const
{
    FName QuestID = QuestTag.GetTagName();
    FName NodeID = NodeTag.GetTagName();
    if (GetWorld())
    {
        if (UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController)))
        {
            if (UQuest* FoundedQuest = QuestComp->QuestMap.FindRef(QuestID))
            {
                UQuestBuilderNode* FoundedNode = FoundedQuest->NodeMap.FindRef(NodeID);
                if (UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(FoundedNode))
                {
                    return ObjectiveNode->IsObjectiveCompleted();
                }
            }
        }
    }
    
    return false;
}

FString UIsQuestObjectiveCompleted::GetNodeDisplayText_Implementation() const
{
    return FString::Printf(TEXT("In Quest: %s \nIn Objective: %s"), *GetShortTag(QuestTag), *GetShortTag(NodeTag));
}
