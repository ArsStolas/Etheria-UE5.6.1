/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "DialogueTypes" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Core/Common/Data/SessionControlTypes.h"
#include "GameplayTagContainer.h"
#include "DialogueSystemTypes.generated.h"

class USoundBase;
class UAnimMontage;
class UUserWidget;
class AActor;
class ACinematicCameraRigBase;

UENUM(BlueprintType)
enum class EDialogueEndReason : uint8
{
    Completed   UMETA(DisplayName="Completed"),
    Interrupted UMETA(DisplayName="Interrupted"),
    Skipped     UMETA(DisplayName="Skipped"),
    Cancelled   UMETA(DisplayName="Cancelled")
};

UENUM(BlueprintType)
enum class EDialogueCameraMode : uint8
{
    None            UMETA(DisplayName="None"),
    FocusSpeaker    UMETA(DisplayName="Focus Speaker"),
    FocusConversation UMETA(DisplayName="Focus Conversation")
};

USTRUCT(BlueprintType)
struct FDialogueLine
{
    GENERATED_BODY()

    /** Who is speaking. If null, your UI can fallback to an "unknown" speaker. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    TSoftObjectPtr<AActor> Speaker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FText Text;

    /** If <= 0, subsystem uses DefaultLineDuration. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ClampMin="0.0"))
    float Duration = 0.0f;

    /** Optional voice line. Played by BP or by your audio system. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    TObjectPtr<USoundBase> Voice = nullptr;

    /** Optional montage. Played by BP or by your animation system. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    TObjectPtr<UAnimMontage> Montage = nullptr;

    /** Free-form tags for gameplay/UI (e.g. "Shout", "Whisper", "Quest"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FGameplayTagContainer Tags;
};

USTRUCT(BlueprintType)
struct FDialogueChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    int32 ChoiceId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FGameplayTagContainer Tags;
};

USTRUCT(BlueprintType)
struct FDialogueControlPolicy
{
    GENERATED_BODY()

    /** Base control policy (locks, cursor, input mode, hud, time dilation...). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Policy")
    FSessionControlPolicy Controls;

    /** How the camera should behave during the dialogue session. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Policy")
    EDialogueCameraMode CameraMode = EDialogueCameraMode::None;

    /** If CameraMode != None, which rig class to spawn (optional). If null, subsystem falls back to no focus camera. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Policy")
    TSubclassOf<ACinematicCameraRigBase> FocusRigClass;

    /** Blend time when switching to/from a focus camera. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Policy", meta=(ClampMin="0.0"))
    float CameraBlendTime = 0.25f;

    /** Show speech bubbles while lines play. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Policy")
    bool bShowSpeechBubbles = true;
};

USTRUCT(BlueprintType)
struct FDialogueSessionRequest
{
    GENERATED_BODY()

    /** Optional initiator (usually the player pawn). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    TSoftObjectPtr<AActor> Initiator;

    /** Optional target (NPC). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    TSoftObjectPtr<AActor> Target;

    /** Scripted lines; you can also ignore this and drive lines manually via BP using Start/Advance APIs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    TArray<FDialogueLine> Lines;

    /** If true, subsystem auto-plays Lines sequentially using timers. If false, you drive NextLine manually. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    bool bAutoPlayLines = true;

    /** Default duration when a line duration is not specified. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(ClampMin="0.1"))
    float DefaultLineDuration = 2.5f;

    /** Policy applied on start; restored on end. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    FDialogueControlPolicy Policy;

    /** Optional initial choices (for UI). If non-empty, subsystem broadcasts ChoicesRequested when session starts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue")
    TArray<FDialogueChoice> InitialChoices;
};

USTRUCT(BlueprintType)
struct FDialogueBubbleRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Bubble")
    FText Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Bubble", meta=(ClampMin="0.1"))
    float Duration = 2.5f;

    /** Optional widget class to use. If null, subsystem uses its DefaultBubbleWidgetClass. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Bubble")
    TSubclassOf<UUserWidget> BubbleWidgetClass;

    /** Relative offset above the actor (in cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Bubble")
    FVector RelativeOffset = FVector(0.f, 0.f, 120.f);

    /** Higher priority bubbles can replace lower priority ones. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Bubble", meta=(ClampMin="0"))
    int32 Priority = 0;

    /** If true, bubble stays until manually hidden. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Bubble")
    bool bPersistent = false;
};

// Delegates (BlueprintAssignable)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueSessionStarted, const FDialogueSessionRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueSessionEnded, EDialogueEndReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueLineStarted, const FDialogueLine&, Line, int32, LineIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueChoicesRequested, const TArray<FDialogueChoice>&, Choices, bool, bIsInitial);
