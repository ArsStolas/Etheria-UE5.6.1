// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "QuestBuilder_EditorStyle.h"

class FQuestBuilder_EditorCommands : public TCommands<FQuestBuilder_EditorCommands>
{
public:

	FQuestBuilder_EditorCommands()
		: TCommands<FQuestBuilder_EditorCommands>(TEXT("Quest_System_Editor"), NSLOCTEXT("Contexts", "Quest_System_Editor", "Quest_System_Editor Plugin"), NAME_None, FQuestBuilder_EditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;

	// New documents
	TSharedPtr< FUICommandInfo > AddNewQuestGraph;
	
	//Toolbar
	TSharedPtr<FUICommandInfo> NewObjective;
	TSharedPtr<FUICommandInfo> NewQuestDecorator;
	TSharedPtr<FUICommandInfo> NewQuestEvent;
	TSharedPtr<FUICommandInfo> QuestSetting;
};