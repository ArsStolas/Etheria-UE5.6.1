/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UPlayerCinematicComponent" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Core/Common/PlayerSessionControlComponent.h"
#include "Core/Cinematics/Data/CinematicTypes.h"
#include "PlayerCinematicComponent.generated.h"

/**
 * Player-side component that applies cinematic control policies (input, cursor, HUD, etc.).
 * Put this on PlayerController (recommended) or PlayerCharacter.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Systems), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UPlayerCinematicComponent : public UPlayerSessionControlComponent
{
    GENERATED_BODY()

public:
    UPlayerCinematicComponent();

    UFUNCTION(BlueprintCallable, Category="Cinematic|Policy")
    void ApplyCinematicPolicy(const FCinematicControlPolicy& Policy);

    UFUNCTION(BlueprintCallable, Category="Cinematic|Policy")
    void RestoreCinematicPolicy();

    UFUNCTION(BlueprintPure, Category="Cinematic|Policy")
    bool IsInCinematic() const { return bInCinematic; }

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Cinematic|Events")
    void BP_OnCinematicPolicyApplied(const FCinematicControlPolicy& Policy);

    UFUNCTION(BlueprintImplementableEvent, Category="Cinematic|Events")
    void BP_OnCinematicPolicyRestored();

private:
    bool bInCinematic = false;
    FCinematicControlPolicy LastPolicy;
};
