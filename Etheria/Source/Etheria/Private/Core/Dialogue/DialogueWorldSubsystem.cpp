/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDialogueWorldSubsystem" - Source
 */
#include "Core/Dialogue/DialogueWorldSubsystem.h"
#include "Core/Dialogue/DialogueBubbleActor.h"
#include "Core/Dialogue/Components/PlayerDialogueComponent.h"
#include "Core/Dialogue/DialogueSystemSettings.h"
#include "Core/Cinematics/CinematicWorldSubsystem.h"
#include "Core/Cinematics/CinematicCameraRigBase.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UDialogueWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

const UDialogueSystemSettings* Settings = UDialogueSystemSettings::Get();
if (Settings)
{
    if (Settings->BubbleActorClass)
    {
        BubbleActorClass = Settings->BubbleActorClass;
    }
    if (Settings->DefaultBubbleWidgetClass)
    {
        DefaultBubbleWidgetClass = Settings->DefaultBubbleWidgetClass;
    }
    PrewarmBubblePoolSize = Settings->PrewarmBubblePoolSize;
    MaxBubbleDistance = Settings->MaxBubbleDistance;
}

    if (!BubbleActorClass)
    {
        BubbleActorClass = ADialogueBubbleActor::StaticClass();
    }

    PrewarmPool();
    StartBubbleCulling();
}

void UDialogueWorldSubsystem::Deinitialize()
{
    EndDialogueSession(EDialogueEndReason::Cancelled);
    HideAllBubbles();
    StopBubbleCulling();

    Super::Deinitialize();
}

void UDialogueWorldSubsystem::PrewarmPool()
{
    UWorld* World = GetWorld();
    if (!World || !BubbleActorClass)
    {
        return;
    }

    for (int32 i = BubblePool.Num(); i < PrewarmBubblePoolSize; ++i)
    {
        ADialogueBubbleActor* Bubble = World->SpawnActor<ADialogueBubbleActor>(BubbleActorClass);
        if (Bubble)
        {
            Bubble->Deactivate();
            BubblePool.Add(Bubble);
        }
    }
}

ADialogueBubbleActor* UDialogueWorldSubsystem::AcquireBubble()
{
    UWorld* World = GetWorld();
    if (!World || !BubbleActorClass)
    {
        return nullptr;
    }

    for (int32 i = BubblePool.Num() - 1; i >= 0; --i)
    {
        if (BubblePool[i] && !BubblePool[i]->IsActive())
        {
            ADialogueBubbleActor* Bubble = BubblePool[i];
            BubblePool.RemoveAt(i);
            return Bubble;
        }
    }

    // Spawn on demand
    return World->SpawnActor<ADialogueBubbleActor>(BubbleActorClass);
}

void UDialogueWorldSubsystem::ReleaseBubble(ADialogueBubbleActor* Bubble)
{
    if (!Bubble)
    {
        return;
    }

    Bubble->Deactivate();
    BubblePool.Add(Bubble);
}

bool UDialogueWorldSubsystem::ShowBubble(AActor* Speaker, const FDialogueBubbleRequest& Request)
{
    if (!Speaker)
    {
        return false;
    }

    // If already active for this speaker, update text and reset timer
    if (ADialogueBubbleActor* Existing = ActiveBubbles.FindRef(Speaker))
    {
        Existing->SetText(Request.Text);

        if (UWorld* World = GetWorld())
        {
            FTimerHandle& Handle = BubbleTimers.FindOrAdd(Speaker);
            World->GetTimerManager().ClearTimer(Handle);

            if (!Request.bPersistent)
            {
                World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &UDialogueWorldSubsystem::HideBubbleWeak, TWeakObjectPtr<AActor>(Speaker)), Request.Duration, false);
            }
        }
        return true;
    }

    ADialogueBubbleActor* Bubble = AcquireBubble();
    if (!Bubble)
    {
        return false;
    }

    const TSubclassOf<UUserWidget> WidgetClassToUse = Request.BubbleWidgetClass ? Request.BubbleWidgetClass : DefaultBubbleWidgetClass;
    Bubble->ActivateForSpeaker(Speaker, WidgetClassToUse, Request.RelativeOffset);
    Bubble->SetText(Request.Text);

    ActiveBubbles.Add(Speaker, Bubble);

    if (UWorld* World = GetWorld())
    {
        if (!Request.bPersistent)
        {
            FTimerHandle& Handle = BubbleTimers.FindOrAdd(Speaker);
            World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &UDialogueWorldSubsystem::HideBubbleWeak, TWeakObjectPtr<AActor>(Speaker)), Request.Duration, false);
        }
    }

    return true;
}

