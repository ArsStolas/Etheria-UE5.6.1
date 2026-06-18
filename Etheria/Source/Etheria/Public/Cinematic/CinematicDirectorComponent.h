/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: UCinematicDirectorComponent
 * Notes: PlayerController component driving Level Sequence cinematics with designer-authored
 *        dialogue (pause-for-input) and an optional hold-to-skip flow (fade -> jump -> fade in).
 *        Sequencer triggers the dialogue through UCinematicDialogueLibrary event endpoints.
 *        Gameplay "barks" (self-talk) are exposed as Blueprint-callable, condition-driven helpers.
 */
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "CinematicDirectorComponent.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class UCinematicDialogueWidget;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;
class APlayerController;

/* ═══════════ Delegates ═══════════ */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCinematicStarted, ULevelSequence*, Sequence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCinematicFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCinematicSkipped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCinematicDialogueShown, FText, Speaker, FText, Text, bool, bWaitingForInput);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCinematicDialogueAdvanced);

UCLASS(ClassGroup = (Etheria), meta = (BlueprintSpawnableComponent,
    ToolTip = "Place on the PlayerController. Plays cinematic Level Sequences, shows designer-authored dialogue, handles pause-for-input, hold-to-skip and screen fades."))
class ETHERIA_API UCinematicDirectorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCinematicDirectorComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    /* ═══════════ Cinematic control ═══════════ */

    // Starts a cinematic from a Level Sequence. Spawns the player, shows the widget, binds Next/Skip input.
    UFUNCTION(BlueprintCallable, Category = "Etheria|Cinematic",
        meta = (ToolTip = "Starts a cinematic from a Level Sequence. SkipTargetMarker is a Marked Frame label the skip jumps to so the final camera change still plays.",
                AdvancedDisplay = "SkipTargetMarker,bStartFadedToBlack"))
    void PlayCinematic(ULevelSequence* Sequence, bool bAllowSkip = true, FName SkipTargetMarker = TEXT("SkipTarget"), bool bStartFadedToBlack = false);

    UFUNCTION(BlueprintCallable, Category = "Etheria|Cinematic",
        meta = (ToolTip = "Stops the current cinematic. Set bFadeIn only when the screen is currently black (e.g. you faded out yourself)."))
    void StopCinematic(bool bFadeIn = false);

    UFUNCTION(BlueprintCallable, Category = "Etheria|Cinematic",
        meta = (ToolTip = "Advances a paused dialogue line. Bound to the Next input, can also be called from Blueprint."))
    void AdvanceDialogue();

    /* ═══════════ Sequencer-driven dialogue (called from the Event Track) ═══════════ */

    UFUNCTION(BlueprintCallable, Category = "Etheria|Cinematic|Dialogue",
        meta = (ToolTip = "Shows a dialogue line during a cinematic. If bWaitForInput the sequence pauses until AdvanceDialogue()."))
    void ShowDialogueLine(FText Speaker, FText Text, bool bWaitForInput = true);

    UFUNCTION(BlueprintCallable, Category = "Etheria|Cinematic|Dialogue",
        meta = (ToolTip = "Clears the dialogue box (e.g. for pure-animation beats)."))
    void HideDialogue();

    /* ═══════════ Gameplay dialogue / barks (non-sequencer, Blueprint-driven) ═══════════ */

    UFUNCTION(BlueprintCallable, Category = "Etheria|Cinematic|Bark",
        meta = (ToolTip = "Shows a self-talk / bark line OUTSIDE any cinematic. Auto-hides after DisplayDuration (0 = stay until HideGameplayDialogue)."))
    void ShowGameplayDialogue(FText Speaker, FText Text, float DisplayDuration = 4.f);

    UFUNCTION(BlueprintCallable, Category = "Etheria|Cinematic|Bark",
        meta = (ToolTip = "Picks a random line from the array and shows it as a bark. Avoids repeating the previous one."))
    void PlayRandomBark(const TArray<FText>& Lines, FText Speaker, float DisplayDuration = 4.f);

    UFUNCTION(BlueprintCallable, Category = "Etheria|Cinematic|Bark")
    void HideGameplayDialogue();

    /* ═══════════ Queries ═══════════ */

    UFUNCTION(BlueprintPure, Category = "Etheria|Cinematic")
    bool IsCinematicPlaying() const { return bCinematicActive; }

    UFUNCTION(BlueprintPure, Category = "Etheria|Cinematic")
    bool IsWaitingForInput() const { return bWaitingForInput; }

    UFUNCTION(BlueprintPure, Category = "Etheria|Cinematic",
        meta = (ToolTip = "First key currently mapped to the Next input, for displaying a key glyph on the widget."))
    FKey GetNextActionKey() const;

    /* ═══════════ Events ═══════════ */

    UPROPERTY(BlueprintAssignable, Category = "Etheria|Cinematic|Events")
    FOnCinematicStarted OnCinematicStarted;

    UPROPERTY(BlueprintAssignable, Category = "Etheria|Cinematic|Events")
    FOnCinematicFinished OnCinematicFinished;

    UPROPERTY(BlueprintAssignable, Category = "Etheria|Cinematic|Events")
    FOnCinematicSkipped OnCinematicSkipped;

    // Broadcast every time a line is shown. Hook VO / SFX here from Blueprint.
    UPROPERTY(BlueprintAssignable, Category = "Etheria|Cinematic|Events")
    FOnCinematicDialogueShown OnDialogueShown;

    UPROPERTY(BlueprintAssignable, Category = "Etheria|Cinematic|Events")
    FOnCinematicDialogueAdvanced OnDialogueAdvanced;

    /* ═══════════ Setup (designer) ═══════════ */

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etheria|Cinematic|Setup",
        meta = (ToolTip = "UMG class used for dialogue. Reparent your widget to UCinematicDialogueWidget."))
    TSubclassOf<UCinematicDialogueWidget> DialogueWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etheria|Cinematic|Setup",
        meta = (ToolTip = "Input Action that advances dialogue. Give it a Pressed trigger (separate key) or a Tap trigger (same key as Skip)."))
    TObjectPtr<UInputAction> NextAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etheria|Cinematic|Setup",
        meta = (ToolTip = "Input Action held to skip the whole cinematic. A simple Pressed/Down trigger is enough; the hold time is driven by SkipHoldDuration."))
    TObjectPtr<UInputAction> SkipAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etheria|Cinematic|Setup",
        meta = (ToolTip = "Mapping Context added while a cinematic is playing (must map Next + Skip)."))
    TObjectPtr<UInputMappingContext> CinematicInputContext;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etheria|Cinematic|Setup",
        meta = (ToolTip = "Priority of the cinematic Mapping Context.", ClampMin = "0"))
    int32 InputContextPriority = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etheria|Cinematic|Setup",
        meta = (ToolTip = "Seconds the Skip input must be held to skip the cinematic.", ClampMin = "0.1"))
    float SkipHoldDuration = 3.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etheria|Cinematic|Setup",
        meta = (ToolTip = "Duration of the skip fade-out / fade-in.", ClampMin = "0.0"))
    float FadeDuration = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Etheria|Cinematic|Setup")
    FLinearColor FadeColor = FLinearColor::Black;

