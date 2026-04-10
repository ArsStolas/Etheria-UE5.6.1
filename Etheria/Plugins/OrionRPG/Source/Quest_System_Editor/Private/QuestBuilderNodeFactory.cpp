// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "QuestBuilderNodeFactory.h"
#include <EdGraph/EdGraphNode.h>
#include "SGraphNode_QuestBuilderEdge.h"
#include "SGraphNode_QuestBuilderNode.h"
#include "QuestBuilderEdNode.h"
#include "QuestBuilderEdNode_Edge.h"


TSharedPtr<class SGraphNode> FQuestBuilderNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UQuestBuilderEdNode* EdNode_GraphNode = Cast<UQuestBuilderEdNode>(Node))
	{
		return SNew(SGraphNode_QuestBuilderNode, EdNode_GraphNode);
	}
	return nullptr;
}
