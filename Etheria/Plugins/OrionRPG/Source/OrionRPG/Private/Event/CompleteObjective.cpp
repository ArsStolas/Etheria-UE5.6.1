// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Event/CompleteObjective.h"
#include "Quest.h"
#include "QuestBuilderFunctionLibrary.h"
#include "QuestComponent.h"
#include "Engine/World.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderNode_Objective.h"

UCompleteObjective::UCompleteObjective()
{
	bUseQuestTag = true;
	bUseNodeTag = true;
}

void UCompleteObjective::BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn)
{
	if (QuestTag.IsValid())
	{
		if (GetWorld())
		{
			UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController));
			if (QuestComp)
			{
				UQuest* Quest = QuestComp->FindQuest(QuestTag);
				if (Quest)
				{
					if (NodeTag.IsValid())
					{
						UQuestBuilderNode_Objective* ObjectiveNode = Cast<UQuestBuilderNode_Objective>(Quest->NodeMap.FindRef(NodeTag.GetTagName()));
						if(ObjectiveNode)
						{
							ObjectiveNode->CompleteObjective();
						}
					}
				}
			}
		}
	}
	EndEvent();
	
}

FString UCompleteObjective::GetNodeDisplayText_Implementation() const
{
	return FString::Printf(TEXT("Complete Objective %s in Quest %s"), *GetShortTag(NodeTag), *GetShortTag(QuestTag));
}
