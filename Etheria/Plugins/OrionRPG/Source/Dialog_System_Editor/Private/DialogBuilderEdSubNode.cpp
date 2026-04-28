// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdSubNode.h"
#include "DialogBuilderSetting.h"

UDialogBuilderEdSubNode::UDialogBuilderEdSubNode()
{
	bIsReadOnly = true;
	bCanRenameNode = false;
	bIsSubNode = true;
}

void UDialogBuilderEdSubNode::AllocateDefaultPins()
{
	//subnode, no pins
}

FText UDialogBuilderEdSubNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("DialogBuilderEditor", "SubNode", "Subnode");
	
}

FLinearColor UDialogBuilderEdSubNode::GetBackgroundColor() const
{
	return GetDefault<UDialogBuilderSetting>()->SubNodeColor;
}
