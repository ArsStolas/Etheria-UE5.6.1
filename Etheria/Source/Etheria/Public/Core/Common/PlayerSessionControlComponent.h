/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UPlayerSessionControlComponent" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Common/Data/SessionControlTypes.h"
#include "PlayerSessionControlComponent.generated.h"

class APlayerController;

/**
 * Base helper component to apply/restore a FSessionControlPolicy on the local player.
 * Put this on either the PlayerController or the PlayerCharacter (it will find the controller).
 */
UCLASS(Abstract, BlueprintType, Blueprintable, ClassGroup=(Systems), meta=(BlueprintSpawnableComponent))
class UPlayerSessionControlComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPlayerSessionControlComponent();

    /** Apply a session policy (locks, cursor, input mode, time dilation, etc.). */
    UFUNCTION(BlueprintCallable, Category="Session|Controls")
    virtual void ApplyPolicy(const FSessionControlPolicy& Policy);

    /** Restore pre-session state (best-effort). */
    UFUNCTION(BlueprintCallable, Category="Session|Controls")
    virtual void RestorePolicy();

    /** True if policy currently applied. */
    UFUNCTION(BlueprintPure, Category="Session|Controls")
    bool IsPolicyApplied() const { return bPolicyApplied; }

    /** Query for gameplay (you may gate your input actions with this). */
    UFUNCTION(BlueprintPure, Category="Session|Controls")
    bool IsActionsLocked() const { return bActionsLocked; }

protected:
    virtual void BeginPlay() override;

    /** Best-effort: find a local PlayerController from owner. */
    UFUNCTION(BlueprintCallable, Category="Session|Controls")
    APlayerController* ResolvePlayerController() const;

    /** For derived components: called after ApplyPolicy */
    UFUNCTION(BlueprintImplementableEvent, Category="Session|Events")
    void BP_OnPolicyApplied(const FSessionControlPolicy& Policy);

    /** For derived components: called after RestorePolicy */
    UFUNCTION(BlueprintImplementableEvent, Category="Session|Events")
    void BP_OnPolicyRestored();

private:
    // Cached pre-session values
    bool bPolicyApplied = false;
    bool bActionsLocked = false;

    bool bPrevIgnoreMove = false;
    bool bPrevIgnoreLook = false;
    bool bPrevShowCursor = false;

    float PrevTimeDilation = 1.0f;

    // We can't reliably retrieve the previous InputMode from UE API, so we restore to GameOnly by default.
    void ApplyInputMode(APlayerController* PC, ESessionInputMode Mode, bool bShowCursor);
    void RestoreInputMode(APlayerController* PC);
};
