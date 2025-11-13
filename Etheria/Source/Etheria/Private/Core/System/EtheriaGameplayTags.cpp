#include "Core/System/EtheriaGameplayTags.h"

// === Movement ===
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement, "State.Movement");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement_Grounded, "State.Movement.Grounded");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement_Grounded_Idle, "State.Movement.Grounded.Idle");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement_Grounded_Walking, "State.Movement.Grounded.Walking");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement_Grounded_Sprinting, "State.Movement.Grounded.Sprinting");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement_Grounded_Crouching, "State.Movement.Grounded.Crouching");

UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement_Airborne, "State.Movement.Airborne");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement_Airborne_Falling, "State.Movement.Airborne.Falling");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement_Airborne_Gliding, "State.Movement.Airborne.Gliding");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement_Airborne_Diving, "State.Movement.Airborne.Diving");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Movement_Airborne_Jumping, "State.Movement.Airborne.Jumping");

// === Combat ===
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Combat, "State.Combat");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Combat_Attacking, "State.Combat.Attacking");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Combat_Attacking_Charging, "State.Combat.Attacking.Charging");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Combat_Blocking, "State.Combat.Blocking");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Combat_Dodging, "State.Combat.Dodging");

// === Life ===
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Life, "State.Life");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Life_TakingDamage, "State.Life.TakingDamage");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Life_Healing, "State.Life.Healing");
UE_DEFINE_GAMEPLAY_TAG(EtheriaTags::State_Life_Dead, "State.Life.Dead");
