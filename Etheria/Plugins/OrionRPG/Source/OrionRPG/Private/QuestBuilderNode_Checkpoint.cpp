// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderNode_Checkpoint.h"
#include "QuestComponent.h"
#include "Quest.h"
#include "Event/OrionEvent.h"


void UQuestBuilderNode_Checkpoint::Initialize(bool bLaunchEventOnLoad)
{
	Super::Initialize(bLaunchEventOnLoad);
}

void UQuestBuilderNode_Checkpoint::BeginNode()
{
	Super::BeginNode();
    BeginCheckpoint();
}

void UQuestBuilderNode_Checkpoint::BeginCheckpoint()
{
	//TODO : Add Delegate On Reached Checkpoint for future use
    QuestComponent->OnQuestUpdated.Broadcast();
	Deinitialize();
}

#if WITH_EDITOR
FText UQuestBuilderNode_Checkpoint::GetNodeTitle() const
{
	return FText::FromString("Checkpoint Node");
}
void UQuestBuilderNode_Checkpoint::SetNodeTitle(const FText& NewTitle)
{
    ID = FName(NewTitle.ToString());
}
FText UQuestBuilderNode_Checkpoint::GetNodeDescription() const
{
	return Description;
}


FLinearColor UQuestBuilderNode_Checkpoint::GetBackgroundColor() const
{
	return FLinearColor();
}
#endif