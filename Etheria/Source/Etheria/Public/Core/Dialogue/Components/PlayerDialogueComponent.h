/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UPlayerDialogueComponent" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Core/Common/PlayerSessionControlComponent.h"
#include "Core/Dialogue/Data/DialogueSystemTypes.h"
#include "PlayerDialogueComponent.generated.h"

/**
 * Player-side component that applies dialogue control policies (input, cursor, HUD, etc.).
 * Put this on PlayerController (recommended) or PlayerCharacter.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Systems), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UPlayerDialogueComponent : public UPlayerSessionControlComponent
{
    GENERATED_BODY()

public:
    UPlayerDialogueComponent();

    UFUNCTION(BlueprintCallable, Category="Dialogue|Policy")
    void ApplyDialoguePolicy(const FDialogueControlPolicy& Policy);

    UFUNCTION(BlueprintCallable, Category="Dialogue|Policy")
    void RestoreDialoguePolicy();

    UFUNCTION(BlueprintPure, Category="Dialogue|Policy")
    bool IsInDialogue() const { return bInDialogue; }

    /** Hook for your gameplay input gating (e.g. abilities) */
    UFUNCTION(BlueprintPure, Category="Dialogue|Policy")
    bool AreActionsLockedByDialogue() const { return IsInDialogue() && IsActionsLocked(); }

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Dialogue|Events")
    void BP_OnDialoguePolicyApplied(const FDialogueControlPolicy& Policy);

    UFUNCTION(BlueprintImplementableEvent, Category="Dialogue|Events")
    void BP_OnDialoguePolicyRestored();

private:
    bool bInDialogue = false;
    FDialogueControlPolicy LastPolicy;
};
