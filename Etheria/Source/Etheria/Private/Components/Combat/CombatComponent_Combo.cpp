/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "CombatComponent - Source (Combos)"
 * Notes: Combo selection and progression runtime.
 */

#include "Components/Combat/CombatComponent.h"

#include "Engine/World.h"

#pragma region COMBO

void UCombatComponent::BeginComboWindow(FName ComboId)
{
    bComboWindowOpen = true;
    if (ComboId != NAME_None) ActiveComboId = ComboId;
}

void UCombatComponent::EndComboWindow(FName ComboId)
{
    bComboWindowOpen = false;
    AdvanceComboIfRequested();
}

void UCombatComponent::RequestComboAdvance()
{
    bComboAdvanceRequested = true;
    if (UWorld* W = GetWorld())
    {
        ComboBufferExpireAt = W->GetTimeSeconds() + FMath::Max(0.0f, MaxComboBufferTime);
    }
}

void UCombatComponent::AdvanceComboIfRequested()
{
    if (!bComboAdvanceRequested) return;

    if (UWorld* W = GetWorld())
    {
        if (ComboBufferExpireAt > 0.0f && W->GetTimeSeconds() > ComboBufferExpireAt)
        {
            bComboAdvanceRequested = false;
            ComboBufferExpireAt = 0.0f;
            return;
        }
    }

    const FComboSpecConfig* Combo = nullptr;

    if (ActiveComboId != NAME_None)
    {
        Combo = Combos.FindByPredicate([&](const FComboSpecConfig& C){ return C.ComboId == ActiveComboId; });
    }

    if (!Combo)
    {
        Combo = Combos.FindByPredicate([&](const FComboSpecConfig& C)
        {
            return C.Steps.Num() > 0 && C.Steps[0].AttackId == LastAttackId;
        });
        if (Combo) ActiveComboId = Combo->ComboId;
    }

    if (!Combo || Combo->Steps.Num() == 0)
    {
        bComboAdvanceRequested = false;
        return;
    }

    // Compute next index based on our known ActiveComboStep (seeded at TryAttackById).
    int32 NextIndex = (ActiveComboStep < 0) ? 0 : ActiveComboStep + 1;
    if (!Combo->Steps.IsValidIndex(NextIndex))
    {
        // End of combo
        ActiveComboId = NAME_None;
        ActiveComboStep = -1;
        bComboAdvanceRequested = false;
        ComboBufferExpireAt = 0.0f;
        return;
    }

    ActiveComboStep = NextIndex;

    const FComboStepConfig& Step = Combo->Steps[ActiveComboStep];
    const FAttackSpecConfig* Spec = FindAttack(Step.AttackId);
    if (!Spec)
    {
        bComboAdvanceRequested = false;
        return;
    }

    FAttackSpecConfig Local = *Spec;
    if (Step.DamageOverride > 0.f)           Local.BaseDamage = Step.DamageOverride;
    if (Step.CritChanceOverride >= 0.f)      Local.CritChance = Step.CritChanceOverride;
    if (Step.CritMultiplierOverride >= 0.f)  Local.CritMultiplier = Step.CritMultiplierOverride;

    // Trigger next step by id. PlayOrJumpMontageSection guarantees no "restart" of first section.
    TryAttackById(Local.AttackId);

    if (UWorld* W = GetWorld())
    {
        ComboResetTime = W->GetTimeSeconds() + Combo->ResetDelay;
    }

    bComboAdvanceRequested = false;
    ComboBufferExpireAt = 0.0f;
}

#pragma endregion

float UCombatComponent::GetComboCooldownRemaining(FName ComboId) const
{
    if (!GetWorld()) return 0.f;
    if (const float* Until = ComboCooldownUntil.Find(ComboId))
    {
        const float Now = GetWorld()->GetTimeSeconds();
        return FMath::Max(0.f, *Until - Now);
    }
    return 0.f;
}

void UCombatComponent::ClearAllComboCooldowns()
{
    ComboCooldownUntil.Reset();
}
