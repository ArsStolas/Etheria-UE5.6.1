// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEdNode_Checkpoint.h"
#include "QuestBuilderSetting.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderNode_Checkpoint.h"

UQuestBuilderEdNode_Checkpoint::UQuestBuilderEdNode_Checkpoint()
{
}


FText UQuestBuilderEdNode_Checkpoint::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UQuestBuilderNode* MyNode = Cast<UQuestBuilderNode>(NodeInstance);
	if (MyNode != NULL)
	{
		return MyNode->GetNodeTitle();
	}

	return Super::GetNodeTitle(TitleType);
}

FText UQuestBuilderEdNode_Checkpoint::GetTooltipText() const
{
	const UQuestBuilderNode_Checkpoint* CheckpointNode = Cast<UQuestBuilderNode_Checkpoint>(NodeInstance);
	if (CheckpointNode)
	{
		return FText::Format(FText::FromString(TEXT("Checkpoint Node\n{0}")),
			NSLOCTEXT("QuestBuilderEditor", "StateNodeScopeTooltip", "This Node serves as the checkpoint for the quest.\nUseful if we want to know that some quest has reached a certain checkpoint.\nThis Node will executes their children from top to bottom in sequence\nIf none of the children succeeds, this node will stop branching until next quest update."));
	}

	return Super::GetTooltipText();
}

FLinearColor UQuestBuilderEdNode_Checkpoint::GetBackgroundColor() const
{
	return GetDefault<UQuestBuilderSetting>()->CheckpointNodeColor;
}

void UQuestBuilderEdNode_Checkpoint::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	AddContextMenuActionsEvents(Menu, "QuestBuilderEdNode", Context);
	AddContextMenuActionsDecorators(Menu, "QuestBuilderEdNode", Context);
}
