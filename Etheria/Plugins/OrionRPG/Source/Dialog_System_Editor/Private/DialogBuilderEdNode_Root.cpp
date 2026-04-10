// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdNode_Root.h"
#include "DialogBuilderNode_Root.h"
#include "DialogBuilderSetting.h"

UDialogBuilderEdNode_Root::UDialogBuilderEdNode_Root()
{
	bIsReadOnly = true;
	bCanRenameNode = false;
}

void UDialogBuilderEdNode_Root::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, "MultipleNode", TEXT("In"));
}

FText UDialogBuilderEdNode_Root::GetTooltipText() const
{
	const UDialogBuilderNode_Root* RootNode = Cast<UDialogBuilderNode_Root>(NodeInstance);
	if (RootNode)
	{
		return FText::Format(FText::FromString(TEXT("Dialog Start Node\n\n{0}")),
			NSLOCTEXT("DialogBuilderEditor", "RootNodeScopeTooltip", "Entry point for your dialog!"));
	}
	return Super::GetTooltipText();
}

FText UDialogBuilderEdNode_Root::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("DialogEditor", "DialogStart", "Dialog Start");
	
}

FLinearColor UDialogBuilderEdNode_Root::GetBackgroundColor() const
{
	return GetDefault<UDialogBuilderSetting>()->RootNodeColor;
}
