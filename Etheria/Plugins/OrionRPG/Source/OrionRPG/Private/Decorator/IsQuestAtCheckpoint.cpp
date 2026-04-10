// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Decorator/IsQuestAtCheckpoint.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderNode_Objective.h"
#include "QuestBuilderNode_Checkpoint.h"
#include "QuestBuilderFunctionLibrary.h"
#include "QuestComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Quest.h"
#include "QuestBuilderGraph.h"
#include "Engine/World.h"

UIsQuestAtCheckpoint::UIsQuestAtCheckpoint()
{
    bUseQuestTag = true;
    bUseNodeTag = true;
}

bool UIsQuestAtCheckpoint::PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const
{
    FName QuestID = QuestTag.GetTagName();
    FName NodeID = NodeTag.GetTagName();
    if (GetWorld())
    {
        if (UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController)))
        {
            //find the first found checkpoint and break
            if (UQuest* FoundedQuest = QuestComp->QuestMap.FindRef(QuestID))
            {
                for (int32 i = FoundedQuest->VisitedNodeIDs.Num() - 1; i >= 0; --i)
                {
                    const FName& id = FoundedQuest->VisitedNodeIDs[i];
                    UQuestBuilderNode* FoundedNode = FoundedQuest->NodeMap.FindRef(id);
                    if (FoundedNode)
                    {
                        if (UQuestBuilderNode_Checkpoint* CheckpointNode = Cast<UQuestBuilderNode_Checkpoint>(FoundedNode))
                        {
                            if (CheckpointNode->ID == NodeID)
                            {
                                return true;
                            }
                            else
                            {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

FString UIsQuestAtCheckpoint::GetNodeDisplayText_Implementation() const
{
    return FString::Printf(TEXT("In Quest: %s \nIn Checkpoint: %s"), *GetShortTag(QuestTag), *GetShortTag(NodeTag));
}
