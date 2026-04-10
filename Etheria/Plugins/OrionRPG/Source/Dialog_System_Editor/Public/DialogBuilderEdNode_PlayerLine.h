// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderEdNode_DialogLine.h"
#include "DialogBuilderEdNode.h"
#include "DialogBuilderEdNode_PlayerLine.generated.h"

/**
 * 
 */
UCLASS()
class DIALOG_SYSTEM_EDITOR_API UDialogBuilderEdNode_PlayerLine : public UDialogBuilderEdNode_DialogLine
{
	GENERATED_BODY()
public:
	UDialogBuilderEdNode_PlayerLine();
public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetBackgroundColor() const override;
	virtual FText GetTooltipText() const override;
	/** Gets a list of actions that can be done to this particular node */
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;


};
