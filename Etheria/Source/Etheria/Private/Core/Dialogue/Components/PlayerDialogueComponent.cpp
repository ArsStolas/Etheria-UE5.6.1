/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UPlayerDialogueComponent" - Source
 */
#include "Core/Dialogue/Components/PlayerDialogueComponent.h"

UPlayerDialogueComponent::UPlayerDialogueComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerDialogueComponent::ApplyDialoguePolicy(const FDialogueControlPolicy& Policy)
{
    LastPolicy = Policy;
    bInDialogue = true;

    ApplyPolicy(Policy.Controls);
    BP_OnDialoguePolicyApplied(Policy);
}

void UPlayerDialogueComponent::RestoreDialoguePolicy()
{
    if (!bInDialogue)
    {
        return;
    }

    bInDialogue = false;
    RestorePolicy();
    BP_OnDialoguePolicyRestored();
}
