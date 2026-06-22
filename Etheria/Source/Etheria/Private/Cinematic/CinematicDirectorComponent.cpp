/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: UCinematicDialogueWidget - Source
 * Notes: See header. Pause-for-input uses ULevelSequencePlayer::Pause()/Play() driven from
 *        Sequencer events. Skip = fade out -> jump to the "SkipTarget" Marked Frame (so the
 *        final camera change is evaluated) -> stop -> fade in.
 */

#include "Cinematic/CinematicDirectorComponent.h"

#include "Cinematic/CinematicDialogueWidget.h"

#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MovieScene.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "MovieSceneSequencePlayer.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "TimerManager.h"

UCinematicDirectorComponent::UCinematicDirectorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;          // Only used while filling the skip bar.
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCinematicDirectorComponent::BeginPlay()
{
    Super::BeginPlay();
    SetupInput();
}

void UCinematicDirectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bCinematicActive)
    {
        FinishInternal(/*bFadeIn*/ false);
    }

    if (UEnhancedInputComponent* EIC = GetEnhancedInput())
    {
        for (const uint32 Handle : InputBindingHandles)
        {
            EIC->RemoveBindingByHandle(Handle);
        }
    }
    InputBindingHandles.Reset();

    if (DialogueWidget)
    {
        DialogueWidget->RemoveFromParent();
        DialogueWidget = nullptr;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SkipTimerHandle);
        World->GetTimerManager().ClearTimer(BarkTimerHandle);
    }

    Super::EndPlay(EndPlayReason);
}

void UCinematicDirectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bSkipKeyHeld || bSkipping || !bAllowSkipCurrent || !bCinematicActive)
    {
        return;
    }

    SkipHoldElapsed += DeltaTime;
    const float Progress = FMath::Clamp(SkipHoldElapsed / FMath::Max(SkipHoldDuration, 0.01f), 0.f, 1.f);

    if (DialogueWidget)
    {
        DialogueWidget->SetSkipProgress(Progress);
    }

    if (Progress >= 1.f)
    {
        BeginSkip();
    }
}

/* ═══════════ Cinematic control ═══════════ */

void UCinematicDirectorComponent::PlayCinematic(ULevelSequence* Sequence, bool bAllowSkip, FName SkipTargetMarker, bool bStartFadedToBlack)
{
    if (!Sequence)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CinematicDirector] PlayCinematic called with a null Sequence."));
        return;
    }

    APlayerController* PC = GetOwningController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CinematicDirector] Owner does not resolve to a PlayerController."));
        return;
    }

    if (bCinematicActive)
    {
        FinishInternal(/*bFadeIn*/ false);
    }

    CurrentSequence   = Sequence;
    bAllowSkipCurrent = bAllowSkip;
    CurrentSkipMarker = SkipTargetMarker;
    bSkipping         = false;
    bWaitingForInput  = false;
    bSkipKeyHeld      = false;
    SkipHoldElapsed   = 0.f;

    FMovieSceneSequencePlaybackSettings Settings;
    Settings.bAutoPlay = false;

    ALevelSequenceActor* OutActor = nullptr;
    SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(PC->GetWorld(), Sequence, Settings, OutActor);
    SequenceActor  = OutActor;

    if (!SequencePlayer)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CinematicDirector] Failed to create the LevelSequencePlayer."));
        return;
    }

    SequencePlayer->OnFinished.AddDynamic(this, &UCinematicDirectorComponent::HandleSequenceFinished);

    EnsureWidget();
    if (DialogueWidget)
    {
        DialogueWidget->SetDialogueVisible(false);
        DialogueWidget->SetSkipBarVisible(false);
        DialogueWidget->HideNextPrompt();
        DialogueWidget->SetNextKey(GetNextActionKey());
    }

    SetupInput();
    if (CinematicInputContext)
    {
        if (const ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Subsys->AddMappingContext(CinematicInputContext, InputContextPriority);
            }
        }
    }

    // Disable gameplay movement / turning / HUD; our Next & Skip actions still fire.
    PC->SetCinematicMode(true, /*bHidePlayer*/ false, /*bAffectsHUD*/ true, /*bAffectsMovement*/ true, /*bAffectsTurning*/ true);

    bCinematicActive = true;
    bFinishing       = false;

    if (bStartFadedToBlack)
    {
        StartFade(/*bToBlack*/ true);
    }

    OnCinematicStarted.Broadcast(Sequence);
    SequencePlayer->Play();
}

