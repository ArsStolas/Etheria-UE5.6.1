/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UPlayerCinematicComponent" - Source
 */
#include "Core/Cinematics/Components/PlayerCinematicComponent.h"

UPlayerCinematicComponent::UPlayerCinematicComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerCinematicComponent::ApplyCinematicPolicy(const FCinematicControlPolicy& Policy)
{
    LastPolicy = Policy;
    bInCinematic = true;

    ApplyPolicy(Policy.Controls);
    BP_OnCinematicPolicyApplied(Policy);
}

void UPlayerCinematicComponent::RestoreCinematicPolicy()
{
    if (!bInCinematic)
    {
        return;
    }

    bInCinematic = false;
    RestorePolicy();
    BP_OnCinematicPolicyRestored();
}
