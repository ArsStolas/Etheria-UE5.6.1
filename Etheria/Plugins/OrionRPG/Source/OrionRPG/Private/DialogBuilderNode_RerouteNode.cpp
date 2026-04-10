// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderNode_RerouteNode.h"
#include "DialogBuilderNode_PlayerChoice.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogBuilderNode_Root.h"
#include "DialogComponent.h"
#include "DialogBuilderGraph.h"
#include "Event/OrionEvent.h"



UDialogBuilderNode_RerouteNode::UDialogBuilderNode_RerouteNode()
{
	RerouteRule = EDialogRerouteRule::E_BackToLastSelection;
	bPlayDialogLine = false;
}

void UDialogBuilderNode_RerouteNode::BeginNode()
{
	Super::BeginNode();
	switch (RerouteRule)
	{
		case EDialogRerouteRule::E_BackToLastSelection:
		{
			bool bSuccess = false;
			if (UDialogBuilderNode_DialogLine* LatestRootSelectionNode = Cast<UDialogBuilderNode_DialogLine>(GetOwningDialogGraph()->LatestRootSelectionNode))
			{
				if (bPlayDialogLine)
				{
					GetOwningDialogGraph()->BeginNode(LatestRootSelectionNode);
					return;
				}
				else
				{
					GetOwningDialogGraph()->CurrentNode = LatestRootSelectionNode;
					bSuccess = LatestRootSelectionNode->EvaluateAnyPlayerOptions();
				}
			}
			if (!bSuccess)
			{
				GetDialogComponent()->OnEndDialog.Broadcast(GetOwningDialogGraph());
			}
			break;
		}
		case EDialogRerouteRule::E_GoToNode:
		{
			if (UDialogBuilderNode* FoundedNode = GetOwningDialogGraph()->NodeMap.FindRef(NodeID))
			{
				GetOwningDialogGraph()->BeginNode(FoundedNode);
			}
			else
			{
				GetDialogComponent()->OnEndDialog.Broadcast(GetOwningDialogGraph());
			}
			break;
		}
	}

	Deinitialize();
}

TArray<FName> UDialogBuilderNode_RerouteNode::GetNodeListToReroute()
{
	TArray<FName> NodeList;
	for (auto& Node : GetOwningDialogGraph()->AllNodes)
	{
		if (Node->IsA(UDialogBuilderNode_RerouteNode::StaticClass()) ||
			Node->IsA(UDialogBuilderNode_Root::StaticClass()))
			continue;

		NodeList.Add(Node->ID);
	}

	//sort in alphabetical order
	NodeList.Sort([](const FName& A, const FName& B) {
		return A.LexicalLess(B);
		});
	return NodeList;
}

#if WITH_EDITOR
FText UDialogBuilderNode_RerouteNode::GetNodeTitle() const
{
    return FText::FromName(ID).IsEmpty() ? FText::FromString(UDialogBuilderNode::GetShortTypeName(this)) : FText::FromName(ID);
}
void UDialogBuilderNode_RerouteNode::SetNodeTitle(const FText& NewTitle)
{
    ID = FName(NewTitle.ToString());
}
FText UDialogBuilderNode_RerouteNode::GetNodeDescription() const
{
    FString RerouteDesc;
	switch (RerouteRule)
	{
	case EDialogRerouteRule::E_BackToLastSelection:
		RerouteDesc = "Go back to last selection.";
		break;
	case EDialogRerouteRule::E_GoToNode:
		RerouteDesc = "Go to node: " + NodeID.ToString();
		break;
	}
    return FText::FromString(RerouteDesc);
}

#endif