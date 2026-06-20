// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"
#include "WorkflowOrientedApp/ApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

/** Application mode for main Dialog editing mode */
class FDialogBuilderEditorApplicationMode : public FApplicationMode
{
public:
	FDialogBuilderEditorApplicationMode(TSharedPtr<class FDialogBuilderEditor> InDialogBuilderEditor);

	virtual void RegisterTabFactories(TSharedPtr<class FTabManager> InTabManager) override;
	virtual void PreDeactivateMode() override;
	virtual void PostActivateMode() override;

protected:
	TWeakPtr<class FDialogBuilderEditor> DialogEditor;

	// Set of spawnable tabs in behavior tree editing mode
	FWorkflowAllowedTabSet DialogBuilderEditorTabFactories;
};

/** Application mode for dialog sequencer editing mode */
class FDialogBuilderSequencerApplicationMode : public FApplicationMode
{
public:
	FDialogBuilderSequencerApplicationMode(TSharedPtr<class FDialogBuilderEditor> InDialogBuilderEditor);

	virtual void RegisterTabFactories(TSharedPtr<class FTabManager> InTabManager) override;
	virtual void PreDeactivateMode() override;
	virtual void PostActivateMode() override;

protected:
	TWeakPtr<class FDialogBuilderEditor> DialogEditor;

	// Set of spawnable tabs in blackboard mode
	FWorkflowAllowedTabSet DialogSequencerTabFactories;
};
