// Copyright 2025 Ivan Chandra. All Rights Reserved.UDialogBuilderEdNode_PlayerChoice

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderEdNode.h"
#include "EdGraph/EdGraphNode.h"
#include "DialogBuilderEdNode_PlayerChoice.generated.h"

UCLASS()
class DIALOG_SYSTEM_EDITOR_API UDialogBuilderEdNode_PlayerChoice : public UDialogBuilderEdNode
{
	GENERATED_BODY()
public:
	UDialogBuilderEdNode_PlayerChoice();

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetBackgroundColor() const override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;

	static FName GetChoiceOutputPinName(int32 ChoiceIndex);
	static int32 GetChoiceIndexFromPin(const UEdGraphPin* Pin);

	virtual void PostPasteNode() override;
	virtual void PostPlacedNewNode() override;
#if WITH_EDITOR
	virtual void PostLoad() override;
	virtual void PostEditUndo() override;
	virtual void PostEditImport() override;
#endif
	virtual void DestroyNode() override;

private:
	void BindChoiceChangeDelegate();
	void UnbindChoiceChangeDelegate();
	void HandleChoiceDataChanged();
};
