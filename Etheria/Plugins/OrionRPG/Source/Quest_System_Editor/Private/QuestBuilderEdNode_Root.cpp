// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEdNode_Root.h"
#include "QuestBuilderNode_Root.h"
#include "QuestBuilderSetting.h"

UQuestBuilderEdNode_Root::UQuestBuilderEdNode_Root()
{
	bIsReadOnly = true;
	bCanRenameNode = false;
}

void UQuestBuilderEdNode_Root::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, "MultipleNode", TEXT("In"));
}

FText UQuestBuilderEdNode_Root::GetTooltipText() const
{
	const UQuestBuilderNode_Root* RootNode = Cast<UQuestBuilderNode_Root>(NodeInstance);
	if (RootNode)
	{
		return FText::Format(FText::FromString(TEXT("Root Node\n\n{0}")),
			NSLOCTEXT("QuestBuilderEditor", "RootNodeScopeTooltip", "Entry point for your quest!"));
	}
	return Super::GetTooltipText();
}

FText UQuestBuilderEdNode_Root::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("QuestEditor", "Quest Start", "Quest Start");
	
}

FLinearColor UQuestBuilderEdNode_Root::GetBackgroundColor() const
{
	return GetDefault<UQuestBuilderSetting>()->RootNodeColor;
}