void UDialogueWorldSubsystem::HideBubble(AActor* Speaker)
{
    if (!Speaker)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        if (FTimerHandle* Handle = BubbleTimers.Find(Speaker))
        {
            World->GetTimerManager().ClearTimer(*Handle);
            BubbleTimers.Remove(Speaker);
        }
    }

    if (ADialogueBubbleActor* Bubble = ActiveBubbles.FindRef(Speaker))
    {
        ActiveBubbles.Remove(Speaker);
        ReleaseBubble(Bubble);
    }
}

void UDialogueWorldSubsystem::HideAllBubbles()
{
    TArray<TWeakObjectPtr<AActor>> Keys;
    ActiveBubbles.GetKeys(Keys);

    for (const TWeakObjectPtr<AActor>& Key : Keys)
    {
        if (Key.IsValid())
        {
            HideBubble(Key.Get());
        }
    }

    ActiveBubbles.Empty();
    BubbleTimers.Empty();
}

bool UDialogueWorldSubsystem::StartDialogueSession(const FDialogueSessionRequest& Request)
{
    if (bDialogueActive)
    {
        EndDialogueSession(EDialogueEndReason::Interrupted);
    }

    ActiveRequest = Request;
    bDialogueActive = true;
    bWaitingForChoice = false;

    ApplyDialoguePolicy();

    OnDialogueSessionStarted.Broadcast(ActiveRequest);

    if (ActiveRequest.InitialChoices.Num() > 0)
    {
        RequestChoices(ActiveRequest.InitialChoices);
    }

    if (ActiveRequest.Lines.Num() > 0)
    {
        CurrentLineIndex = 0;
        PlayLine(CurrentLineIndex);
    }
    else
    {
        CurrentLineIndex = INDEX_NONE;
    }

    return true;
}

void UDialogueWorldSubsystem::EndDialogueSession(EDialogueEndReason Reason)
{
    if (!bDialogueActive)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(LineTimerHandle);
    }

    // Release dialogue camera (if any)
    if (UCinematicWorldSubsystem* Cinematics = GetWorld() ? GetWorld()->GetSubsystem<UCinematicWorldSubsystem>() : nullptr)
    {
        Cinematics->PopCameraLayer(TEXT("Dialogue"), ActiveRequest.Policy.CameraBlendTime);
    }

    RestoreDialoguePolicy();

    bDialogueActive = false;
    bWaitingForChoice = false;
    CurrentLineIndex = INDEX_NONE;

    OnDialogueSessionEnded.Broadcast(Reason);
}

void UDialogueWorldSubsystem::NextLine()
{
    if (!bDialogueActive || bWaitingForChoice)
    {
        return;
    }

    const int32 NextIndex = (CurrentLineIndex == INDEX_NONE) ? 0 : (CurrentLineIndex + 1);
    if (ActiveRequest.Lines.IsValidIndex(NextIndex))
    {
        CurrentLineIndex = NextIndex;
        PlayLine(CurrentLineIndex);
    }
    else
    {
        EndDialogueSession(EDialogueEndReason::Completed);
    }
}

void UDialogueWorldSubsystem::SkipCurrentLine()
{
    if (!bDialogueActive)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(LineTimerHandle);
    }

    NextLine();
}

void UDialogueWorldSubsystem::PlayLine(int32 LineIndex)
{
    if (!bDialogueActive || !ActiveRequest.Lines.IsValidIndex(LineIndex))
    {
        return;
    }

    const FDialogueLine& Line = ActiveRequest.Lines[LineIndex];

    UpdateDialogueCameraTarget(Line);

    OnDialogueLineStarted.Broadcast(Line, LineIndex);

    if (ActiveRequest.Policy.bShowSpeechBubbles)
    {
        FDialogueBubbleRequest BubbleReq;
        BubbleReq.Text = Line.Text;
        BubbleReq.Duration = (Line.Duration > 0.0f) ? Line.Duration : ActiveRequest.DefaultLineDuration;
        ShowBubble(Line.Speaker.Get(), BubbleReq);
    }

    if (ActiveRequest.bAutoPlayLines)
    {
        const float Duration = (Line.Duration > 0.0f) ? Line.Duration : ActiveRequest.DefaultLineDuration;

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(LineTimerHandle, this, &UDialogueWorldSubsystem::HandleLineTimer, Duration, false);
        }
    }
}

