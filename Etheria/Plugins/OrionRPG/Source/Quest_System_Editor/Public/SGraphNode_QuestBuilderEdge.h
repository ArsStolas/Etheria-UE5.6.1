// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "Widgets/SCompoundWidget.h"
#include "QuestBuilderEdNode_Edge.h"

/**
 * 
 */
class QUEST_SYSTEM_EDITOR_API SGraphNode_QuestBuilderEdge : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_QuestBuilderEdge)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, UQuestBuilderEdNode_Edge* InNode);
};
