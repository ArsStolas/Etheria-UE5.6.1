// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilderEditorTabFactories.h"

#include "DialogBuilderEditor.h"
#include "DialogBuilderGraph.h"
#include "Containers/Array.h"
#include "Engine/Blueprint.h"
#include "GraphEditor.h"
#include "HAL/PlatformCrt.h"
#include "Internationalization/Internationalization.h"
#include "Math/Vector2D.h"
#include "Misc/AssertionMacros.h"
#include "Styling/AppStyle.h"
#include "Styling/ISlateStyle.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Docking/SDockTab.h"

class SWidget;
struct FSlateBrush;

#define LOCTEXT_NAMESPACE "DialogBuilderEditorTabFactories"

FDialogEditorDetailsSummoner::FDialogEditorDetailsSummoner(TSharedPtr<class FDialogBuilderEditor> InDialogEditorPtr)
	: FWorkflowTabFactory(FDialogBuilderEditorTabs::DialogBuilderPropertyID, InDialogEditorPtr)
	, DialogEditorPtr(InDialogEditorPtr)
{
	TabLabel = LOCTEXT("DialogEditorDetailsLabel", "Details");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("DialogEditorDetailsView", "Details");
	ViewMenuTooltip = LOCTEXT("DialogEditorDetailsView_ToolTip", "Show the details view");
}

TSharedRef<SWidget> FDialogEditorDetailsSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	check(DialogEditorPtr.IsValid());
	return DialogEditorPtr.Pin()->SpawnProperties();
}

FText FDialogEditorDetailsSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("DialogEditorDetailsTabTooltip", "The dialog editor details tab allows editing of the properties of dialog nodes");
}

FDialogCameraPresetsSummoner::FDialogCameraPresetsSummoner(TSharedPtr<class FDialogBuilderEditor> InDialogEditorPtr)
	: FWorkflowTabFactory(FDialogBuilderEditorTabs::DialogCameraPresetsID, InDialogEditorPtr)
	, DialogEditorPtr(InDialogEditorPtr)
{
	TabLabel = LOCTEXT("DialogCameraPresetsLabel", "Camera Presets");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.LockCamera");
	bIsSingleton = true;
	ViewMenuDescription = LOCTEXT("DialogCameraPresetsView", "Camera Presets");
	ViewMenuTooltip = LOCTEXT("DialogCameraPresetsView_ToolTip", "Show the dialog camera presets tab");
}

TSharedRef<SWidget> FDialogCameraPresetsSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	check(DialogEditorPtr.IsValid());
	return DialogEditorPtr.Pin()->SpawnDialogCameraPresetsTab();
}

FText FDialogCameraPresetsSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("DialogCameraPresetsTabToolTip", "Show the dialog camera presets tab");
}


FDialogStageSettingsSummoner::FDialogStageSettingsSummoner(TSharedPtr<class FDialogBuilderEditor> InDialogEditorPtr)
	: FWorkflowTabFactory(FDialogBuilderEditorTabs::DialogStageSettingsID, InDialogEditorPtr)
	, DialogEditorPtr(InDialogEditorPtr)
{
	TabLabel = LOCTEXT("DialogStageSettingsLabel", "Dialog Stage");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("DialogStageSettingsView", "Dialog Stage");
	ViewMenuTooltip = LOCTEXT("DialogStageSettings_ToolTip", "Show the dialog stage settings");
}

TSharedRef<SWidget> FDialogStageSettingsSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	check(DialogEditorPtr.IsValid());
	return DialogEditorPtr.Pin()->SpawnDialogStageSettings();
}

FText FDialogStageSettingsSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("DialogStageSettingsTooltip", "The dialog editor details tab allows editing of the properties of dialog stage");
}


DialogDefinitionsSummoner::DialogDefinitionsSummoner(TSharedPtr<class FDialogBuilderEditor> InDialogEditorPtr)
	: FWorkflowTabFactory(FDialogBuilderEditorTabs::DialogDefinitionsID, InDialogEditorPtr)
	, DialogEditorPtr(InDialogEditorPtr)
{
	TabLabel = LOCTEXT("DialogDefinitionsLabel", "Dialog Definitions");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.FindResults");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("DialogDefinitionsView", "Dialog Definitions");
	ViewMenuTooltip = LOCTEXT("DialogDefinitionsView_ToolTip", "Show the dialog Definitions");
}

TSharedRef<SWidget> DialogDefinitionsSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return DialogEditorPtr.Pin()->SpawnDialogDefinitions();
}

FText DialogDefinitionsSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("DialogDefinitionsTabTooltip", "Show the dialog Definitions");
}


FDialogSequencerTabSummoner::FDialogSequencerTabSummoner(TSharedPtr<class FDialogBuilderEditor> InDialogEditorPtr)
	: FWorkflowTabFactory(FDialogBuilderEditorTabs::DialogSequencerTabID, InDialogEditorPtr)
	, DialogEditorPtr(InDialogEditorPtr)
{
	TabLabel = LOCTEXT("DialogSequencerTabLabel", "Sequencer");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Cinematics");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("DialogSequencerTabView", "Sequencer");
	ViewMenuTooltip = LOCTEXT("DialogSequencerTabView_ToolTip", "Show the dialog sequencer tab");
}

TSharedRef<SWidget> FDialogSequencerTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return DialogEditorPtr.Pin()->SpawnDialogSequencerTab();
}

FText FDialogSequencerTabSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("DialogSequencerTabToolTip", "Show the dialog sequencer tab");
}


FDialogSequencerViewportSummoner::FDialogSequencerViewportSummoner(TSharedPtr<class FDialogBuilderEditor> InDialogEditorPtr)
	: FWorkflowTabFactory(FDialogBuilderEditorTabs::DialogSequencerViewportID, InDialogEditorPtr)
	, DialogEditorPtr(InDialogEditorPtr)
{
	TabLabel = LOCTEXT("DialogSequencerViewportTabLabel", "Dialog Sequencer Viewport");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports");
	bIsSingleton = true;
	ViewMenuDescription = LOCTEXT("DialogSequencerViewportTabView", "Dialog Sequencer Viewport");
	ViewMenuTooltip = LOCTEXT("DialogSequencerViewportTabView_ToolTip", "Show the dialog sequencer viewport tab");
}

TSharedRef<SWidget> FDialogSequencerViewportSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return DialogEditorPtr.Pin()->SpawnDialogSequencerViewportTab();
}

FText FDialogSequencerViewportSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("DialogSequencerViewportTabToolTip", "Show the dialog sequencer viewport tab");
}

#undef LOCTEXT_NAMESPACE

