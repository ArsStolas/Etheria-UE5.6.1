/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "CombatComponent - Source (Core)"
 * Notes: Lifecycle, owner references, and common runtime helpers.
 */

#include "Components/Combat/CombatComponent.h"
#include "Components/Combat/LockTarget/LockTargetComponent.h"
#include "Data/Weapons/WeaponData.h"
#include "Characters/BaseCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/Characters/CharacterStateComponent.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    ResolveOwnerRefs();

    if (OwnerCharacter.IsValid())
    {
        MoveComp = OwnerCharacter->GetCharacterMovement();
        if (MoveComp.IsValid())
        {
            BaseWalkSpeed = MoveComp->MaxWalkSpeed;
        }
    }

    if (OwnerMesh.IsValid())
    {
        BaseGlobalAnimRate = OwnerMesh->GlobalAnimRateScale;
    }

    if (AActor* O = GetOwner())
    {
        LockComp = O->FindComponentByClass<ULockTargetComponent>();
    }

    // Auto-apply weapon data if provided
    if (WeaponData)
    {
        // Keep current weapon data in sync so aiming/ranged systems can read Ranged config.
        SetCurrentWeaponData(WeaponData);
    }
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Bow charge build-up (0..1). This only runs while the player is holding "fire" for a bow.
    if (bIsCharging)
    {
        const UWeaponData* Data = GetCurrentWeaponData();
        if (Data && Data->Ranged.bIsRangedWeapon && Data->Ranged.bUseChargeOnAim && Data->Ranged.WeaponKind == EWeaponRangedType::Bow)
        {
            UpdateCharge(DeltaTime);
        }
    }
}

bool UCombatComponent::ResolveOwnerRefs()
{
    AActor* O = GetOwner();
    OwnerCharacter = Cast<ABaseCharacter>(O);

    // Prefer the character mesh if available
    if (OwnerCharacter.IsValid())
    {
        OwnerMesh = OwnerCharacter->GetMesh();
        MoveComp  = OwnerCharacter->GetCharacterMovement();
        StateComp = OwnerCharacter->GetStateComponent();
    }
    else if (O)
    {
        // Fallback: any skeletal mesh on the owner
        if (USkeletalMeshComponent* Skel = O->FindComponentByClass<USkeletalMeshComponent>())
        {
            OwnerMesh = Skel;
        }
    }

    return OwnerCharacter.IsValid() || OwnerMesh.IsValid();
}

void UCombatComponent::SetAttacks(const TArray<FAttackSpecConfig>& InAttacks) { Attacks = InAttacks; }
void UCombatComponent::SetCombos(const TArray<FComboSpecConfig>& InCombos)   { Combos = InCombos; }

void UCombatComponent::SetWeaponData(UWeaponData* InData)
{
    // For backwards compatibility, SetWeaponData also updates CurrentWeaponData.
    SetCurrentWeaponData(InData);
}

void UCombatComponent::ApplyWeaponData()
{
    UWeaponData* Data = GetCurrentWeaponData();
    if (!Data) return;

    Attacks = Data->Attacks;
    Combos  = Data->Combos;

    if (Data->DamageTraceChannelOverride != ECC_MAX)
    {
        DamageTraceChannel = Data->DamageTraceChannelOverride;
    }
    if (Data->bOverrideMagnetism)
    {
        MagnetismAngleDeg = Data->MagnetismAngleDeg;
        MagnetismStrength = Data->MagnetismStrength;
    }
}

void UCombatComponent::SetCurrentWeaponData(UWeaponData* NewWeaponData)
{
    if (CurrentWeaponData == NewWeaponData) return;

    // Stop any ongoing ranged behavior when swapping weapons.
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);
    }

    bIsCharging = false;
    CurrentChargeLevel = 0.f;

    CurrentWeaponData = NewWeaponData;
    WeaponData = NewWeaponData;

    ApplyWeaponData();
}

bool UCombatComponent::IsInCooldown() const
{
    UWorld* W = GetWorld();
    return W ? (W->GetTimeSeconds() < CooldownEndTime) : false;
}

bool UCombatComponent::IsJumpBlocked() const { return JumpLocks.Num() > 0; }
bool UCombatComponent::IsCrouchBlocked() const { return CrouchLocks.Num() > 0; }

void UCombatComponent::PushInputLock(FName LockId, bool bBlockJump, bool bBlockCrouch)
{
    if (LockId == NAME_None) return;
    if (bBlockJump)   JumpLocks.Add(LockId);
    if (bBlockCrouch) CrouchLocks.Add(LockId);
}

void UCombatComponent::PopInputLock(FName LockId)
{
    if (LockId == NAME_None) return;
    JumpLocks.Remove(LockId);
    CrouchLocks.Remove(LockId);
}

void UCombatComponent::SetExternalTarget(AActor* InTarget) { ExternalTarget = InTarget; }

AActor* UCombatComponent::GetCurrentTarget() const
{
    if (LockComp.IsValid() && LockComp->GetCurrentTarget())
    {
        return LockComp->GetCurrentTarget();
    }
    return ExternalTarget.Get();
}

void UCombatComponent::PushRecentHitActor(AActor* A)
{
    if (!A) return;
    RecentHitActors.AddUnique(A);
}

void UCombatComponent::ClearRecentHitActors()
{
    RecentHitActors.Reset();
}

void UCombatComponent::GetRecentHitActors(TArray<AActor*>& Out) const
{
    for (const TWeakObjectPtr<AActor>& W : RecentHitActors)
    {
        if (W.IsValid()) Out.Add(W.Get());
    }
}

/** Config Helper */
float UCombatComponent::GetCurrentAttackRange() const
{
    if (!CurrentWeaponData) return 0.f;

    if (CurrentWeaponData->Ranged.bIsRangedWeapon)
    {
        return CurrentWeaponData->Ranged.FullAutoRate > 0.f ?
               CurrentWeaponData->Ranged.AimArmLength : 
               200.f;
    }
    
    if (Attacks.Num() > 0)
    {
        return Attacks[0].Range;
    }
    return 0.f;
}

void UCombatComponent::RequestComboAdvanceAI()
{
    bComboAdvanceRequested = true;
}
