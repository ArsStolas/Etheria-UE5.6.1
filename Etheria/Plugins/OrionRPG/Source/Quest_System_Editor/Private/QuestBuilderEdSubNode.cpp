// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEdSubNode.h"
#include "QuestBuilderSetting.h"

UQuestBuilderEdSubNode::UQuestBuilderEdSubNode()
{
	bIsReadOnly = true;
	bCanRenameNode = false;
	bIsSubNode = true;
}

void UQuestBuilderEdSubNode::AllocateDefaultPins()
{
	//subnode, no pins
}

FText UQuestBuilderEdSubNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("QuestBuilderEditor", "SubNode", "Subnode");
	
}

FLinearColor UQuestBuilderEdSubNode::GetBackgroundColor() const
{
	return GetDefault<UQuestBuilderSetting>()->SubNodeColor;
}