void UDialogueWorldSubsystem::HandleLineTimer()
{
    NextLine();
}

void UDialogueWorldSubsystem::RequestChoices(const TArray<FDialogueChoice>& Choices)
{
    if (!bDialogueActive)
    {
        return;
    }

    bWaitingForChoice = true;

    // Your UI can bind to this and display choices.
    OnDialogueChoicesRequested.Broadcast(Choices, true);
}

void UDialogueWorldSubsystem::SubmitChoice(int32 ChoiceId)
{
    if (!bDialogueActive)
    {
        return;
    }

    // Subsystem doesn't enforce logic; your BP decides what to do with ChoiceId.
    bWaitingForChoice = false;
}

void UDialogueWorldSubsystem::ApplyDialoguePolicy()
{
    if (UPlayerDialogueComponent* PlayerComp = ResolvePlayerDialogueComponent())
    {
        PlayerComp->ApplyDialoguePolicy(ActiveRequest.Policy);
    }

    // Optional camera focus
    if (ActiveRequest.Policy.CameraMode != EDialogueCameraMode::None)
    {
        if (UCinematicWorldSubsystem* Cinematics = GetWorld() ? GetWorld()->GetSubsystem<UCinematicWorldSubsystem>() : nullptr)
        {
            AActor* FocusTarget = ActiveRequest.Target.Get();
            if (!FocusTarget && ActiveRequest.Lines.Num() > 0)
            {
                FocusTarget = ActiveRequest.Lines[0].Speaker.Get();
            }

            if (FocusTarget && ActiveRequest.Policy.FocusRigClass)
            {
                Cinematics->PushCameraRigLayer(TEXT("Dialogue"), ActiveRequest.Policy.FocusRigClass, FocusTarget, ActiveRequest.Policy.CameraBlendTime);
            }
        }
    }
}

void UDialogueWorldSubsystem::RestoreDialoguePolicy()
{
    if (UPlayerDialogueComponent* PlayerComp = ResolvePlayerDialogueComponent())
    {
        PlayerComp->RestoreDialoguePolicy();
    }
}

UPlayerDialogueComponent* UDialogueWorldSubsystem::ResolvePlayerDialogueComponent() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (PC)
    {
        if (UPlayerDialogueComponent* CompOnPC = PC->FindComponentByClass<UPlayerDialogueComponent>())
        {
            return CompOnPC;
        }

        if (APawn* Pawn = PC->GetPawn())
        {
            return Pawn->FindComponentByClass<UPlayerDialogueComponent>();
        }
    }

    return nullptr;
}

void UDialogueWorldSubsystem::StartBubbleCulling()
{
    if (MaxBubbleDistance <= 0.0f)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(BubbleCullTimer, this, &UDialogueWorldSubsystem::HandleBubbleCullingTick, 0.25f, true);
    }
}

void UDialogueWorldSubsystem::StopBubbleCulling()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BubbleCullTimer);
    }
}

void UDialogueWorldSubsystem::HandleBubbleCullingTick()
{
    if (MaxBubbleDistance <= 0.0f)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
    if (!PlayerPawn)
    {
        return;
    }

    for (auto& It : ActiveBubbles)
    {
        AActor* Speaker = It.Key.Get();
        ADialogueBubbleActor* Bubble = It.Value;

        if (!Speaker || !Bubble)
        {
            continue;
        }

        const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), Speaker->GetActorLocation());
        const bool bVisible = (Dist <= MaxBubbleDistance);

        Bubble->SetActorHiddenInGame(!bVisible);
        Bubble->SetActorTickEnabled(bVisible);
    }
}

void UDialogueWorldSubsystem::HideBubbleWeak(TWeakObjectPtr<AActor> Speaker)
{
    if (Speaker.IsValid())
    {
        HideBubble(Speaker.Get());
    }
}

void UDialogueWorldSubsystem::UpdateDialogueCameraTarget(const FDialogueLine& Line)
{
    if (ActiveRequest.Policy.CameraMode != EDialogueCameraMode::FocusSpeaker)
    {
        return;
    }

    if (UCinematicWorldSubsystem* Cinematics = GetWorld() ? GetWorld()->GetSubsystem<UCinematicWorldSubsystem>() : nullptr)
    {
        if (ACinematicCameraRigBase* Rig = Cinematics->GetActiveRigForLayer(TEXT("Dialogue")))
        {
            Rig->SetTargetActor(Line.Speaker.Get());
        }
    }
}