void UCinematicDirectorComponent::StopCinematic(bool bFadeIn)
{
    if (!bCinematicActive)
    {
        return;
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SkipTimerHandle);
    }
    bSkipping = false;
    FinishInternal(bFadeIn);
}

void UCinematicDirectorComponent::AdvanceDialogue()
{
    if (!bCinematicActive || !bWaitingForInput || bSkipping)
    {
        return;
    }

    bWaitingForInput = false;
    if (DialogueWidget)
    {
        DialogueWidget->HideNextPrompt();
    }

    if (SequencePlayer)
    {
        SequencePlayer->Play(); // Resume from the paused frame; the next animation block plays.
    }

    OnDialogueAdvanced.Broadcast();
}

/* ═══════════ Sequencer-driven dialogue ═══════════ */

void UCinematicDirectorComponent::ShowDialogueLine(FText Speaker, FText Text, bool bWaitForInput)
{
    EnsureWidget();
    if (DialogueWidget)
    {
        DialogueWidget->SetDialogueVisible(true);
        DialogueWidget->SetSpeaker(Speaker, !Speaker.IsEmpty());
        DialogueWidget->SetDialogueText(Text);
    }

    if (bWaitForInput && SequencePlayer)
    {
        SequencePlayer->Pause();
        bWaitingForInput = true;
        if (DialogueWidget)
        {
            DialogueWidget->ShowNextPrompt();
        }
    }
    else
    {
        bWaitingForInput = false;
        if (DialogueWidget)
        {
            DialogueWidget->HideNextPrompt();
        }
    }

    OnDialogueShown.Broadcast(Speaker, Text, bWaitingForInput);
}

void UCinematicDirectorComponent::HideDialogue()
{
    if (DialogueWidget)
    {
        DialogueWidget->SetDialogueVisible(false);
        DialogueWidget->HideNextPrompt();
    }
    bWaitingForInput = false;
}

/* ═══════════ Gameplay barks ═══════════ */

void UCinematicDirectorComponent::ShowGameplayDialogue(FText Speaker, FText Text, float DisplayDuration)
{
    if (bCinematicActive)
    {
        return; // Don't clobber an active cinematic.
    }

    EnsureWidget();
    if (!DialogueWidget)
    {
        return;
    }

    DialogueWidget->SetDialogueVisible(true);
    DialogueWidget->SetSpeaker(Speaker, !Speaker.IsEmpty());
    DialogueWidget->SetDialogueText(Text);
    DialogueWidget->HideNextPrompt();
    DialogueWidget->SetSkipBarVisible(false);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BarkTimerHandle);
        if (DisplayDuration > 0.f)
        {
            World->GetTimerManager().SetTimer(BarkTimerHandle, this, &UCinematicDirectorComponent::HideGameplayDialogue, DisplayDuration, false);
        }
    }
}

void UCinematicDirectorComponent::PlayRandomBark(const TArray<FText>& Lines, FText Speaker, float DisplayDuration)
{
    if (Lines.Num() == 0)
    {
        return;
    }

    int32 Index = FMath::RandRange(0, Lines.Num() - 1);
    if (Lines.Num() > 1 && Index == LastBarkIndex)
    {
        Index = (Index + 1) % Lines.Num();
    }
    LastBarkIndex = Index;

    ShowGameplayDialogue(Speaker, Lines[Index], DisplayDuration);
}

void UCinematicDirectorComponent::HideGameplayDialogue()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BarkTimerHandle);
    }
    if (DialogueWidget)
    {
        DialogueWidget->SetDialogueVisible(false);
    }
}

/* ═══════════ Skip ═══════════ */

void UCinematicDirectorComponent::BeginSkip()
{
    if (bSkipping)
    {
        return;
    }

    bSkipping        = true;
    bSkipKeyHeld     = false;
    bWaitingForInput = false;
    SetComponentTickEnabled(false);

    if (DialogueWidget)
    {
        DialogueWidget->SetSkipProgress(1.f);
        DialogueWidget->SetSkipBarVisible(false);
        DialogueWidget->HideNextPrompt();
        DialogueWidget->SetDialogueVisible(false);
    }

    OnCinematicSkipped.Broadcast();

    StartFade(/*bToBlack*/ true);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SkipTimerHandle, this, &UCinematicDirectorComponent::CompleteSkip, FMath::Max(FadeDuration, 0.01f), false);
    }
    else
    {
        CompleteSkip();
    }
}

