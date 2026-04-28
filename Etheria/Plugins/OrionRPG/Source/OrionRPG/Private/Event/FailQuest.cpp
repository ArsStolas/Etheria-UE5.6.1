// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Event/FailQuest.h"
#include "Quest.h"
#include "Engine/World.h"
#include "QuestBuilderFunctionLibrary.h"
#include "QuestComponent.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"

UFailQuest::UFailQuest()
{
	bUseQuestTag = true;
}

void UFailQuest::BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn)
{
	if (QuestTag.IsValid())
	{
		if (GetWorld())
		{
			UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController));
			if (QuestComp)
			{
				QuestComp->FailQuestFromTag(QuestTag);
			}
		}
	}
	EndEvent();
	
}

FString UFailQuest::GetNodeDisplayText_Implementation() const
{
	return FString::Printf(TEXT("Fail Quest :  %s"), *GetShortTag(QuestTag));
}
