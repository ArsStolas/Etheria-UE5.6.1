// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Event/CompleteQuest.h"
#include "Quest.h"
#include "QuestBuilderFunctionLibrary.h"
#include "QuestComponent.h"
#include "Engine/World.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"

UCompleteQuest::UCompleteQuest()
{
	bUseQuestTag = true;
}

void UCompleteQuest::BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn)
{
	if (QuestTag.IsValid())
	{
		if (GetWorld())
		{
			UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController));
			if (QuestComp)
			{
				QuestComp->CompleteQuestFromTag(QuestTag);
			}
		}
	}
	EndEvent();
	
}

FString UCompleteQuest::GetNodeDisplayText_Implementation() const
{
	return FString::Printf(TEXT("Fail Quest :  %s"), *GetShortTag(QuestTag));
}
