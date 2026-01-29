/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "SessionControlTypes" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "SessionControlTypes.generated.h"

/**
 * Common policies used by dialogue/cinematic systems to control player input & presentation.
 * Keep this lightweight and Blueprint-friendly.
 */

UENUM(BlueprintType)
enum class ESessionInputMode : uint8
{
    GameOnly        UMETA(DisplayName = "Game Only"),
    UIOnly          UMETA(DisplayName = "UI Only"),
    GameAndUI       UMETA(DisplayName = "Game And UI")
};

USTRUCT(BlueprintType)
struct FSessionControlPolicy
{
    GENERATED_BODY()

    /** Locks character movement inputs (WASD / stick). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Session|Controls")
    bool bLockMovement = true;

    /** Locks non-movement gameplay actions (attack, interact, etc). Your gameplay should query IsActionsLocked() if needed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Session|Controls")
    bool bLockActions = true;

    /** Locks camera look/rotation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Session|Controls")
    bool bLockCamera = false;

    /** Should we show a mouse cursor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Session|Controls")
    bool bShowCursor = false;

    /** Desired input mode. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Session|Controls")
    ESessionInputMode InputMode = ESessionInputMode::GameOnly;

    /** Hide HUD (CinematicMode). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Session|Presentation")
    bool bHideHUD = false;

    /** Optional global time dilation for the session (1.0 = normal). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Session|Presentation", meta=(ClampMin="0.01", ClampMax="2.0"))
    float TimeDilation = 1.0f;
};