void UCinematicDirectorComponent::CompleteSkip()
{
    JumpToSkipTargetOrEnd();      // Evaluate the end-state so the final camera change is applied.
    FinishInternal(/*bFadeIn*/ true);
    bSkipping = false;
}

void UCinematicDirectorComponent::JumpToSkipTargetOrEnd()
{
    if (!SequencePlayer)
    {
        return;
    }

    float MarkerSeconds = 0.f;
    if (CurrentSkipMarker != NAME_None && TryGetMarkerTime(CurrentSkipMarker, MarkerSeconds))
    {
        FMovieSceneSequencePlaybackParams Params;
        Params.PositionType = EMovieScenePositionType::MarkedFrame;
        Params.MarkedFrame  = CurrentSkipMarker.ToString();
        Params.UpdateMethod = EUpdatePositionMethod::Jump;
        SequencePlayer->SetPlaybackPosition(Params);
    }
    else
    {
        FMovieSceneSequencePlaybackParams Params;
        Params.PositionType = EMovieScenePositionType::Time;
        Params.Time         = FMath::Max((float)SequencePlayer->GetEndTime().AsSeconds() - UE_KINDA_SMALL_NUMBER, 0.f);
        Params.UpdateMethod = EUpdatePositionMethod::Jump;
        SequencePlayer->SetPlaybackPosition(Params);
    }
}

/* ═══════════ Finish / fade ═══════════ */

void UCinematicDirectorComponent::HandleSequenceFinished()
{
    if (bSkipping)
    {
        return; // The skip flow drives its own finish.
    }
    FinishInternal(/*bFadeIn*/ false);
}

void UCinematicDirectorComponent::FinishInternal(bool bFadeIn)
{
    if (bFinishing)
    {
        return;
    }
    bFinishing = true;

    APlayerController* PC = GetOwningController();

    if (SequencePlayer)
    {
        SequencePlayer->OnFinished.RemoveDynamic(this, &UCinematicDirectorComponent::HandleSequenceFinished);
        SequencePlayer->Stop(); // Releases bound tracks -> player camera resumes.
    }

    if (PC)
    {
        PC->SetCinematicMode(false, false, true, true, true);

        if (CinematicInputContext)
        {
            if (const ULocalPlayer* LP = PC->GetLocalPlayer())
            {
                if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                {
                    Subsys->RemoveMappingContext(CinematicInputContext);
                }
            }
        }

        PC->FlushPressedKeys(); // Clear any held Next/Skip so the pawn doesn't get a stray input.
    }

    if (DialogueWidget)
    {
        DialogueWidget->SetDialogueVisible(false);
        DialogueWidget->SetSkipBarVisible(false);
        DialogueWidget->HideNextPrompt();
    }

    SetComponentTickEnabled(false);

    bCinematicActive = false;
    bWaitingForInput = false;
    bSkipKeyHeld     = false;
    SkipHoldElapsed  = 0.f;

    if (bFadeIn)
    {
        StartFade(/*bToBlack*/ false);
    }

    if (SequenceActor)
    {
        SequenceActor->Destroy();
        SequenceActor = nullptr;
    }
    SequencePlayer  = nullptr;
    CurrentSequence = nullptr;

    OnCinematicFinished.Broadcast();

    bFinishing = false;
}

void UCinematicDirectorComponent::StartFade(bool bToBlack)
{
    if (APlayerController* PC = GetOwningController())
    {
        if (PC->PlayerCameraManager)
        {
            const float From = bToBlack ? 0.f : 1.f;
            const float To   = bToBlack ? 1.f : 0.f;
            PC->PlayerCameraManager->StartCameraFade(From, To, FMath::Max(FadeDuration, 0.f), FadeColor, /*bFadeAudio*/ true, /*bHoldWhenFinished*/ bToBlack);
        }
    }
}

bool UCinematicDirectorComponent::TryGetMarkerTime(FName MarkerLabel, float& OutSeconds) const
{
    if (!CurrentSequence)
    {
        return false;
    }
    UMovieScene* MovieScene = CurrentSequence->GetMovieScene();
    if (!MovieScene)
    {
        return false;
    }

    const FFrameRate TickResolution = MovieScene->GetTickResolution();
    const FString LabelStr = MarkerLabel.ToString();

    for (const FMovieSceneMarkedFrame& Marked : MovieScene->GetMarkedFrames())
    {
        if (Marked.Label == LabelStr)
        {
            OutSeconds = (float)TickResolution.AsSeconds(FFrameTime(Marked.FrameNumber));
            return true;
        }
    }
    return false;
}

