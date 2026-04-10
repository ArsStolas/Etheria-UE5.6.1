// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "DialogBuilder_EditorStyle.h"

class FDialogBuilder_EditorCommands : public TCommands<FDialogBuilder_EditorCommands>
{
public:

	FDialogBuilder_EditorCommands()
		: TCommands<FDialogBuilder_EditorCommands>(TEXT("Dialog_System_Editor"), NSLOCTEXT("Contexts", "Dialog_System_Editor", "Dialog_System_Editor Plugin"), NAME_None, FDialogBuilder_EditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;

	// New documents
	TSharedPtr< FUICommandInfo > AddNewDialogGraph;
	
	//Toolbar
	TSharedPtr<FUICommandInfo> NewDialogDecorator;
	TSharedPtr<FUICommandInfo> NewDialogEvent;
	TSharedPtr<FUICommandInfo> NewDialogCameraShot;
	TSharedPtr<FUICommandInfo> DialogSetting;
};