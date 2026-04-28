// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilderNodeFactory.h"
#include <EdGraph/EdGraphNode.h>
#include "SGraphNode_DialogBuilderEdge.h"
#include "SGraphNode_DialogBuilderNode.h"
#include "DialogBuilderEdNode.h"
#include "DialogBuilderEdNode_Edge.h"


TSharedPtr<class SGraphNode> FDialogBuilderNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UDialogBuilderEdNode* EdNode_GraphNode = Cast<UDialogBuilderEdNode>(Node))
	{
		return SNew(SGraphNode_DialogBuilderNode, EdNode_GraphNode);
	}
	return nullptr;
}
