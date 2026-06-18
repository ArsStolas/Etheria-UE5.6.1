// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilderEditorModes.h"

#include "DialogBuilderEditor.h"
#include "DialogBuilderEditorToolbar.h"
#include "HAL/PlatformApplicationMisc.h"
#include "DialogBuilderEditorTabFactories.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/AssertionMacros.h"
#include "Types/SlateEnums.h"

/////////////////////////////////////////////////////
// FDialogBuilderEditorApplicationMode

#define LOCTEXT_NAMESPACE "DialogEditorApplicationMode"

FDialogBuilderEditorApplicationMode::FDialogBuilderEditorApplicationMode(TSharedPtr<class FDialogBuilderEditor> InDialogBuilderEditor)
	: FApplicationMode(FDialogBuilderEditor::DialogEditorMode, FDialogBuilderEditor::GetLocalizedMode)
{
	DialogEditor = InDialogBuilderEditor;

	DialogBuilderEditorTabFactories.RegisterFactory(MakeShareable(new FDialogEditorDetailsSummoner(InDialogBuilderEditor)));


	TabLayout = FTabManager::NewLayout( "Standalone_DialogEditor_Layout_v2" )
	->AddArea
	(
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.2f)
				->AddTab(FDialogBuilderEditorTabs::DialogDefinitionsID, ETabState::ClosedTab)
			)
			/*->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.7f)
				->AddTab("Document1", ETabState::ClosedTab)
				->SetHideTabWell(true)
			)*/
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.2f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(FDialogBuilderEditorTabs::DialogBuilderPropertyID, ETabState::OpenedTab)
				)
			)
		)
	);
	
	//InDialogBuilderEditor->GetToolbarBuilder()->AddModesToolbar(ToolbarExtender);
	InDialogBuilderEditor->GetToolbarBuilder()->AddDialogSystemToolbar(ToolbarExtender);
}

void FDialogBuilderEditorApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	check(DialogEditor.IsValid());
	TSharedPtr<FDialogBuilderEditor> DialogEditorPtr = DialogEditor.Pin();
	
	DialogEditorPtr->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	DialogEditorPtr->PushTabFactories(DialogBuilderEditorTabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FDialogBuilderEditorApplicationMode::PreDeactivateMode()
{
	FApplicationMode::PreDeactivateMode();

	check(DialogEditor.IsValid());
	TSharedPtr<FDialogBuilderEditor> DialogEditorPtr = DialogEditor.Pin();
	
	DialogEditorPtr->SaveEditedObjectState();
}

void FDialogBuilderEditorApplicationMode::PostActivateMode()
{
	// Reopen any documents that were open when the blueprint was last saved
	check(DialogEditor.IsValid());
	TSharedPtr<FDialogBuilderEditor> DialogEditorPtr = DialogEditor.Pin();
	DialogEditorPtr->RestoreDialogEditor();

	FApplicationMode::PostActivateMode();
}

#undef  LOCTEXT_NAMESPACE

/////////////////////////////////////////////////////
// FDialogBuilderSequencerApplicationMode

#define LOCTEXT_NAMESPACE "DialogSequencerApplicationMode"

FDialogBuilderSequencerApplicationMode::FDialogBuilderSequencerApplicationMode(TSharedPtr<class FDialogBuilderEditor> InDialogBuilderEditor)
	: FApplicationMode(FDialogBuilderEditor::DialogSequencerMode, FDialogBuilderEditor::GetLocalizedMode)
{
	DialogEditor = InDialogBuilderEditor;
	
	DialogSequencerTabFactories.RegisterFactory(MakeShareable(new FDialogSequencerTabSummoner(InDialogBuilderEditor)));
	DialogSequencerTabFactories.RegisterFactory(MakeShareable(new FDialogSequencerViewportSummoner(InDialogBuilderEditor)));
	DialogSequencerTabFactories.RegisterFactory(MakeShareable(new FDialogEditorDetailsSummoner(InDialogBuilderEditor)));
	DialogSequencerTabFactories.RegisterFactory(MakeShareable(new DialogDefinitionsSummoner(InDialogBuilderEditor)));
	DialogSequencerTabFactories.RegisterFactory(MakeShareable(new FDialogStageSettingsSummoner(InDialogBuilderEditor)));
	DialogSequencerTabFactories.RegisterFactory(MakeShareable(new FDialogCameraPresetsSummoner(InDialogBuilderEditor)));

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);
	const double DPIScale = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(static_cast<float>(DisplayMetrics.PrimaryDisplayWorkAreaRect.Left), static_cast<float>(DisplayMetrics.PrimaryDisplayWorkAreaRect.Top));

	const float CenterScale = 0.4f;
	const FVector2D DisplaySize(
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Right - DisplayMetrics.PrimaryDisplayWorkAreaRect.Left,
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom - DisplayMetrics.PrimaryDisplayWorkAreaRect.Top);
	const FVector2D WindowSize = (CenterScale * DisplaySize) / DPIScale;

	TabLayout = FTabManager::NewLayout("Standalone_DialogSequencer_Layout_v2")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->Split
				(
					
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
						->SetSizeCoefficient(0.55f)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.85f)
							->SetHideTabWell(true)
							->AddTab(FDialogBuilderEditorTabs::DialogSequencerViewportID, ETabState::OpenedTab)
						)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.15f)
							->AddTab(FDialogBuilderEditorTabs::DialogCameraPresetsID, ETabState::OpenedTab)
						)
						
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.45f)
						->AddTab("Document", ETabState::ClosedTab)
						->AddTab(FDialogBuilderEditorTabs::DialogSequencerTabID, ETabState::OpenedTab)
						->AddTab(FDialogBuilderEditorTabs::DialogStageSettingsID, ETabState::OpenedTab)
					)
				)
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.25f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.55f)
						->AddTab(FDialogBuilderEditorTabs::DialogBuilderPropertyID, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.45f)
						->AddTab(FDialogBuilderEditorTabs::DialogDefinitionsID, ETabState::OpenedTab)
					)
				)
			)
			
		)
		->AddArea
		(
			// Sequencer popup
			FTabManager::NewArea(WindowSize)
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(1.0f)
				->AddTab("SequencerGraphEditor", ETabState::ClosedTab)
			)
		);

	//InDialogBuilderEditor->GetToolbarBuilder()->AddModesToolbar(ToolbarExtender);
}

void FDialogBuilderSequencerApplicationMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	check(DialogEditor.IsValid());
	TSharedPtr<FDialogBuilderEditor> DialogEditorPtr = DialogEditor.Pin();
	
	DialogEditorPtr->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	DialogEditorPtr->PushTabFactories(DialogSequencerTabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FDialogBuilderSequencerApplicationMode::PreDeactivateMode()
{
	FApplicationMode::PreDeactivateMode();

	check(DialogEditor.IsValid());
	TSharedPtr<FDialogBuilderEditor> DialogEditorPtr = DialogEditor.Pin();
	DialogEditorPtr->SaveEditedObjectState();

}

void FDialogBuilderSequencerApplicationMode::PostActivateMode()
{
	check(DialogEditor.IsValid());
	TSharedPtr<FDialogBuilderEditor> DialogEditorPtr = DialogEditor.Pin();
	DialogEditorPtr->RestoreDialogEditor();

	FApplicationMode::PostActivateMode();
}

#undef LOCTEXT_NAMESPACE
