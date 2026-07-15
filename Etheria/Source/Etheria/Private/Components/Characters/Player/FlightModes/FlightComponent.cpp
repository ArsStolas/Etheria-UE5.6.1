/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: ArsStolas
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
        GlideMinimumHeight,
        GlideWindStreamAcceleration,
        GlideWindEscapeInputThreshold,
        GlideWindEscapeHoldTime);

    DiveMode->ConfigureDiveTuning(
        DiveMaxSpeed,
        DiveMinSpeed,
        DiveCruiseSpeed,
        DiveAcceleration,
        DiveDeceleration,
        DiveEntrySpeedBonus,
        DiveWindStreamAcceleration,
        DiveMaxPitch,
        DiveMaxRoll,
        DiveTurnRate,
        DiveLiftFactor,
        DivePitchResponse,
        DiveRollResponse,
        DiveCruiseInterpSpeed,
        DiveTurnDrag,
        DiveNeutralSinkSpeed,
        DiveMaxSinkSpeed,
        DiveMaxClimbSpeed,
        DiveLowSpeedClimbSink,
        DiveClimbSpeedCostMultiplier,
        bDiveUseInputAttitude,
        bDiveDebugMode);

    GlideMode->Initialize(Owner);
    DiveMode->Initialize(Owner);
}

void UFlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!Owner || !ActiveMode)
    {
        DiveDirection = FVector2D::ZeroVector;
        return;
    }

    if (DiveMode)
    {
        DiveMode->SetDiveDebugMode(bDiveDebugMode);
        DiveMode->SetUseInputAttitude(bDiveUseInputAttitude);
    }

    ActiveMode->TickMode(DeltaTime);
    DiveDirection = (CurrentMode == EFlightMode::Dive && DiveMode)
        ? DiveMode->GetDiveDirection()
        : FVector2D::ZeroVector;

    if (CurrentMode == EFlightMode::Dive && Owner->IsGrounded())
    {
        StopMode();
        HandleLandingState();
        return;
    }

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
    if (!Owner || !Owner->GetCharacterMovement())
        return;

    const bool bCanStartFromAir = Owner->GetCharacterMovement()->IsFalling();
    const bool bCanStartFromGlide = CurrentMode == EFlightMode::Glide && ActiveMode == GlideMode;
    if (!bCanStartFromAir && !bCanStartFromGlide)
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

    const EFlightMode PreviousMode = CurrentMode;
    if (ActiveMode)
    {
        ActiveMode->Exit();
    }
    
    CurrentMode = EFlightMode::Dive;
    ActiveMode = DiveMode;
    DiveMode->SetDiveDebugMode(bDiveDebugMode);
    DiveMode->SetUseInputAttitude(bDiveUseInputAttitude);
    ActiveMode->Enter();

    if (PreviousMode == EFlightMode::Glide)
    {
        OnGlideStop.Broadcast();
    }

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
    DiveDirection = FVector2D::ZeroVector;

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
