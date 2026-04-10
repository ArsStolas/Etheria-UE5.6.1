// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Objective/CompleteAnObjective.h"
#include "Quest.h"
#include "QuestComponent.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderNode_Objective.h"

UCompleteAnObjective::UCompleteAnObjective()
{
}

void UCompleteAnObjective::BeginObjective()
{
	if (QuestComponent && !QuestComponent->OnQuestObjectiveUpdated.IsAlreadyBound(this, &ThisClass::EvaluateObjectiveCompleted))
	{
		QuestComponent->OnQuestObjectiveUpdated.AddDynamic(this, &ThisClass::EvaluateObjectiveCompleted);
		EvaluateObjectiveCompleted(nullptr);
	}
	Super::BeginObjective();
}

void UCompleteAnObjective::CompleteObjective()
{
	if (QuestComponent)
	{
		QuestComponent->OnQuestObjectiveUpdated.RemoveDynamic(this, &ThisClass::EvaluateObjectiveCompleted);
	}
	Super::CompleteObjective();
}

void UCompleteAnObjective::EvaluateObjectiveCompleted(UQuest* InQuest)
{
	if (GetOwningQuest())
	{
		UQuestBuilderNode* FoundedNode = GetOwningQuest()->NodeMap.FindRef(ObjectiveTag.GetTagName());
		UQuestBuilderNode_Objective* ObjectiveNode = FoundedNode ? Cast<UQuestBuilderNode_Objective>(FoundedNode) : nullptr;
		if (ObjectiveNode && ObjectiveNode->ObjectiveState == EObjectiveState::E_Completed)
		{
			CompleteObjective();
		}
	}
}
#if WITH_EDITOR
FText UCompleteAnObjective::GetNodeTitle() const
{
	return FText::FromString(FString::Printf(TEXT("Complete an Objective :  %s"), *GetShortTag(ObjectiveTag)));
}
#endif
