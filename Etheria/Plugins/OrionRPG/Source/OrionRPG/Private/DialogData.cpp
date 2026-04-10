// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogData.h"
#include "OrionRPG.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_Root.h"
#include "AssetRegistry/AssetRegistryModule.h"

//Gameplay Tags
ORIONRPG_API UE_DEFINE_GAMEPLAY_TAG(TAG_Dialog, "Dialog");
ORIONRPG_API UE_DEFINE_GAMEPLAY_TAG(TAG_Dialog_Participant, "Dialog.Participant");
ORIONRPG_API UE_DEFINE_GAMEPLAY_TAG(TAG_Dialog_Participant_Player, "Dialog.Participant.Player");