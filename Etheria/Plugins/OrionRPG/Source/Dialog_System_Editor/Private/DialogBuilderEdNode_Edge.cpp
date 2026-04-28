// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdNode_Edge.h"
#include "DialogBuilderEdge.h"
#include "DialogBuilderEdNode.h"

#define LOCTEXT_NAMESPACE "EdNode_DialogSystemGraphEdge"

UDialogBuilderEdNode_Edge::UDialogBuilderEdNode_Edge()
{
	bCanRenameNode = true;
}

void UDialogBuilderEdNode_Edge::SetEdge(UDialogBuilderEdge* Edge)
{
	DialogSystemGraphEdge = Edge;
}

void UDialogBuilderEdNode_Edge::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, TEXT("Edge"), FName(), TEXT("In"));
	Inputs->bHidden = true;
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, TEXT("Edge"), FName(), TEXT("Out"));
	Outputs->bHidden = true;
}

FText UDialogBuilderEdNode_Edge::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (DialogSystemGraphEdge)
	{
		return DialogSystemGraphEdge->GetNodeTitle();
	}
	return FText();
}

void UDialogBuilderEdNode_Edge::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin->LinkedTo.Num() == 0)
	{
		// Commit suicide; transitions must always have an input and output connection
		Modify();

		// Our parent graph will have our graph in SubGraphs so needs to be modified to record that.
		if (UEdGraph* ParentGraph = GetGraph())
		{
			ParentGraph->Modify();
		}

		DestroyNode();
	}
}

void UDialogBuilderEdNode_Edge::PrepareForCopying()
{
	DialogSystemGraphEdge->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UDialogBuilderEdNode_Edge::CreateConnections(UDialogBuilderEdNode* Start, UDialogBuilderEdNode* End)
{
	Pins[0]->Modify();
	Pins[0]->LinkedTo.Empty();

	Start->GetOutputPin()->Modify();
	Pins[0]->MakeLinkTo(Start->GetOutputPin());

	// This to next
	Pins[1]->Modify();
	Pins[1]->LinkedTo.Empty();

	End->GetInputPin()->Modify();
	Pins[1]->MakeLinkTo(End->GetInputPin());
}

UDialogBuilderEdNode* UDialogBuilderEdNode_Edge::GetStartNode()
{
	if (Pins[0]->LinkedTo.Num() > 0)
	{
		return Cast<UDialogBuilderEdNode>(Pins[0]->LinkedTo[0]->GetOwningNode());
	}
	else
	{
		return nullptr;
	}
}

UDialogBuilderEdNode* UDialogBuilderEdNode_Edge::GetEndNode()
{
	if (Pins[1]->LinkedTo.Num() > 0)
	{
		return Cast<UDialogBuilderEdNode>(Pins[1]->LinkedTo[0]->GetOwningNode());
	}
	else
	{
		return nullptr;
	}
}

#undef LOCTEXT_NAMESPACE