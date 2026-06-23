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

	//Viewport
	TSharedPtr< FUICommandInfo > ShowGrid;

	// New documents
	TSharedPtr< FUICommandInfo > AddNewDialogGraph;
	
	//Toolbar
	TSharedPtr<FUICommandInfo> NewDialogDecorator;
	TSharedPtr<FUICommandInfo> NewDialogEvent;
	TSharedPtr<FUICommandInfo> NewDialogCameraShot;
	TSharedPtr<FUICommandInfo> DialogSetting;

	//Sequencer
	/** Create camera and set it as the current camera cut */
	TSharedPtr< FUICommandInfo > CreateCamera;

	/** Actor pilot commands */
	TSharedPtr< FUICommandInfo > SelectPilotedActor;
	TSharedPtr< FUICommandInfo > EjectActorPilot;
	TSharedPtr< FUICommandInfo > PilotSelectedActor;

	/** Toggles showing the exact camera view when locking a viewport to a camera */
	TSharedPtr< FUICommandInfo > ToggleActorPilotCameraView;

	/** Toggles game preview in the viewport */
	TSharedPtr< FUICommandInfo > ToggleGameView;

	// Viewport camera modes
	TSharedPtr<FUICommandInfo> SetViewportCameraPerspective;
	TSharedPtr<FUICommandInfo> SetViewportCameraDialogCamera;
	TSharedPtr<FUICommandInfo> SetViewportCameraSequencerCuts;
};