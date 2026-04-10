// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FQuestBuilderEditor;
class FExtender;
class FToolBarBuilder;

class QUEST_SYSTEM_EDITOR_API FQuestBuilderEditorToolbar : public TSharedFromThis<FQuestBuilderEditorToolbar>
{
public:
	FQuestBuilderEditorToolbar(TSharedPtr<FQuestBuilderEditor> InQuestBuilderEditor)
		: QuestSystemEditor(InQuestBuilderEditor) {}

	void AddQuestSystemToolbar(TSharedPtr<FExtender> Extender);

private:
	void FillQuestSystemToolbar(FToolBarBuilder& ToolbarBuilder);

protected:
	/** Pointer back to the blueprint editor tool that owns us */
	TWeakPtr<FQuestBuilderEditor> QuestSystemEditor;
};
