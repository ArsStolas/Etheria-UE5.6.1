// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdNode_DialogSequence.h"
#include "DialogBuilderEdNode_PlayerChoice.h"
#include "DialogBuilderNode_DialogSequence.h"
#include "DialogBuilderSetting.h"
#include "DialogBuilderGraph.h"
#include "DialogStage.h"
#include "DialogBuilderNode.h"



UDialogBuilderEdNode_DialogSequence::UDialogBuilderEdNode_DialogSequence()
{
	GetOutermost()->PackageMarkedDirtyEvent.AddUObject(this, &UDialogBuilderEdNode_DialogSequence::OnPackageMarkedDirty);
}

FText UDialogBuilderEdNode_DialogSequence::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const UDialogBuilderNode* DialogNode = Cast<UDialogBuilderNode>(NodeInstance);

	if (DialogNode != NULL)
	{
		return DialogNode->GetNodeTitle();
	}
	else if (!ClassData.GetClassName().IsEmpty())
	{
		FString StoredClassName = ClassData.GetClassName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));

		return FText::Format(NSLOCTEXT("DialogGraph", "NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
	}

	return Super::GetNodeTitle(TitleType);
}

FLinearColor UDialogBuilderEdNode_DialogSequence::GetBackgroundColor() const
{
	return GetDefault<UDialogBuilderSetting>()->DialogSequenceNodeColor;
}

FText UDialogBuilderEdNode_DialogSequence::GetTooltipText() const
{
	const UDialogBuilderNode_DialogSequence* DialogSequenceNode = Cast<UDialogBuilderNode_DialogSequence>(NodeInstance);
	if (DialogSequenceNode)
	{
		return FText::Format(FText::FromString(TEXT("Dialog Line Node\n{0}")),
			NSLOCTEXT("DialogBuilderEditor", "SpeakerNodeScopeTooltip", "This Node contain the sequencer tailored for dialog"));
	}

	return Super::GetTooltipText();
}

void UDialogBuilderEdNode_DialogSequence::PostPasteNode()
{
	Super::PostPasteNode();
	UDialogBuilderNode_DialogSequence* DialogSequenceNode = Cast<UDialogBuilderNode_DialogSequence>(NodeInstance);
	if (DialogSequenceNode)
	{
		if (UDialogBuilderGraph* DialogGraph = DialogSequenceNode->GetOwningDialogGraph())
		{
			for (auto& DialogStage : DialogGraph->DialogStages)
			{
				UDialogStage* CurrentTemplateStage = DialogSequenceNode->DialogStageToUse;
				if (DialogStage && CurrentTemplateStage)
				{
					if (CurrentTemplateStage->Name.ToString() == DialogStage->Name.ToString())
					{
						DialogSequenceNode->DialogStageToUse = DialogStage;
						break;
					}
				}
			}
		}
	}
	GetGraph()->NotifyGraphChanged();
}

void UDialogBuilderEdNode_DialogSequence::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();
	UDialogBuilderNode_DialogSequence* DialogSequenceNode = Cast<UDialogBuilderNode_DialogSequence>(NodeInstance);
	if (DialogSequenceNode )
	{
		DialogSequenceNode->EnsureSequenceCreated();
	}
	GetGraph()->NotifyGraphChanged();
}

void UDialogBuilderEdNode_DialogSequence::NodeConnectionListChanged()
{
	
}

void UDialogBuilderEdNode_DialogSequence::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	AddContextMenuActionsEvents(Menu, "DialogBuilderEdNode", Context);
	AddContextMenuActionsDecorators(Menu, "DialogBuilderEdNode", Context);
}

void UDialogBuilderEdNode_DialogSequence::OnSharedParticipantChanged()
{
	GetGraph()->NotifyGraphChanged();
}

void UDialogBuilderEdNode_DialogSequence::OnPackageMarkedDirty(UPackage* ModifiedPackage, bool bWasDirty)
{
	UDialogBuilderNode_DialogSequence* DialogSequenceNode = Cast<UDialogBuilderNode_DialogSequence>(NodeInstance);
	if (DialogSequenceNode)
	{
	}
}