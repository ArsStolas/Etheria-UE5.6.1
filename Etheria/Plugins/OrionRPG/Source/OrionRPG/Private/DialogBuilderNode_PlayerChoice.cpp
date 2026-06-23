// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilderNode_PlayerChoice.h"
#include "DialogBuilderGraph.h"
#include "DialogComponent.h"
#include "Event/OrionEvent.h"

UDialogBuilderNode_PlayerChoice::UDialogBuilderNode_PlayerChoice()
{
	SelectionType = EDialogSelectionType::E_Sequence;
	SelectionTimeLimit = ESelectionTimeLimit::E_NoTimeLimit;
	TimeLimit = 5.0f; 
	PlaybackSettings.LoopCount.Value = -1;
}

void UDialogBuilderNode_PlayerChoice::BeginNode()
{
	//Super::BeginNode();
	if (this->IsA(UDialogBuilderNode_PlayerChoice::StaticClass()))
	{
		GetOwningDialogGraph()->LatestRootSelectionNode = this;
	}
	GetOwningDialogGraph()->BeginChoiceSelection(this);
	GetDialogComponent()->OnEnterChoiceSelection.Broadcast(this);
}

void UDialogBuilderNode_PlayerChoice::SelectChoice(int32 ChoiceIndex)
{
	UE_LOG(LogTemp, Log, TEXT("PlayerChoice selected: ChoiceIndex = %d, Node = %s"), ChoiceIndex, *GetNameSafe(this));
	SelectedChoiceIndex = ChoiceIndex;
	EvaluateNextNode();
}

bool UDialogBuilderNode_PlayerChoice::IsChoiceVisible(int32 ChoiceIndex)
{
	TArray<TObjectPtr<UDialogBuilderNode>>& NodesToBranch = ChoiceBranches[ChoiceIndex].ChildrenNodes;

	//Node is empty, choosing this will end the dialog
	if (NodesToBranch.IsEmpty()) return true;

	bool bVisible = false;

	for (auto& NodeToBranch : NodesToBranch)
	{
		//Node will not visible if they have connected nodes, but cant branch to any of them
		if (NodeToBranch->DecoratorConditionMet())
		{
			bVisible = true;
			break;
		}
	}

	return bVisible;
}

void UDialogBuilderNode_PlayerChoice::DetermineNextNode()
{
	if (ChoiceBranches.IsValidIndex(SelectedChoiceIndex))
	{
		TArray<TObjectPtr<UDialogBuilderNode>>& NodesToBranch = ChoiceBranches[SelectedChoiceIndex].ChildrenNodes;
		for (auto& NodeToBranch : NodesToBranch)
		{
			if (NodeToBranch->DecoratorConditionMet())
			{
				NextNode = NodeToBranch;
				break;
			}
		}
	}
	
}


#if WITH_EDITOR

FText UDialogBuilderNode_PlayerChoice::GetNodeTitle() const
{
	return FText::FromString("Player Choice");
}

void UDialogBuilderNode_PlayerChoice::SetNodeTitle(const FText& NewTitle)
{
	ID = FName(NewTitle.ToString());
}

FText UDialogBuilderNode_PlayerChoice::GetNodeDescription() const
{
	return FText();
}

void UDialogBuilderNode_PlayerChoice::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty)
	{
		const FName PropertyName = PropertyChangedEvent.MemberProperty->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_PlayerChoice, ChoiceList))
		{
			OnPlayerChoiceDataChanged.Broadcast();
		}
	}
}

void UDialogBuilderNode_PlayerChoice::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.PropertyChain.GetHead() && PropertyChangedEvent.PropertyChain.GetHead()->GetValue())
	{
		const FName PropertyName = PropertyChangedEvent.PropertyChain.GetHead()->GetValue()->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UDialogBuilderNode_PlayerChoice, ChoiceList))
		{
			OnPlayerChoiceDataChanged.Broadcast();
		}
	}
}
#endif