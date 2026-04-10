// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Event/UnlockQuest.h"
#include "Quest.h"
#include "QuestBuilderFunctionLibrary.h"
#include "QuestComponent.h"
#include "Engine/World.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"

UUnlockQuest::UUnlockQuest()
{
	bUseQuestTag = true;
}

void UUnlockQuest::BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn)
{
	if (QuestTag.IsValid())
	{
		if (GetWorld())
		{
			UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController));
			if (QuestComp)
			{
				QuestComp->UnlockQuestFromTag(QuestTag);
			}
		}
	}
	EndEvent();
	
}

FString UUnlockQuest::GetNodeDisplayText_Implementation() const
{
	return FString::Printf(TEXT("Unlock Quest :  %s"), *GetShortTag(QuestTag));
}