private:
    /* ═══════════ Internal ═══════════ */
    void SetupInput();
    void HandleNextInput();
    void HandleSkipStarted();
    void HandleSkipReleased();

    void BeginSkip();
    void CompleteSkip();
    void JumpToSkipTargetOrEnd();

    UFUNCTION()
    void HandleSequenceFinished();

    void FinishInternal(bool bFadeIn);
    void StartFade(bool bToBlack);
    bool TryGetMarkerTime(FName MarkerLabel, float& OutSeconds) const;
    void EnsureWidget();

    APlayerController* GetOwningController() const;
    UEnhancedInputComponent* GetEnhancedInput() const;

    UPROPERTY(Transient) TObjectPtr<ULevelSequencePlayer> SequencePlayer;
    UPROPERTY(Transient) TObjectPtr<ALevelSequenceActor> SequenceActor;
    UPROPERTY(Transient) TObjectPtr<UCinematicDialogueWidget> DialogueWidget;
    UPROPERTY(Transient) TObjectPtr<ULevelSequence> CurrentSequence;

    TArray<uint32> InputBindingHandles;
    FTimerHandle SkipTimerHandle;
    FTimerHandle BarkTimerHandle;

    bool bInputBound = false;
    bool bCinematicActive = false;
    bool bWaitingForInput = false;
    bool bAllowSkipCurrent = false;
    bool bSkipKeyHeld = false;
    bool bSkipping = false;
    bool bFinishing = false;
    float SkipHoldElapsed = 0.f;
    FName CurrentSkipMarker = NAME_None;
    int32 LastBarkIndex = -1;
};
