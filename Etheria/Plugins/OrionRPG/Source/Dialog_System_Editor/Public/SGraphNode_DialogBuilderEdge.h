// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "Widgets/SCompoundWidget.h"
#include "DialogBuilderEdNode_Edge.h"

/**
 * 
 */
class DIALOG_SYSTEM_EDITOR_API SGraphNode_DialogBuilderEdge : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_DialogBuilderEdge)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, UDialogBuilderEdNode_Edge* InNode);
};
