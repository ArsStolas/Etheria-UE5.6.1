// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdNode_PlayerLine.h"
#include "DialogBuilderSetting.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_PlayerLine.h"

UDialogBuilderEdNode_PlayerLine::UDialogBuilderEdNode_PlayerLine()
{
}

FText UDialogBuilderEdNode_PlayerLine::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UDialogBuilderNode* MyNode = Cast<UDialogBuilderNode>(NodeInstance);
	if (MyNode != NULL)
	{
		return MyNode->GetNodeTitle();
	}

	return Super::GetNodeTitle(TitleType);
}

FText UDialogBuilderEdNode_PlayerLine::GetTooltipText() const
{
	const UDialogBuilderNode_PlayerLine* PlayerLineNode = Cast< UDialogBuilderNode_PlayerLine>(NodeInstance);
	if (PlayerLineNode)
	{
		return FText::Format(FText::FromString(TEXT("Player Line Node\n{0}")),
			NSLOCTEXT("DialogBuilderEditor", "PlayerLineNodeScopeTooltip", "This Node contain the data for player lines.\n**Right click the node to start adding a subnodes."));
	}

	return Super::GetTooltipText();
}

FLinearColor UDialogBuilderEdNode_PlayerLine::GetBackgroundColor() const
{
	return GetDefault<UDialogBuilderSetting>()->PlayerLineNodeColor;
}

void UDialogBuilderEdNode_PlayerLine::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	AddContextMenuActionsEvents(Menu, "DialogBuilderEdNode", Context);
	AddContextMenuActionsDecorators(Menu, "DialogBuilderEdNode", Context);
}
