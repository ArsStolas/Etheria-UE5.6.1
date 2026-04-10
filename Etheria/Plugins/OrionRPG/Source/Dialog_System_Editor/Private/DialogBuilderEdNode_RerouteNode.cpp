// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdNode_RerouteNode.h"
#include "DialogBuilderSetting.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_RerouteNode.h"

UDialogBuilderEdNode_RerouteNode::UDialogBuilderEdNode_RerouteNode()
{
}

void UDialogBuilderEdNode_RerouteNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, "MultipleNode", TEXT("In"));
}

FText UDialogBuilderEdNode_RerouteNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UDialogBuilderNode* MyNode = Cast<UDialogBuilderNode>(NodeInstance);
	if (MyNode != NULL)
	{
		return MyNode->GetNodeTitle();
	}

	return Super::GetNodeTitle(TitleType);
}

FText UDialogBuilderEdNode_RerouteNode::GetTooltipText() const
{
	const UDialogBuilderNode_RerouteNode* RerouteSelectorNode = Cast<UDialogBuilderNode_RerouteNode>(NodeInstance);
	if (RerouteSelectorNode)
	{
		return FText::Format(FText::FromString(TEXT("Reroute Node\n{0}")),
			NSLOCTEXT("DialogBuilderEditor", "RerouteSelectorNodeScopeTooltip", "This Node will reroute/jump back to other node.\nUseful if you want to revisit other node after choice selections"));
	}

	return Super::GetTooltipText();
}

FLinearColor UDialogBuilderEdNode_RerouteNode::GetBackgroundColor() const
{
	return GetDefault<UDialogBuilderSetting>()->RerouteNodeColor;
}

void UDialogBuilderEdNode_RerouteNode::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
}
