// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Objective/CompleteAQuest.h"
#include "Quest.h"
#include "QuestComponent.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"

UCompleteAQuest::UCompleteAQuest()
{
}

void UCompleteAQuest::BeginObjective()
{
    if (QuestComponent && !QuestComponent->OnQuestCompleted.IsAlreadyBound(this, &ThisClass::EvaluateQuestCompleted))
    {
		QuestComponent->OnQuestCompleted.AddDynamic(this, &ThisClass::EvaluateQuestCompleted);
    }

	UQuest* FoundedQuest = QuestComponent->QuestMap.FindRef(QuestTag.GetTagName());
	if (FoundedQuest)
		EvaluateQuestCompleted(FoundedQuest);
	
	Super::BeginObjective();
}
void UCompleteAQuest::CompleteObjective()
{
	if (QuestComponent)
	{
		QuestComponent->OnQuestCompleted.RemoveDynamic(this, &ThisClass::EvaluateQuestCompleted);
	}
	Super::CompleteObjective();
}
void UCompleteAQuest::EvaluateQuestCompleted(UQuest* InQuest)
{
	if (InQuest && InQuest->QuestState == EQuestState::E_Complete && QuestTag.GetTagName() == InQuest->ID)
	{
		CompleteObjective();
	}
}



#if WITH_EDITOR
FText UCompleteAQuest::GetNodeTitle() const
{
	return FText::FromString(FString::Printf(TEXT("Complete a Quest :  %s"), *GetShortTag(QuestTag)));
}
#endif