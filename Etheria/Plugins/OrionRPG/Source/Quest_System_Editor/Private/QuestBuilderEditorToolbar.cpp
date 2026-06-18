// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEditorToolbar.h"
#include "QuestBuilder_EditorCommands.h"
#include "QuestBuilder_EditorStyle.h"
#include "QuestBuilderEditor.h"

#define LOCTEXT_NAMESPACE "QuestBuilderEditorToolbar"
void FQuestBuilderEditorToolbar::AddQuestSystemToolbar(TSharedPtr<FExtender> Extender)
{
	check(QuestSystemEditor.IsValid());
	TSharedPtr<FQuestBuilderEditor> QuestSystemEditorPtr = QuestSystemEditor.Pin();
	
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, QuestSystemEditorPtr->GetToolkitCommands(), FToolBarExtensionDelegate::CreateSP(this, &FQuestBuilderEditorToolbar::FillQuestSystemToolbar));
	QuestSystemEditorPtr->AddToolbarExtender(ToolbarExtender);
}

void FQuestBuilderEditorToolbar::FillQuestSystemToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(QuestSystemEditor.IsValid());
	TSharedPtr<FQuestBuilderEditor> QuestSystemEditorPtr = QuestSystemEditor.Pin();

	ToolbarBuilder.BeginSection("Quest System");
	{
		const FText NewObjectiveLabel = LOCTEXT("NewTask_Label", "New Objective");
		const FText NewObjectiveTooltip = LOCTEXT("NewTask_ToolTip", "Create a new objective node Blueprint from a base class");
		const FSlateIcon NewObjectiveIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "BTEditor.Graph.NewTask");

		
		ToolbarBuilder.AddToolBarButton(FQuestBuilder_EditorCommands::Get().NewObjective,
			NAME_None,
			NewObjectiveLabel,
			NewObjectiveTooltip,
			NewObjectiveIcon
		);
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Quest System");
	{
		const FText NewDecoratorLabel = LOCTEXT("NewOrionDecorator_Label", "New Orion Condition");
		const FText NewDecoratorTooltip = LOCTEXT("NewOrionDecorator_ToolTip", "Create new Condition Blueprint from a base class");
		const FSlateIcon NewDecoratorIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "BTEditor.Graph.BTNode.Decorator.Conditional.Icon");


		ToolbarBuilder.AddToolBarButton(FQuestBuilder_EditorCommands::Get().NewQuestDecorator,
			NAME_None,
			NewDecoratorLabel,
			NewDecoratorTooltip,
			NewDecoratorIcon
		);
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Quest System");
	{
		const FText NewEventLabel = LOCTEXT("NewQuestEvent_Label", "New Orion Event");
		const FText NewEventTooltip = LOCTEXT("NewQuestEvent_ToolTip", "Create a new Quest Event Blueprint from a base class");
		const FSlateIcon NewEventIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.CustomEvent_16x");


		ToolbarBuilder.AddToolBarButton(FQuestBuilder_EditorCommands::Get().NewQuestEvent,
			NAME_None,
			NewEventLabel,
			NewEventTooltip,
			NewEventIcon
		);
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Quest System");
	{
		const FText QuestSettingLabel = LOCTEXT("QuestSetting_Label", "Quest Default Info");
		const FText QuestSettingTooltip = LOCTEXT("QuestSetting_ToolTip", "Access Quest Default Info");
		const FSlateIcon QuestSettingIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ProjectSettings.TabIcon");


		ToolbarBuilder.AddToolBarButton(FQuestBuilder_EditorCommands::Get().QuestSetting,
			NAME_None,
			QuestSettingLabel,
			QuestSettingTooltip,
			QuestSettingIcon
		);
	}
	ToolbarBuilder.EndSection();

	
}



#undef LOCTEXT_NAMESPACE
