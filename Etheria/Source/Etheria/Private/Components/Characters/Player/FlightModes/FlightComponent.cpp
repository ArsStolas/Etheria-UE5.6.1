/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: FlightComponent - Source
*/

#include "Components/Characters/Player/FlightModes/FlightComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "Components/Characters/Player/FlightModes/Dive/DiveMode.h"
#include "Components/Characters/Player/FlightModes/Glide/GlideMode.h"
#include "GameFramework/CharacterMovementComponent.h"

UFlightComponent::UFlightComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UFlightComponent::BeginPlay()
{
    Super::BeginPlay();

    Owner = Cast<APlayerCharacter>(GetOwner());
    if (!Owner) return;

    GlideMode = NewObject<UGlideMode>(this);
    DiveMode  = NewObject<UDiveMode>(this);

    GlideMode->ConfigureGlideTuning(
        GlideSpeed,
        GlideDescendRate,
        GlideInterpSpeed,
        GlideDescentInterpSpeed,
        GlideMinimumHeight);

    DiveMode->ConfigureDiveTuning(
        DiveMaxSpeed,
        DiveMinSpeed,
        DiveAcceleration,
        DiveDeceleration,
        DiveEntrySpeedBonus,
        DiveWindStreamAcceleration,
        DiveMaxPitch,
        DiveMaxRoll,
        DiveTurnRate,
        DiveLiftFactor);

    GlideMode->Initialize(Owner);
    DiveMode->Initialize(Owner);
}

void UFlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!Owner || !ActiveMode) return;

    ActiveMode->TickMode(DeltaTime);

    // Dive: vérifier contact sol
    if (CurrentMode == EFlightMode::Dive && Owner->IsGrounded())
    {
        StopMode();
        HandleLandingState();
        return;
    }

    // Glide: vérifier marche au sol
    if (CurrentMode == EFlightMode::Glide && Owner->GetCharacterMovement()->IsWalking())
    {
        StopMode();
        return;
    }
}

void UFlightComponent::StartGlide()
{
    if (!Owner || !Owner->GetCharacterMovement()->IsFalling())
        return;

    if (!GlideMode->CanStartGliding())
        return;

    StopMode();
    
    CurrentMode = EFlightMode::Glide;
    ActiveMode = GlideMode;
    ActiveMode->Enter();

    OnGlideStart.Broadcast();
}

void UFlightComponent::StartDive()
{
    if (!Owner || !Owner->GetCharacterMovement()->IsFalling())
        return;

    FHitResult Hit;
    const FVector TraceStart = Owner->GetActorLocation();
    const FVector TraceEnd = TraceStart - Owner->GetActorUpVector() * DiveMinimumHeight;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Owner);

    const bool bTooCloseToGround = Owner->GetWorld()->LineTraceSingleByChannel(
        Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
    if (bTooCloseToGround)
        return;

    // Exit Glide, Enter Dive
    if (ActiveMode)
        ActiveMode->Exit();
    
    CurrentMode = EFlightMode::Dive;
    ActiveMode = DiveMode;
    ActiveMode->Enter();

    OnDiveStart.Broadcast();
}

void UFlightComponent::StopMode()
{
    if (!ActiveMode)
        return;

    const EFlightMode PreviousMode = CurrentMode;

    ActiveMode->Exit();
    ActiveMode = nullptr;
    CurrentMode = EFlightMode::None;

    // Broadcast l'événement correspondant
    if (PreviousMode == EFlightMode::Glide)
        OnGlideStop.Broadcast();
    else if (PreviousMode == EFlightMode::Dive)
        OnDiveStop.Broadcast();
}

void UFlightComponent::HandleLandingState()
{
    if (!Owner || !Owner->GetStateComponent())
        return;

    const int Hor = Owner->GetHorizontalInput();
    const int Ver = Owner->GetVerticalInput();

    Owner->GetStateComponent()->SetMovementState(
        (Hor != 0 || Ver != 0) 
            ? EtheriaTags::State_Movement_Grounded_Walking 
            : EtheriaTags::State_Movement_Grounded_Idle
    );
}
