/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: UCinematicDialogueLibrary
 * Notes: Static helpers called from the Level Sequence Director Blueprint (Event Track endpoints).
 *        They forward to the local player's UCinematicDirectorComponent, so a dialogue event in
 *        Sequencer is a single function-call node with editable payload (Speaker / Text / Wait).
 */
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CinematicDialogueLibrary.generated.h"

class UCinematicDirectorComponent;

UCLASS()
class ETHERIA_API UCinematicDialogueLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Etheria|Cinematic",
        meta = (WorldContext = "WorldContextObject",
                ToolTip = "Returns the local player's Cinematic Director component."))
    static UCinematicDirectorComponent* GetCinematicDirector(const UObject* WorldContextObject);

    // Sequencer Event Track endpoint: show a dialogue line. If bWaitForInput, the sequence pauses
    // until the player presses Next. Leave Speaker empty to hide the name field.
    UFUNCTION(BlueprintCallable, Category = "Etheria|Cinematic|Dialogue",
        meta = (WorldContext = "WorldContextObject",
                ToolTip = "Sequencer event endpoint: show a dialogue line (Speaker / Text / Wait-for-input)."))
    static void ShowCinematicDialogue(const UObject* WorldContextObject, FText Speaker, FText Text, bool bWaitForInput = true);

    UFUNCTION(BlueprintCallable, Category = "Etheria|Cinematic|Dialogue",
        meta = (WorldContext = "WorldContextObject",
                ToolTip = "Sequencer event endpoint: clear the dialogue box (for pure-animation beats)."))
    static void HideCinematicDialogue(const UObject* WorldContextObject);
};
