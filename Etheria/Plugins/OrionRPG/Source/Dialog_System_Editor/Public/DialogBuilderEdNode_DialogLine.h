// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderEdNode.h"
#include "DialogBuilderEdNode_DialogLine.generated.h"

/**
 * 
 */
UCLASS()
class DIALOG_SYSTEM_EDITOR_API UDialogBuilderEdNode_DialogLine : public UDialogBuilderEdNode
{
	GENERATED_BODY()
public:
	UDialogBuilderEdNode_DialogLine();
public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetBackgroundColor() const override;
	virtual FText GetTooltipText() const override;
	virtual void PostPasteNode() override;
	virtual void PostPlacedNewNode() override;
	virtual void NodeConnectionListChanged() override;
	/** Gets a list of actions that can be done to this particular node */
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	UFUNCTION()
	void OnSharedParticipantChanged();

	void OnPackageMarkedDirty(UPackage* ModifiedPackage, bool bWasDirty);
};
