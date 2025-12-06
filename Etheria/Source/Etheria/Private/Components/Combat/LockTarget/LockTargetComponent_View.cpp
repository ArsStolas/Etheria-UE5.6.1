/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "LockTargetComponent" - Source (View & Movement)
 * Notes: Handles per-frame validation of the lock target, soft-lock maintenance, camera alignment, movement behavior while locked and debug drawing.
 */

#include "Components/Combat/LockTarget/LockTargetComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void ULockTargetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!CurrentTarget.IsValid())
    {
        // Target was destroyed or became invalid between ticks.
        ClearLock();
        return;
    }

    FVector EyeLoc;
    FVector Forward;
    GetViewLocationAndForward(EyeLoc, Forward);

    // Validate current target against distance / visibility constraints.
    ValidateCurrentTarget(EyeLoc);
    if (!CurrentTarget.IsValid())
    {
        return;
    }

    if (bEnableCameraLock)
    {
        UpdateCameraAndMovement(EyeLoc, DeltaTime);
    }

    if (bEnableSoftLock)
    {
        UpdateLockMaintenance(EyeLoc, Forward);
    }

    if (bDebugDrawLock)
    {
        DebugDrawLock(EyeLoc, Forward);
    }
}

void ULockTargetComponent::ValidateCurrentTarget(const FVector& EyeLoc)
{
    if (!CurrentTarget.IsValid())
    {
        ClearLock();
        return;
    }

    AActor* Target = CurrentTarget.Get();
    if (!Target)
    {
        ClearLock();
        return;
    }

    // Auto-unlock when target becomes invalid or is being destroyed.
    if (bAutoUnlockOnInvalidTarget)
    {
        if (!IsValid(Target))
        {
            ClearLock();
            return;
        }
    }

    // Auto-unlock when target is hidden (for example dead, invisible, or removed from gameplay).
    if (bAutoUnlockOnHiddenTarget)
    {
        if (Target->IsHidden())
        {
            ClearLock();
            return;
        }
    }

    // Auto-unlock when target is too far.
    if (bAutoUnlockOnDistance)
    {
        const float DistSq = FVector::DistSquared(EyeLoc, Target->GetActorLocation());
        if (DistSq > MaxDistance * MaxDistance)
        {
            ClearLock();
            return;
        }
    }
}

void ULockTargetComponent::UpdateLockMaintenance(const FVector& EyeLoc, const FVector& Forward)
{
    if (!CurrentTarget.IsValid())
    {
        return;
    }

    const FVector ToCurrent = (CurrentTarget->GetActorLocation() - EyeLoc).GetSafeNormal();
    const float   Dot       = FVector::DotProduct(ToCurrent, Forward);
    const float   AngleDeg  = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

    // Inside the soft lock cone: keep current target, allow camera offset.
    if (AngleDeg <= MaintainLockAngleDeg)
    {
        return;
    }

    // If auto-switch is disabled, only clear the lock when we are outside the hard maximum angle.
    if (!bEnableAutoSwitch)
    {
        if (AngleDeg > MaxAngleDeg)
        {
            ClearLock();
        }
        return;
    }

    TArray<AActor*> Candidates = GatherCandidates();
    if (Candidates.Num() == 0)
    {
        if (AngleDeg > MaxAngleDeg)
        {
            ClearLock();
        }
        return;
    }

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

    if (Best && Best != CurrentTarget.Get() && AngleDeg >= SwitchLockAngleDeg)
    {
        CurrentTarget = Best;
        UE_LOG(LogTemp, Log, TEXT("[Lock] Auto-switched to %s"), *Best->GetName());
        OnLockChanged.Broadcast(Best);
    }
    else if (AngleDeg > MaxAngleDeg)
    {
        ClearLock();
    }
}

void ULockTargetComponent::UpdateCameraAndMovement(const FVector& EyeLoc, float DeltaTime)
{
    if (!CurrentTarget.IsValid())
    {
        return;
    }

    APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn)
    {
        return;
    }

    AController* Controller = Pawn->GetController();
    if (!Controller)
    {
        return;
    }

    // Focus slightly above target origin (chest/head instead of feet).
    FVector TargetLoc = CurrentTarget->GetActorLocation();
    TargetLoc.Z += TargetHeightOffset;

    const FRotator Desired = (TargetLoc - EyeLoc).Rotation();
    FRotator CurrentRot = Controller->GetControlRotation();

    // Yaw lock (always when camera lock is enabled).
    const float NewYaw = FMath::FInterpTo(
        CurrentRot.Yaw,
        Desired.Yaw,
        DeltaTime,
        CameraYawInterpSpeed
    );
    CurrentRot.Yaw = NewYaw;

    // Optional pitch lock.
    if (bLockPitch)
    {
        float NewPitch = FMath::FInterpTo(
            CurrentRot.Pitch,
            Desired.Pitch,
            DeltaTime,
            CameraPitchInterpSpeed
        );

        if (bClampPitch)
        {
            NewPitch = FMath::Clamp(NewPitch, MinPitchDeg, MaxPitchDeg);
        }

        CurrentRot.Pitch = NewPitch;
    }

    Controller->SetControlRotation(CurrentRot);

    ApplyMovementSettingsForLock(true);
}

void ULockTargetComponent::ApplyMovementSettingsForLock(bool bLocked)
{
    if (!bUseLockMovementMode)
    {
        return;
    }

    ACharacter* Char = Cast<ACharacter>(GetOwner());
    if (!Char)
    {
        return;
    }

    UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();
    if (!MoveComp)
    {
        return;
    }

    if (bLocked)
    {
        Char->bUseControllerRotationYaw     = true;
        MoveComp->bOrientRotationToMovement = false;
    }
    else
    {
        Char->bUseControllerRotationYaw     = false;
        MoveComp->bOrientRotationToMovement = true;
    }
}

void ULockTargetComponent::DebugDrawLock(const FVector& EyeLoc, const FVector& Forward) const
{
    if (!bDebugDrawLock)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float ConeLength = 250.f;

    // Draw main forward line.
    DrawDebugLine(World, EyeLoc, EyeLoc + Forward * ConeLength, FColor::Cyan, false, 0.f, 0, 1.f);

    if (CurrentTarget.IsValid())
    {
        const FVector TargetLoc = CurrentTarget->GetActorLocation();
        DrawDebugSphere(World, TargetLoc, 60.f, 16, FColor::Yellow, false, 0.f, 0, 1.5f);
        DrawDebugLine(World, EyeLoc, TargetLoc, FColor::Yellow, false, 0.f, 0, 0.5f);
    }
}
