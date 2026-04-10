// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderNode_Root.h"
#include "QuestComponent.h"
#include "QuestBuilderNode.h"

void UQuestBuilderNode_Root::BeginNode()
{
	Super::BeginNode();
	Quest->UpdateQuest();
	Deinitialize();
}

#if WITH_EDITOR

FText UQuestBuilderNode_Root::GetNodeDescription() const
{
	return FText::FromString(TEXT("Root Node"));
}
#endif