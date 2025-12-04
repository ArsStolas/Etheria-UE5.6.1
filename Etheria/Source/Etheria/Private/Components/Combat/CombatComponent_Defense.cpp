/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "CombatComponent - Source (Defense)"
 * Notes: Parry / dodge / perfect window helpers.
 */

#include "Components/Combat/CombatComponent.h"

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/BaseCharacter.h"
#include "Components/Characters/CharacterStateComponent.h"

#pragma region PARRY / DODGE

void UCombatComponent::SetParryHeld(bool bHeld)
{
    if (bParryHeld == bHeld) return;
    bParryHeld = bHeld;

    if (StateComp.IsValid())
    {
        StateComp->SetCombatState(bHeld ? EtheriaTags::State_Combat_Blocking : EtheriaTags::State_Combat);
    }

    if (MoveComp.IsValid() && BaseWalkSpeed > 0.f)
    {
        const float Mult = bParryHeld ? ParryMoveSpeedMultiplier : 1.f;
        MoveComp->MaxWalkSpeed = BaseWalkSpeed * Mult;
    }
}

void UCombatComponent::BeginPerfectParryWindow()
{
    bPerfectParryWindow = true;
    OnCue.Broadcast(FName("PerfectParryWindow"), ECombatCuePhase::Start);
}

void UCombatComponent::EndPerfectParryWindow()
{
    bPerfectParryWindow = false;
    OnCue.Broadcast(FName("PerfectParryWindow"), ECombatCuePhase::End);
}

void UCombatComponent::StartDodgeIFrames(float DurationOverride)
{
    if (bInDodgeIFrames) return;
    bInDodgeIFrames = true;

    const float Duration = (DurationOverride > 0.f) ? DurationOverride : DodgeIFrameDuration;
    OnCue.Broadcast(FName("DodgeIFrames"), ECombatCuePhase::Start);

    if (StateComp.IsValid())
    {
        StateComp->SetCombatState(EtheriaTags::State_Combat_Dodging);
    }

    if (UWorld* W = GetWorld())
    {
        FTimerHandle H;
        W->GetTimerManager().SetTimer(H, [this]()
        {
            bInDodgeIFrames = false;
            OnCue.Broadcast(FName("DodgeIFrames"), ECombatCuePhase::End);
        }, Duration, false);
    }
}

void UCombatComponent::BeginPerfectDodgeWindow()
{
    bPerfectDodgeWindow = true;
    OnCue.Broadcast(FName("PerfectDodgeWindow"), ECombatCuePhase::Start);
}

void UCombatComponent::EndPerfectDodgeWindow()
{
    bPerfectDodgeWindow = false;
    OnCue.Broadcast(FName("PerfectDodgeWindow"), ECombatCuePhase::End);
}

bool UCombatComponent::CanStartDodge() const
{
    if (!OwnerCharacter.IsValid())
    {
        return false;
    }

    // Already in i-frames -> don't stack dodges.
    if (bInDodgeIFrames)
    {
        return false;
    }

    // If you want to allow cancel out of attacks, remove this check.
    if (IsAttackActive())
    {
        return false;
    }

    return true;
}

/**
 * Summary: Handles a dodge tap (sprint double-tap) using a world-space movement direction.
 */
void UCombatComponent::HandleDodgeInputTap(const FVector& WorldDirection)
{
    if (!CanStartDodge())
    {
        return;
    }

    FVector Dir = WorldDirection;
    Dir.Z = 0.f;

    if (Dir.IsNearlyZero())
    {
        // No direction -> ignore tap
        return;
    }

    Dir.Normalize();

    const float MagnitudeSq = Dir.SizeSquared();
    if (MagnitudeSq < FMath::Square(DodgeMinInputThreshold))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float Now = World->GetTimeSeconds();

    // Second tap in time window -> real dodge.
    if (bDodgeTapPending && (Now - LastDodgeTapTime) <= DodgeDoubleTapMaxDelay)
    {
        bDodgeTapPending   = false;
        LastDodgeTapTime   = 0.f;
        LastDodgeDirection = FVector::ZeroVector;

        TryDodgeWorldDirection(Dir);
        return;
    }

    // First tap, just store time & direction.
    bDodgeTapPending   = true;
    LastDodgeTapTime   = Now;
    LastDodgeDirection = Dir;
}

/**
 * Summary: Tries to start a directional dodge based on a world-space direction.
 */
