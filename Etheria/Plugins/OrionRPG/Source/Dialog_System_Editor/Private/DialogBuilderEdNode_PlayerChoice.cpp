// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdNode_PlayerChoice.h"
#include "DialogBuilderSetting.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_PlayerChoice.h"

UDialogBuilderEdNode_PlayerChoice::UDialogBuilderEdNode_PlayerChoice()
{
}

FText UDialogBuilderEdNode_PlayerChoice::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UDialogBuilderNode* MyNode = Cast<UDialogBuilderNode>(NodeInstance);
	if (MyNode != NULL)
	{
		return MyNode->GetNodeTitle();
	}

	return Super::GetNodeTitle(TitleType);
}

FText UDialogBuilderEdNode_PlayerChoice::GetTooltipText() const
{
	const UDialogBuilderNode_PlayerChoice* PlayerChoiceNode = Cast<UDialogBuilderNode_PlayerChoice>(NodeInstance);
	if (PlayerChoiceNode)
	{
		return FText::Format(FText::FromString(TEXT("Player Choice Node\n{0}")),
			NSLOCTEXT("DialogBuilderEditor", "PlayerOptionNodeScopeTooltip", "This Node contain data for player choices.\n**Right click the node to start adding a subnodes."));
	}

	return Super::GetTooltipText();
}

FLinearColor UDialogBuilderEdNode_PlayerChoice::GetBackgroundColor() const
{
	return GetDefault<UDialogBuilderSetting>()->PlayerOptionNodeColor;
}

void UDialogBuilderEdNode_PlayerChoice::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	AddContextMenuActionsDecorators(Menu, "DialogBuilderEdNode", Context);
}
