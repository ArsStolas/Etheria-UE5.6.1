/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UBarkComponent" - Source
 */
#include "Core/Dialogue/Components/BarkComponent.h"
#include "Core/Dialogue/DialogueWorldSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UBarkComponent::UBarkComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UBarkComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoStart)
    {
        StartAmbientBarks();
    }
}

void UBarkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopAmbientBarks();
    Super::EndPlay(EndPlayReason);
}

void UBarkComponent::StartAmbientBarks()
{
    if (bRunning)
    {
        return;
    }

    bRunning = true;
    ScheduleNext();
}

void UBarkComponent::StopAmbientBarks()
{
    if (!bRunning)
    {
        return;
    }

    bRunning = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BarkTimer);
    }
}

void UBarkComponent::PlayBarkOnce(const FDialogueBubbleRequest& Request)
{
    if (UWorld* World = GetWorld())
    {
        if (UDialogueWorldSubsystem* Dialogue = World->GetSubsystem<UDialogueWorldSubsystem>())
        {
            Dialogue->ShowBubble(GetOwner(), Request);
        }
    }
}

void UBarkComponent::ScheduleNext()
{
    if (!bRunning)
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        const float Delay = FMath::FRandRange(MinInterval, MaxInterval);
        World->GetTimerManager().SetTimer(BarkTimer, this, &UBarkComponent::HandleBarkTimer, Delay, false);
    }
}

void UBarkComponent::HandleBarkTimer()
{
    if (!bRunning)
    {
        return;
    }

    if (BarkTexts.Num() == 0)
    {
        ScheduleNext();
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ScheduleNext();
        return;
    }

    // Distance gate
    if (MaxDistanceToPlayer > 0.0f)
    {
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
        if (PlayerPawn)
        {
            const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), GetOwner()->GetActorLocation());
            if (Dist > MaxDistanceToPlayer)
            {
                ScheduleNext();
                return;
            }
        }
    }

    const int32 Index = FMath::RandRange(0, BarkTexts.Num() - 1);

    FDialogueBubbleRequest Req;
    Req.Text = BarkTexts[Index];
    Req.Duration = 2.5f;
    Req.bPersistent = false;

    PlayBarkOnce(Req);

    ScheduleNext();
}
