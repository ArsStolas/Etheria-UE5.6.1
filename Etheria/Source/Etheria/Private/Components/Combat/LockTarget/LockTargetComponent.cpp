/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "LockTargetComponent" - Source (Core)
 * Notes: Implements core lock-on logic (toggle, clear, switch, candidate search and scoring) and controls when the component tick is enabled.
 */

#include "Components/Combat/LockTarget/LockTargetComponent.h"

#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

ULockTargetComponent::ULockTargetComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    // We only tick while a lock is active and at least one runtime feature needs updates.
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULockTargetComponent::BeginPlay()
{
    Super::BeginPlay();
    SetComponentTickEnabled(false);
}

void ULockTargetComponent::GetViewLocationAndForward(FVector& OutLocation, FVector& OutForward) const
{
    OutLocation = FVector::ZeroVector;
    OutForward  = FVector::ForwardVector;

    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    OutLocation = Owner->GetActorLocation();
    OutForward  = Owner->GetActorForwardVector();

    if (const APawn* Pawn = Cast<APawn>(Owner))
    {
        OutLocation = Pawn->GetPawnViewLocation();

        if (const AController* Controller = Pawn->GetController())
        {
            const FRotator ViewRot = Controller->GetControlRotation();
            OutForward = ViewRot.Vector();
        }
    }
}

bool ULockTargetComponent::ToggleLock(AActor* Preferred)
{
    // If already locked, clear and exit.
    if (CurrentTarget.IsValid())
    {
        ClearLock();
        return false;
    }

    // Try to find a new target.
    CurrentTarget = FindBestTarget(Preferred);

    // No valid candidate: do nothing (no DOF, no cinematic, movement stays normal).
    if (!CurrentTarget.IsValid())
    {
        ApplyMovementSettingsForLock(false);
        UE_LOG(LogTemp, Verbose, TEXT("[Lock] ToggleLock: no valid target found."));
        return false;
    }

    // We have a valid target: apply lock movement style if enabled.
    ApplyMovementSettingsForLock(true);

    // Enable ticking only if at least one runtime feature needs it.
    const bool bNeedsTick =
        bEnableCameraLock || bEnableSoftLock || bDebugDrawLock ||
        bAutoUnlockOnDistance || bAutoUnlockOnHiddenTarget || bAutoUnlockOnInvalidTarget;

    if (bNeedsTick)
    {
        SetComponentTickEnabled(true);
    }

    UE_LOG(LogTemp, Log, TEXT("[Lock] ToggleLock: locked onto %s"), *CurrentTarget->GetName());
    OnLockChanged.Broadcast(CurrentTarget.Get());

    return true;
}

void ULockTargetComponent::ClearLock()
{
    const bool bHadTarget = CurrentTarget.IsValid();
    CurrentTarget = nullptr;

    if (bHadTarget)
    {
        ApplyMovementSettingsForLock(false);
        SetComponentTickEnabled(false);
    }

    OnLockChanged.Broadcast(nullptr);
}

bool ULockTargetComponent::SwitchTarget(bool bRight)
{
    if (!CurrentTarget.IsValid())
    {
        return false;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    TArray<AActor*> Candidates = GatherCandidates();
    if (Candidates.Num() == 0)
    {
        return false;
    }

    FVector EyeLoc;
    FVector Forward;
    GetViewLocationAndForward(EyeLoc, Forward);

    const FVector Right = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();

    float   BestScore = -FLT_MAX;
    AActor* Best      = nullptr;

    for (AActor* C : Candidates)
    {
        if (!C || C == CurrentTarget.Get())
        {
            continue;
        }

        const FVector Dir  = (C->GetActorLocation() - EyeLoc).GetSafeNormal();
        const float   Side = FVector::DotProduct(Dir, Right);

        if (bRight && Side < 0.f)
        {
            continue;
        }
        if (!bRight && Side > 0.f)
        {
            continue;
        }

        const float Score = ScoreCandidate(C, EyeLoc, Forward);
        if (Score > BestScore)
        {
            BestScore = Score;
            Best      = C;
        }
    }

    if (Best)
    {
        CurrentTarget = Best;
        UE_LOG(LogTemp, Log, TEXT("[Lock] SwitchTarget: switched to %s"), *Best->GetName());
        OnLockChanged.Broadcast(Best);
        return true;
    }

    return false;
}

AActor* ULockTargetComponent::FindBestTarget(AActor* Preferred) const
{
    if (Preferred)
    {
        return Preferred;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    TArray<AActor*> Candidates = GatherCandidates();
    UE_LOG(LogTemp, Verbose, TEXT("[Lock] FindBestTarget: %d candidates."), Candidates.Num());
    if (Candidates.Num() == 0)
    {
        return nullptr;
    }

    FVector EyeLoc;
    FVector Forward;
    GetViewLocationAndForward(EyeLoc, Forward);

    float   BestScore = -FLT_MAX;
    AActor* Best      = nullptr;

    for (AActor* C : Candidates)
    {
        if (!C)
        {
            continue;
        }

        const float Score = ScoreCandidate(C, EyeLoc, Forward);
        if (Score > BestScore)
        {
            BestScore = Score;
            Best      = C;
        }
    }

    return Best;
}

TArray<AActor*> ULockTargetComponent::GatherCandidates() const
{
    TArray<AActor*> Result;

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return Result;
    }

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
    ObjTypes.Add(UEngineTypes::ConvertToObjectType(TargetChannel));

    TArray<AActor*> Ignore;
    Ignore.Add(Owner);

    TArray<AActor*> OutActors;
    const bool bHit = UKismetSystemLibrary::SphereOverlapActors(
        Owner,
        Owner->GetActorLocation(),
        MaxDistance,
        ObjTypes,
        AActor::StaticClass(),
        Ignore,
        OutActors
    );

    if (!bHit)
    {
        return Result;
    }

    FVector EyeLoc;
    FVector Forward;
    GetViewLocationAndForward(EyeLoc, Forward);

    for (AActor* A : OutActors)
    {
        if (!A || A == Owner)
        {
            continue;
        }

        // Only Pawns are eligible when we require a hostile tag.
        APawn* Pawn = Cast<APawn>(A);
        if (!Pawn)
        {
            continue;
        }

        // If we require a hostile tag, make sure this Pawn has it.
        if (bRequireHostileTag && !A->ActorHasTag(HostileTagName))
        {
            continue;
        }
        
        const float DistSq = FVector::DistSquared(EyeLoc, A->GetActorLocation());
        if (DistSq > MaxDistance * MaxDistance)
        {
            continue;
        }

        const FVector Dir   = (A->GetActorLocation() - EyeLoc).GetSafeNormal();
        const float   Dot   = FVector::DotProduct(Dir, Forward);
        const float   Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

        if (Angle <= MaxAngleDeg)
        {
            Result.Add(A);
        }
    }

    return Result;
}

float ULockTargetComponent::ScoreCandidate(AActor* Candidate, const FVector& EyeLoc, const FVector& Forward) const
{
    const FVector Dir  = (Candidate->GetActorLocation() - EyeLoc).GetSafeNormal();
    const float   Dot  = FVector::DotProduct(Dir, Forward);
    const float   Dist = FVector::Dist(EyeLoc, Candidate->GetActorLocation());

    // Favor candidates close to the view center (Dot) and relatively close in distance.
    return Dot * 2.0f + (1.0f - FMath::Clamp(Dist / MaxDistance, 0.f, 1.f));
}
