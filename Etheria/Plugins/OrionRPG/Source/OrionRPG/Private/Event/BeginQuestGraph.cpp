// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Event/BeginQuestGraph.h"
#include "Quest.h"
#include "QuestBuilderFunctionLibrary.h"
#include "GameFramework/PlayerController.h"
#include "QuestComponent.h"
#include "Engine/World.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"

UBeginQuestGraph::UBeginQuestGraph()
{
}

void UBeginQuestGraph::BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn)
{
	if (GetWorld())
	{
		UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController));
		if (QuestComp)
		{
			QuestComp->BeginQuestGraph(QuestAsset);
		}
	}
	EndEvent();
	
	
}

FString UBeginQuestGraph::GetNodeDisplayText_Implementation() const
{
	if (QuestAsset)
	{
		return FString::Printf(TEXT("Begin Quest Graph :  %s"), *QuestAsset->GetName());
	}
	else
	{
		return FString("Begin Quest Graph");
	}
}
