// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilder_EditorCommands.h"

#define LOCTEXT_NAMESPACE "FDialog_System_EditorModule"

void FDialogBuilder_EditorCommands::RegisterCommands()
{
	//Viewport
	UI_COMMAND(ShowGrid, "Show Grid", "Toggles viewport grid", EUserInterfaceActionType::ToggleButton, FInputChord());

	//DialogGraph
	UI_COMMAND(OpenPluginWindow, "Dialog_System_Editor", "Bring up Dialog_System_Editor window", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(NewDialogDecorator, "New Orion Condition", "New Orion Condition", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(NewDialogEvent, "New Orion Event", "New Orion Event", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(NewDialogCameraShot, "New Dialog Camera Shot", "New Dialog Camera Shot", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AddNewDialogGraph, "New Dialog Graph", "New Dialog Graph", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Q));
	UI_COMMAND(DialogSetting, "Dialog Setting", "Open Dialog Setting", EUserInterfaceActionType::Button, FInputChord());

	//Sequencer
	UI_COMMAND(CreateCamera, "Create Camera", "Create a new camera and set it as the current camera cut", EUserInterfaceActionType::Button, FInputChord());

	//Pilot Camera
	UI_COMMAND(SelectPilotedActor, "Select Piloted Actor", "Select the currently piloted actor.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(PilotSelectedActor, "Pilot Selected Actor", "Move the selected actor around using the viewport controls, and bind the viewport to the actor's location and orientation.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::P));
	UI_COMMAND(EjectActorPilot, "Eject from Actor Pilot", "Stop piloting an actor with the current viewport. Unlocks the viewport's position and orientation from the actor the viewport is currently piloting.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ToggleActorPilotCameraView, "Actor Pilot Camera View", "Toggles showing the exact camera view when using the viewport to pilot a camera", EUserInterfaceActionType::ToggleButton, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::C));

	//Toggle Game View
	UI_COMMAND(ToggleGameView, "Game View", "Toggles game view.  Game view shows the scene as it appears in game", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::G));

	// Viewport camera modes
	UI_COMMAND(SetViewportCameraPerspective, "Perspective", "Switch viewport camera mode to perspective", EUserInterfaceActionType::Button, FInputChord(EKeys::One));
	UI_COMMAND(SetViewportCameraDialogCamera, "Dialog Camera", "Lock viewport camera to the dialog camera", EUserInterfaceActionType::Button, FInputChord(EKeys::Two));
	UI_COMMAND(SetViewportCameraSequencerCuts, "Sequencer Cuts", "Enable Sequencer camera cuts", EUserInterfaceActionType::Button, FInputChord(EKeys::Three));
}

#undef LOCTEXT_NAMESPACE
