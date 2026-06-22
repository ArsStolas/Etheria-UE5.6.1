// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEditorToolbar.h"
#include "DialogBuilder_EditorCommands.h"
#include "DialogBuilder_EditorStyle.h"
#include "DialogBuilderEditor.h"
#include "Delegates/Delegate.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/SlateDelegates.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Layout/Margin.h"
#include "Math/Vector2D.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Attribute.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"
#include "UObject/NameTypes.h"
#include "UObject/UnrealNames.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "WorkflowOrientedApp/SModeWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "DialogBuilderEditorToolbar"


void FDialogBuilderEditorToolbar::AddModesToolbar(TSharedPtr<FExtender> Extender)
{
	check(DialogBuilderEditor.IsValid());
	TSharedPtr<FDialogBuilderEditor> DialogBuilderEditorPtr = DialogBuilderEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		DialogBuilderEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FDialogBuilderEditorToolbar::FillModesToolbar));
}

void FDialogBuilderEditorToolbar::FillModesToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(DialogBuilderEditor.IsValid());
	TSharedPtr<FDialogBuilderEditor> DialogBuilderEditorPtr = DialogBuilderEditor.Pin();

	TAttribute<FName> GetActiveMode(DialogBuilderEditorPtr.ToSharedRef(), &FDialogBuilderEditor::GetCurrentMode);
	FOnModeChangeRequested SetActiveMode = FOnModeChangeRequested::CreateSP(DialogBuilderEditorPtr.ToSharedRef(), &FDialogBuilderEditor::SetCurrentMode);

	// Left side padding
	DialogBuilderEditorPtr->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(4.0f, 1.0f)));

	DialogBuilderEditorPtr->AddToolbarWidget(
		SNew(SModeWidget, FDialogBuilderEditor::GetLocalizedMode(FDialogBuilderEditor::DialogEditorMode), FDialogBuilderEditor::DialogEditorMode)
		.OnGetActiveMode(GetActiveMode)
		.OnSetActiveMode(SetActiveMode)
		.CanBeSelected(DialogBuilderEditorPtr.Get(), &FDialogBuilderEditor::CanAccessDialogEditorMode)
		.ToolTipText(LOCTEXT("DialogEditorModeButtonTooltip", "Switch to Dialog Editor Mode"))
		.IconImage(FAppStyle::GetBrush("BTEditor.SwitchToBehaviorTreeMode"))
	);

	DialogBuilderEditorPtr->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(10.0f, 1.0f)));

	DialogBuilderEditorPtr->AddToolbarWidget(
		SNew(SModeWidget, FDialogBuilderEditor::GetLocalizedMode(FDialogBuilderEditor::DialogSequencerMode), FDialogBuilderEditor::DialogSequencerMode)
		.OnGetActiveMode(GetActiveMode)
		.OnSetActiveMode(SetActiveMode)
		.CanBeSelected(DialogBuilderEditorPtr.Get(), &FDialogBuilderEditor::CanAccessDialogSequencerMode)
		.ToolTipText(LOCTEXT("DialogSequencerModeButtonTooltip", "Switch to Dialog Sequencer Mode"))
		.IconImage(FAppStyle::GetBrush("BTEditor.SwitchToBlackboardMode"))
	);

	// Right side padding
	DialogBuilderEditorPtr->AddToolbarWidget(SNew(SSpacer).Size(FVector2D(10.0f, 1.0f)));

}

void FDialogBuilderEditorToolbar::AddDialogSystemToolbar(TSharedPtr<FExtender> Extender)
{
	check(DialogBuilderEditor.IsValid());
	TSharedPtr<FDialogBuilderEditor> DialogBuilderEditorPtr = DialogBuilderEditor.Pin();
	
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, DialogBuilderEditorPtr->GetToolkitCommands(), FToolBarExtensionDelegate::CreateSP(this, &FDialogBuilderEditorToolbar::FillDialogSystemToolbar));
	DialogBuilderEditorPtr->AddToolbarExtender(ToolbarExtender);
}

void FDialogBuilderEditorToolbar::FillDialogSystemToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(DialogBuilderEditor.IsValid());
	TSharedPtr<FDialogBuilderEditor> DialogBuilderEditorPtr = DialogBuilderEditor.Pin();

	ToolbarBuilder.BeginSection("Dialog System");
	{
		const TWeakPtr<FDialogBuilderEditor> DialogBuilderEditorWeak = DialogBuilderEditor;

		ToolbarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateLambda([DialogBuilderEditorWeak]()
				{
					TSharedPtr<FDialogBuilderEditor> DialogBuilderEditorPtr = DialogBuilderEditorWeak.Pin();
					if (!DialogBuilderEditorPtr.IsValid())
					{
						return SNullWidget::NullWidget;
					}

					FMenuBuilder MenuBuilder(true, DialogBuilderEditorPtr->GetToolkitCommands());

					MenuBuilder.AddMenuEntry(FDialogBuilder_EditorCommands::Get().NewDialogDecorator);
					MenuBuilder.AddMenuEntry(FDialogBuilder_EditorCommands::Get().NewDialogEvent);
					MenuBuilder.AddMenuEntry(FDialogBuilder_EditorCommands::Get().NewDialogCameraShot);

					return MenuBuilder.MakeWidget();
				}),
			LOCTEXT("AddNewClass_Label", "Add New Class"),
			LOCTEXT("AddNewClass_ToolTip", "Create a new dialog class."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
			false,
			FName("AddNewClass"),
			EVisibility::All,
			LOCTEXT("AddNewClass_Label", "Add New Class")

		);
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Dialog System");
	{
		const FText DialogSettingLabel = LOCTEXT("DialogSetting_Label", "Dialog Default Info");
		const FText DialogSettingTooltip = LOCTEXT("DialogSetting_ToolTip", "Access Dialog Default Info");
		const FSlateIcon DialogSettingIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon");

		ToolbarBuilder.AddToolBarButton(
			FDialogBuilder_EditorCommands::Get().DialogSetting,
			NAME_None,
			DialogSettingLabel,
			DialogSettingTooltip,
			DialogSettingIcon
		);
	}
	ToolbarBuilder.EndSection();

	
}



#undef LOCTEXT_NAMESPACE
