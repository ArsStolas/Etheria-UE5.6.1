/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "CombatComponent - Source (Charge)"
 * Notes: Charge input flow and damage scaling.
 */

#include "Characters/BaseCharacter.h"
#include "Components/Combat/CombatComponent.h"

#include "Components/DecalComponent.h"
#include "Engine/World.h"

#pragma region CHARGE

void UCombatComponent::BeginCharge(FName AttackId, float ExpectedDuration)
{
    if (AttackId == NAME_None)
    {
        if (CurrentAttackId == NAME_None && Attacks.Num() > 0) AttackId = Attacks[0].AttackId;
        else AttackId = CurrentAttackId;
    }

    const FAttackSpecConfig* Spec = FindAttack(AttackId);
    if (!Spec || !Spec->Charge.bChargeable) return;

    CurrentAttackId = AttackId;
    bCharging = true;
    ChargeStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    ChargeExpectedDuration = FMath::Max(0.1f, ExpectedDuration);
    ChargeAccumulated = 0.f;
    ChargeLevelIndex = -1;
    ObservedChargeLevel = -1;

    if (Spec->Charge.bShowTelegraph)
    {
        SpawnTelegraph();
        UpdateTelegraph(0.f);
    }

    OnCue.Broadcast(FName("ChargeStart"), ECombatCuePhase::Start);
}

void UCombatComponent::UpdateChargeProgress(float DeltaTime)
{
    if (!bCharging) return;
    ChargeAccumulated += FMath::Max(0.f, DeltaTime);

    const float Alpha = FMath::Clamp(ChargeAccumulated / FMath::Max(0.001f, ChargeExpectedDuration), 0.f, 1.f);
    UpdateTelegraph(Alpha);

    const FAttackSpecConfig* Spec = FindAttack(CurrentAttackId);
    if (!Spec) return;

    int32 NewObserved = -1;
    for (int32 i = 0; i < Spec->Charge.Levels.Num(); ++i)
    {
        if (ChargeAccumulated >= Spec->Charge.Levels[i].Time) NewObserved = i;
    }

    if (NewObserved != ObservedChargeLevel)
    {
        ObservedChargeLevel = NewObserved;
        if (ObservedChargeLevel >= 0)
        {
            const FName Cue = FName(*FString::Printf(TEXT("ChargeLevel_%d"), ObservedChargeLevel + 1));
            OnCue.Broadcast(Cue, ECombatCuePhase::Start);
        }
    }
}

void UCombatComponent::EndCharge(bool bCanceled)
{
    if (!bCharging) return;
    bCharging = false;

    const FAttackSpecConfig* Spec = FindAttack(CurrentAttackId);
    if (!Spec)
    {
        DestroyTelegraph();
        return;
    }

    float Elapsed = ChargeAccumulated;
    int32 Level = -1;
    float DamageScale = 1.f;
    float RangeScale  = 1.f;

    if (Spec->Charge.Levels.Num() > 0)
    {
        for (int32 i = 0; i < Spec->Charge.Levels.Num(); ++i)
        {
            if (Elapsed >= Spec->Charge.Levels[i].Time) { Level = i; }
        }
        if (Level >= 0)
        {
            DamageScale = Spec->Charge.Levels[Level].DamageMultiplier;
            RangeScale  = Spec->Charge.Levels[Level].RangeMultiplier;
        }
    }
    ChargeLevelIndex = Level;

    DestroyTelegraph();

    if (bCanceled)
    {
        OnCue.Broadcast(FName("ChargeCancel"), ECombatCuePhase::End);
        return;
    }

    OnCue.Broadcast(FName("ChargeRelease"), ECombatCuePhase::Impact);

    // Optional: montage "Release_Lx" section jump
    if (OwnerCharacter.IsValid() && Spec->Montage && ChargeLevelIndex >= 0 && Spec->Charge.ReleaseMontages.Num() == 0)
    {
        if (UAnimInstance* Anim = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr)
        {
            const FName ReleaseSection = FName(*FString::Printf(TEXT("Release_L%d"), ChargeLevelIndex + 1));
            if (Anim->Montage_IsPlaying(Spec->Montage))
            {
                Anim->Montage_JumpToSection(ReleaseSection, Spec->Montage);
            }
            else
            {
                OwnerCharacter->PlayAnimMontage(Spec->Montage, 1.f, ReleaseSection);
            }
        }
    }
    else
    {
        ExecuteAttack(*Spec, DamageScale, RangeScale);
    }

    // If charge describes extra hit shape, fire it here (visual telegraph already destroyed).
    if (Spec->Charge.Shape == EChargeShape::Radial)
    {
        FAttackSpecConfig Copy = *Spec;
        Copy.AttackType = EAttackType::AoE;
        Copy.Radius = Spec->Charge.MaxRadius;
        PerformAoE(Copy, DamageScale, RangeScale);
    }
    else if (Spec->Charge.Shape == EChargeShape::FrontalRect)
    {
        PerformFrontalRect(*Spec, DamageScale, RangeScale);
    }
}