/* ═══════════ Widget / input plumbing ═══════════ */

void UCinematicDirectorComponent::EnsureWidget()
{
    if (DialogueWidget)
    {
        return;
    }
    APlayerController* PC = GetOwningController();
    if (!PC || !DialogueWidgetClass)
    {
        if (!DialogueWidgetClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CinematicDirector] DialogueWidgetClass is not set on the component."));
        }
        return;
    }

    DialogueWidget = CreateWidget<UCinematicDialogueWidget>(PC, DialogueWidgetClass);
    if (DialogueWidget)
    {
        DialogueWidget->AddToViewport(100);
        DialogueWidget->SetDialogueVisible(false);
        DialogueWidget->SetSkipBarVisible(false);
        DialogueWidget->HideNextPrompt();
    }
}

void UCinematicDirectorComponent::SetupInput()
{
    if (bInputBound)
    {
        return;
    }

    UEnhancedInputComponent* EIC = GetEnhancedInput();
    if (!EIC)
    {
        return; // InputComponent not ready yet; retried from PlayCinematic.
    }

    if (NextAction)
    {
        // Triggered (not Started): works with a Pressed trigger (fires once on press)
        // or a Tap trigger (fires once on quick release; never on a long hold = skip).
        FEnhancedInputActionEventBinding& B = EIC->BindAction(NextAction, ETriggerEvent::Triggered, this, &UCinematicDirectorComponent::HandleNextInput);
        InputBindingHandles.Add(B.GetHandle());
    }

    if (SkipAction)
    {
        FEnhancedInputActionEventBinding& BStart  = EIC->BindAction(SkipAction, ETriggerEvent::Started,   this, &UCinematicDirectorComponent::HandleSkipStarted);
        FEnhancedInputActionEventBinding& BDone   = EIC->BindAction(SkipAction, ETriggerEvent::Completed, this, &UCinematicDirectorComponent::HandleSkipReleased);
        FEnhancedInputActionEventBinding& BCancel = EIC->BindAction(SkipAction, ETriggerEvent::Canceled,  this, &UCinematicDirectorComponent::HandleSkipReleased);
        InputBindingHandles.Add(BStart.GetHandle());
        InputBindingHandles.Add(BDone.GetHandle());
        InputBindingHandles.Add(BCancel.GetHandle());
    }

    bInputBound = (InputBindingHandles.Num() > 0);
}

void UCinematicDirectorComponent::HandleNextInput()
{
    AdvanceDialogue();
}

void UCinematicDirectorComponent::HandleSkipStarted()
{
    if (!bCinematicActive || !bAllowSkipCurrent || bSkipping)
    {
        return;
    }

    bSkipKeyHeld    = true;
    SkipHoldElapsed = 0.f;

    if (DialogueWidget)
    {
        DialogueWidget->SetSkipProgress(0.f);
        DialogueWidget->SetSkipBarVisible(true);
    }

    SetComponentTickEnabled(true);
}

void UCinematicDirectorComponent::HandleSkipReleased()
{
    if (!bSkipKeyHeld)
    {
        return;
    }

    bSkipKeyHeld    = false;
    SkipHoldElapsed = 0.f;

    if (!bSkipping)
    {
        SetComponentTickEnabled(false);
        if (DialogueWidget)
        {
            DialogueWidget->SetSkipProgress(0.f);
            DialogueWidget->SetSkipBarVisible(false);
        }
    }
}

/* ═══════════ Queries / helpers ═══════════ */

FKey UCinematicDirectorComponent::GetNextActionKey() const
{
    if (!NextAction)
    {
        return EKeys::Invalid;
    }
    if (const APlayerController* PC = GetOwningController())
    {
        if (const ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                const TArray<FKey> Keys = Subsys->QueryKeysMappedToAction(NextAction);
                if (Keys.Num() > 0)
                {
                    return Keys[0];
                }
            }
        }
    }
    return EKeys::Invalid;
}

APlayerController* UCinematicDirectorComponent::GetOwningController() const
{
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        return PC;
    }
    if (const APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        return Cast<APlayerController>(Pawn->GetController());
    }
    return nullptr;
}

UEnhancedInputComponent* UCinematicDirectorComponent::GetEnhancedInput() const
{
    if (const APlayerController* PC = GetOwningController())
    {
        return Cast<UEnhancedInputComponent>(PC->InputComponent);
    }
    return nullptr;
}
