// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Event/RestartQuest.h"
#include "Engine/World.h"
#include "Quest.h"
#include "QuestComponent.h"
#include "QuestBuilderFunctionLibrary.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderNode_Objective.h"
#include "QuestBuilderNode_Root.h"

URestartQuest::URestartQuest()
{
	bUseQuestTag = true;
	bUseNodeTag = true;
}

void URestartQuest::BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn)
{
	if (QuestTag.IsValid())
	{
		if (GetWorld())
		{
			UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController));
			if (QuestComp)
			{
				QuestComp->RestartQuestFromTag(QuestTag, NodeTag);
			}
		}
	}
	EndEvent();
}

FString URestartQuest::GetNodeDisplayText_Implementation() const
{
    if(NodeTag.IsValid())
    {  
		return FString::Printf(TEXT("Restart Quest :  %s From %s"), *GetShortTag(QuestTag), *GetShortTag(NodeTag));
	}
	else
	{
		return FString::Printf(TEXT("Restart Quest :  %s"), *GetShortTag(QuestTag));
	}

}