#pragma endregion

#pragma region TELEGRAPH VISUALS

void UCombatComponent::SpawnTelegraph()
{
    DestroyTelegraph();
    if (!GetOwner()) return;

    const FAttackSpecConfig* Spec = FindAttack(CurrentAttackId);
    if (!Spec || !Spec->Charge.bShowTelegraph) return;

    UMaterialInterface* Mat = nullptr;
    if (Spec->Charge.Shape == EChargeShape::Radial)       Mat = RadialDecalMaterial;
    else if (Spec->Charge.Shape == EChargeShape::FrontalRect) Mat = RectDecalMaterial;

    if (!Mat) return;

    ActiveDecal = NewObject<UDecalComponent>(GetOwner(), UDecalComponent::StaticClass(), NAME_None);
    if (!ActiveDecal) return;

    ActiveDecal->RegisterComponent();
    ActiveDecal->SetDecalMaterial(Mat);
    ActiveDecal->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

    FVector Loc = GetOwner()->GetActorLocation();
    FRotator Rot = GetOwner()->GetActorRotation();
    Rot.Pitch = -90.f;

    ActiveDecal->SetWorldLocationAndRotation(Loc, Rot);
    ActiveDecal->SetFadeScreenSize(0.0001f);
}

void UCombatComponent::UpdateTelegraph(float Alpha)
{
    if (!ActiveDecal) return;

    const FAttackSpecConfig* Spec = FindAttack(CurrentAttackId);
    if (!Spec) return;

    Alpha = FMath::Clamp(Alpha, 0.f, 1.f);

    if (Spec->Charge.Shape == EChargeShape::Radial)
    {
        const float R = FMath::Lerp(0.f, Spec->Charge.MaxRadius, Alpha);
        ActiveDecal->DecalSize = FVector(1.f, R, R);
    }
    else if (Spec->Charge.Shape == EChargeShape::FrontalRect)
    {
        const float L = FMath::Lerp(0.f, Spec->Charge.MaxLength, Alpha);
        const float W = FMath::Lerp(0.f, Spec->Charge.MaxWidth, Alpha);

        ActiveDecal->DecalSize = FVector(1.f, L * 0.5f, W * 0.5f);

        FVector Fwd = GetOwner()->GetActorForwardVector();
        FVector Loc = GetOwner()->GetActorLocation() + Fwd * (L * 0.5f);
        FRotator Rot = GetOwner()->GetActorRotation();
        Rot.Pitch = -90.f;

        ActiveDecal->SetWorldLocationAndRotation(Loc, Rot);
    }
}

void UCombatComponent::DestroyTelegraph()
{
    if (ActiveDecal)
    {
        ActiveDecal->DestroyComponent();
        ActiveDecal = nullptr;
    }
}

#pragma endregion
