/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "CombatComponent - Source (Montages)"
 * Notes: Montage play/stop and notify bindings.
 */

#include "Components/Combat/CombatComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Characters/BaseCharacter.h"

#pragma region MONTAGES

void UCombatComponent::PrePlayMontageSafety(const FAttackSpecConfig& Spec)
{
    if (!bForceFallbackAnimBPForMontages) return;
    if (!OwnerCharacter.IsValid() || !Spec.Montage) return;

    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (!Mesh) return;

    UAnimInstance* Anim = Mesh->GetAnimInstance();

    // Swap to fallback AnimBP that contains the Slot, if needed
    if (FallbackMontageAnimClass && (!Anim || !Anim->IsA(FallbackMontageAnimClass)))
    {
        SavedAnimClass = Mesh->GetAnimClass();
        Mesh->SetAnimInstanceClass(FallbackMontageAnimClass);
        bUsingFallbackAnimClass = true;

        Anim = Mesh->GetAnimInstance();
    }

    if (Anim)
    {
        Anim->OnMontageEnded.RemoveDynamic(this, &UCombatComponent::HandleMontageEnded_RestoreAnimClass);
        Anim->OnMontageEnded.AddDynamic(this, &UCombatComponent::HandleMontageEnded_RestoreAnimClass);
    }
}

void UCombatComponent::HandleMontageEnded_RestoreAnimClass(UAnimMontage* Montage, bool bInterrupted)
{
    if (!OwnerCharacter.IsValid()) return;

    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (!Mesh) return;

    if (bUsingFallbackAnimClass && SavedAnimClass)
    {
        Mesh->SetAnimInstanceClass(SavedAnimClass);
    }

    bUsingFallbackAnimClass = false;
    SavedAnimClass = nullptr;
}

void UCombatComponent::PlayOrJumpMontageSection(const FAttackSpecConfig& Spec)
{
    if (!OwnerCharacter.IsValid() || !Spec.Montage) return;

    // Ensure a valid Slot by swapping to fallback if needed
    PrePlayMontageSafety(Spec);

    UAnimInstance* AnimInst = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInst) return;

    if (AnimInst->Montage_IsPlaying(Spec.Montage))
    {
        if (Spec.MontageSection != NAME_None)
        {
            AnimInst->Montage_JumpToSection(Spec.MontageSection, Spec.Montage);
            return;
        }
    }

    OwnerCharacter->PlayAnimMontage(Spec.Montage, 1.f, Spec.MontageSection);
}

#pragma endregion
