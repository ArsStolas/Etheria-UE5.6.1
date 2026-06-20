// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"
#include "CoreMinimal.h"

class FDialogBuilderEditor;
class FExtender;
class FToolBarBuilder;

class DIALOG_SYSTEM_EDITOR_API FDialogBuilderEditorToolbar : public TSharedFromThis<FDialogBuilderEditorToolbar>
{
public:
	FDialogBuilderEditorToolbar(TSharedPtr<FDialogBuilderEditor> InDialogBuilderEditor)
		: DialogBuilderEditor(InDialogBuilderEditor) {}

	void AddModesToolbar(TSharedPtr<FExtender> Extender);
	void AddDialogSystemToolbar(TSharedPtr<FExtender> Extender);

private:
	void FillModesToolbar(FToolBarBuilder& ToolbarBuilder);
	void FillDialogSystemToolbar(FToolBarBuilder& ToolbarBuilder);

protected:
	/** Pointer back to the blueprint editor tool that owns us */
	TWeakPtr<FDialogBuilderEditor> DialogBuilderEditor;
};
