// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEdNode_Edge.h"
#include "QuestBuilderEdge.h"
#include "QuestBuilderEdNode.h"

#define LOCTEXT_NAMESPACE "EdNode_QuestSystemGraphEdge"

UQuestBuilderEdNode_Edge::UQuestBuilderEdNode_Edge()
{
	bCanRenameNode = true;
}

void UQuestBuilderEdNode_Edge::SetEdge(UQuestBuilderEdge* Edge)
{
	QuestSystemGraphEdge = Edge;
}

void UQuestBuilderEdNode_Edge::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, TEXT("Edge"), FName(), TEXT("In"));
	Inputs->bHidden = true;
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, TEXT("Edge"), FName(), TEXT("Out"));
	Outputs->bHidden = true;
}

FText UQuestBuilderEdNode_Edge::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (QuestSystemGraphEdge)
	{
		return QuestSystemGraphEdge->GetNodeTitle();
	}
	return FText();
}

void UQuestBuilderEdNode_Edge::PinConnectionListChanged(UEdGraphPin* Pin)
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

void UQuestBuilderEdNode_Edge::PrepareForCopying()
{
	QuestSystemGraphEdge->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UQuestBuilderEdNode_Edge::CreateConnections(UQuestBuilderEdNode* Start, UQuestBuilderEdNode* End)
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

UQuestBuilderEdNode* UQuestBuilderEdNode_Edge::GetStartNode()
{
	if (Pins[0]->LinkedTo.Num() > 0)
	{
		return Cast<UQuestBuilderEdNode>(Pins[0]->LinkedTo[0]->GetOwningNode());
	}
	else
	{
		return nullptr;
	}
}

UQuestBuilderEdNode* UQuestBuilderEdNode_Edge::GetEndNode()
{
	if (Pins[1]->LinkedTo.Num() > 0)
	{
		return Cast<UQuestBuilderEdNode>(Pins[1]->LinkedTo[0]->GetOwningNode());
	}
	else
	{
		return nullptr;
	}
}

#undef LOCTEXT_NAMESPACE