// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogSequenceShot.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderNode_DialogLine.h"
#include <CineCameraActor.h>
#include <CineCameraComponent.h>
#include "GameFramework/PlayerController.h"

UDialogSequenceShot::UDialogSequenceShot()
{
	TrackedBone = FName("head");
	bFocusCameraOnActor = true;
}