bool UCombatComponent::TryDodgeWorldDirection(const FVector& WorldDirection)
{
    if (!CanStartDodge())
    {
        return false;
    }

    FVector Dir = WorldDirection;
    Dir.Z = 0.f;

    if (Dir.IsNearlyZero())
    {
        if (!OwnerCharacter.IsValid())
        {
            return false;
        }

        Dir = OwnerCharacter->GetActorForwardVector();
        Dir.Z = 0.f;
        if (Dir.IsNearlyZero())
        {
            return false;
        }
    }

    Dir.Normalize();

    FVector Forward = FVector::ForwardVector;
    FVector Right   = FVector::RightVector;

    if (OwnerCharacter.IsValid())
    {
        Forward = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
        Right   = OwnerCharacter->GetActorRightVector().GetSafeNormal2D();
    }

    const float FwdDot   = FVector::DotProduct(Dir, Forward);
    const float RightDot = FVector::DotProduct(Dir, Right);

    const float AbsFwd   = FMath::Abs(FwdDot);
    const float AbsRight = FMath::Abs(RightDot);

    EDodgeDirection DodgeDir;
    if (AbsFwd >= AbsRight)
    {
        DodgeDir = (FwdDot >= 0.f) ? EDodgeDirection::Forward : EDodgeDirection::Backward;
    }
    else
    {
        DodgeDir = (RightDot >= 0.f) ? EDodgeDirection::Right : EDodgeDirection::Left;
    }

    return TryDodgeDirection(DodgeDir);
}

/**
 * Summary: Tries to start a dodge in a fixed enum direction.
 */
bool UCombatComponent::TryDodgeDirection(EDodgeDirection DodgeDirection)
{
    if (!CanStartDodge())
    {
        return false;
    }

    UAnimMontage* Montage = GetDodgeMontage(DodgeDirection);
    if (!Montage || !OwnerCharacter.IsValid() || !OwnerMesh.IsValid())
    {
        return false;
    }

    // Enter dodge i-frames (perfect windows & state are already handled inside StartDodgeIFrames).
    StartDodgeIFrames(-1.f);

    // Block jump & crouch during the dodge.
    PushInputLock(FName("Dodge"), true, true);

    const float Duration = OwnerCharacter->PlayAnimMontage(Montage);
    if (Duration <= 0.f)
    {
        PopInputLock(FName("Dodge"));
        return false;
    }

    if (UWorld* World = GetWorld())
    {
        FTimerHandle Handle;
        World->GetTimerManager().SetTimer(
            Handle,
            FTimerDelegate::CreateUObject(this, &UCombatComponent::PopInputLock, FName("Dodge")),
            Duration,
            false
        );
    }

    OnCue.Broadcast(FName("Dodge"), ECombatCuePhase::Start);
    return true;
}

/**
 * Summary: Returns the dodge montage corresponding to a direction.
 */
UAnimMontage* UCombatComponent::GetDodgeMontage(EDodgeDirection Direction) const
{
    switch (Direction)
    {
        case EDodgeDirection::Forward:  return DodgeForwardMontage;
        case EDodgeDirection::Backward: return DodgeBackwardMontage;
        case EDodgeDirection::Left:     return DodgeLeftMontage;
        case EDodgeDirection::Right:    return DodgeRightMontage;
        default:                        return nullptr;
    }
}

void UCombatComponent::ApplyPerfectBoost(EPerfectKind Kind)
{
    if (MoveComp.IsValid() && BaseWalkSpeed > 0.f)
    {
        MoveComp->MaxWalkSpeed = BaseWalkSpeed * PerfectMoveSpeedMultiplier;
    }
    if (OwnerMesh.IsValid())
    {
        OwnerMesh->GlobalAnimRateScale = BaseGlobalAnimRate * PerfectAnimRateMultiplier;
    }

    if (UWorld* W = GetWorld())
    {
        FTimerHandle H;
        W->GetTimerManager().SetTimer(H, [this](){ RestoreBoosts(); }, PerfectBoostDuration, false);
    }
}

void UCombatComponent::RestoreBoosts()
{
    if (MoveComp.IsValid() && BaseWalkSpeed > 0.f)
    {
        const float ParryMult = bParryHeld ? ParryMoveSpeedMultiplier : 1.f;
        MoveComp->MaxWalkSpeed = BaseWalkSpeed * ParryMult;
    }
    if (OwnerMesh.IsValid())
    {
        OwnerMesh->GlobalAnimRateScale = BaseGlobalAnimRate;
    }
}

#pragma endregion
