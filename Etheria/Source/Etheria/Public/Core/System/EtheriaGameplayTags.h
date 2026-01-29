/**
 * Etheria's End Project, 2025
 * Created by: "Zhailendra"
 * Last Updated by: "0nnen"
 * Class: "EtheriaGameplayTags" - Header
 * Notes: Centralized gameplay tag declarations.
 */

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace EtheriaTags
{
	// === Life ===
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Life);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Life_TakingDamage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Life_Healing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Life_Dead);
	
	// === Movement - Grounded ===
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Grounded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Grounded_Idle);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Grounded_Walking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Grounded_Sprinting);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Grounded_Crouching);

	// === Movement - Airborne ===
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Airborne);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Airborne_Falling);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Airborne_Gliding);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Airborne_Diving);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Airborne_Jumping);
	
	// === Swim ===
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Swim);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Swim_Surface);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Swim_Underwater);
	
	// === Rope ===
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Rope);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Rope_Attached);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Rope_Swinging);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Rope_Detached);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Rope_Climbing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Rope_Pulling);

	// === Combat ===
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_Attacking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_Attacking_Charging);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_Blocking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_Dodging);
	
}
