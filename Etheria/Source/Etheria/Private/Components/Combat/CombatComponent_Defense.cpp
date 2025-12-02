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
