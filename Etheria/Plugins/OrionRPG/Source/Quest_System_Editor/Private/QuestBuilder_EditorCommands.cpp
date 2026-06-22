// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "QuestBuilder_EditorCommands.h"

#define LOCTEXT_NAMESPACE "FQuest_System_EditorModule"

void FQuestBuilder_EditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "Quest_System_Editor", "Bring up Quest_System_Editor window", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(NewObjective, "New Objective", "New Objective", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(NewQuestDecorator, "New Orion Condition", "New Orion Condition", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(NewQuestEvent, "New Orion Event", "New Orion Event", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AddNewQuestGraph, "New Quest Graph", "New Quest Graph", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Q));
	UI_COMMAND(QuestSetting, "Quest Setting", "Open Quest Setting", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
