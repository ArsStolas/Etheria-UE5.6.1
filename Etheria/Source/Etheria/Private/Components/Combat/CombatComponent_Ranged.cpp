/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "CombatComponent - Source (Ranged)"
 * Notes: Ranged aiming state and fire helpers.
 */

#include "Characters/BaseCharacter.h"
#include "Components/Combat/CombatComponent.h"
#include "Data/Weapons/WeaponData.h"

#include "Engine/World.h"

#pragma region RANGED COMBAT

void UCombatComponent::GetAimCameraParams(float& OutArmLength, float& OutFOV) const
{
    // Local overrides (editor-friendly) have priority.
    if (bUseCameraOverrides)
    {
        OutArmLength = AimArmLengthOverride;
        OutFOV = AimFOVOverride;
        return;
    }

    // Default: read from current weapon config if available.
    const UWeaponData* Data = GetCurrentWeaponData();
    if (Data && Data->Ranged.bIsRangedWeapon)
    {
        OutArmLength = Data->Ranged.AimArmLength;
        OutFOV = Data->Ranged.AimFOV;
        return;
    }

    // Fallback:
    OutArmLength = AimArmLengthOverride;
    OutFOV = AimFOVOverride;
}

void UCombatComponent::StartRangedFire()
{
    if (!GetWorld()) return;
    if (!OwnerCharacter.IsValid()) ResolveOwnerRefs();

    UWeaponData* Data = GetCurrentWeaponData();
    if (!Data) return;

    const FWeaponRangedConfig& Ranged = Data->Ranged;
    if (!Ranged.bIsRangedWeapon) return;

    // Bow: hold to charge, release to fire
    if (Ranged.bUseChargeOnAim && Ranged.WeaponKind == EWeaponRangedType::Bow)
    {
        bIsCharging = true;
        CurrentChargeLevel = 0.f;
        UE_LOG(LogTemp, Verbose, TEXT("[Combat] Bow charge started"));
        return;
    }

    // Semi / Full auto fire
    PerformRangedFire();

    if (Ranged.WeaponKind == EWeaponRangedType::FullAuto)
    {
        const float Interval = 1.f / FMath::Max(0.01f, Ranged.FullAutoRate);
        GetWorld()->GetTimerManager().SetTimer(AutoFireHandle, this, &UCombatComponent::PerformRangedFire, Interval, true);
    }
}

void UCombatComponent::StopRangedFire()
{
    if (!GetWorld()) return;

    UWeaponData* Data = GetCurrentWeaponData();
    if (!Data) return;

    const FWeaponRangedConfig& Ranged = Data->Ranged;
    if (!Ranged.bIsRangedWeapon) return;

    // Stop auto fire
    GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);

    // Bow: release shot
    if (Ranged.bUseChargeOnAim && Ranged.WeaponKind == EWeaponRangedType::Bow && bIsCharging)
    {
        bIsCharging = false;
        PerformRangedFire();
        UE_LOG(LogTemp, Verbose, TEXT("[Combat] Bow shot released (Charge=%.2f)"), CurrentChargeLevel);
    }
}

void UCombatComponent::UpdateCharge(float DeltaTime)
{
    // 1 second to full charge (simple version). You can make this data-driven later.
    CurrentChargeLevel = FMath::Clamp(CurrentChargeLevel + (DeltaTime / 1.0f), 0.f, 1.f);
}

void UCombatComponent::PerformRangedFire()
{
    if (!OwnerCharacter.IsValid() || !CurrentWeaponData) return;

    const FWeaponRangedConfig& Ranged = CurrentWeaponData->Ranged;
    if (!Ranged.bIsRangedWeapon) return;

    UE_LOG(LogTemp, Log, TEXT("[Combat] Fire from %s | WeaponKind: %d | AimAttackId: %s"),
        *OwnerCharacter->GetName(),
        (int)Ranged.WeaponKind,
        *Ranged.AimAttackId.ToString());

    // NOTE:
    // - For bows, ChargeLevel is currently only logged + stored in CurrentChargeLevel.
    // - If you want the attack to scale damage/range, wire ChargeLevel into your AttackSpec (Charge levels)
    //   or add a "ChargeDamageMultiplier" in WeaponData and apply it in TryAttackById / ExecuteAttack.
    TryAttackById(Ranged.AimAttackId);
}

#pragma endregion
