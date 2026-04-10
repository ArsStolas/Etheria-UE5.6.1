// Copyright 2025 Ivan Chandra. All Rights Reserved.UDialogBuilderEdNode_PlayerChoice

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderEdNode.h"
#include "EdGraph/EdGraphNode.h"
#include "DialogBuilderEdNode_PlayerChoice.generated.h"

/**
 * 
 */
UCLASS()
class DIALOG_SYSTEM_EDITOR_API UDialogBuilderEdNode_PlayerChoice : public UDialogBuilderEdNode
{
	GENERATED_BODY()
public:
	UDialogBuilderEdNode_PlayerChoice();
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetBackgroundColor() const override;
	/** Gets a list of actions that can be done to this particular node */
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

};
