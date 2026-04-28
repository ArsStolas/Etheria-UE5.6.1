// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Event/ActivateQuest.h"
#include "Quest.h"
#include "QuestComponent.h"
#include "Engine/World.h"
#include "QuestBuilderFunctionLibrary.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"

UActivateQuest::UActivateQuest()
{
	bUseQuestTag = true;
}

void UActivateQuest::BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn)
{
	if (QuestTag.IsValid())
	{
		if (GetWorld())
		{
			UQuestComponent* QuestComp =  UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController));
			if (QuestComp)
			{
				QuestComp->ActivateQuestFromTag(QuestTag);
			}
		}
	}
	EndEvent();
	
}

FString UActivateQuest::GetNodeDisplayText_Implementation() const
{
	return FString::Printf(TEXT("Activate Quest :  %s"), *GetShortTag(QuestTag));
}
