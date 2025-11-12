#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * Global gameplay tag declarations for Etheria.
 */
namespace EtheriaTags
{
	// === Movement ===
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Grounded);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Grounded_Idle);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Grounded_Walking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Grounded_Sprinting);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Airborne);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Airborne_Falling);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Airborne_Gliding);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Airborne_Diving);

	// === Combat ===
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_Attacking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_Blocking);

	// === Life ===
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Life);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Life_TakingDamage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Life_Healing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Life_Dead);
}
