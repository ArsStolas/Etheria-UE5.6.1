/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "CombatComponent - Source (Telegraph)"
 * Notes: Decal-based telegraph visuals for charged attacks.
 */

#include "Components/Combat/CombatComponent.h"

#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#pragma region TELEGRAPH VISUALS

void UCombatComponent::SpawnTelegraph()
{
    DestroyTelegraph();

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    const FAttackSpecConfig* Spec = FindAttack(CurrentAttackId);
    if (!Spec || !Spec->Charge.bShowTelegraph)
    {
        return;
    }

    UMaterialInterface* Mat = nullptr;
    if (Spec->Charge.Shape == EChargeShape::Radial)
    {
        Mat = RadialDecalMaterial;
    }
    else if (Spec->Charge.Shape == EChargeShape::FrontalRect)
    {
        Mat = RectDecalMaterial;
    }

    if (!Mat)
    {
        return;
    }

    ActiveDecal = NewObject<UDecalComponent>(Owner, UDecalComponent::StaticClass(), NAME_None);
    if (!ActiveDecal)
    {
        return;
    }

    ActiveDecal->RegisterComponent();
    ActiveDecal->SetDecalMaterial(Mat);
    ActiveDecal->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

    FVector Loc = Owner->GetActorLocation();
    Loc.Z -= TelegraphHeightOffset;

    FRotator Rot = Owner->GetActorRotation();
    Rot.Pitch = -90.f; // project downward

    ActiveDecal->SetWorldLocationAndRotation(Loc, Rot);

    // X = thickness along projection direction
    ActiveDecal->DecalSize = FVector(TelegraphDecalThickness, 1.f, 1.f);

    // Very small fade screen size so the decal remains visible even from far away.
    ActiveDecal->SetFadeScreenSize(0.0001f);
}

void UCombatComponent::UpdateTelegraph(float Alpha)
{
    if (!ActiveDecal)
    {
        return;
    }

    const FAttackSpecConfig* Spec = FindAttack(CurrentAttackId);
    if (!Spec)
    {
        return;
    }

    Alpha = FMath::Clamp(Alpha, 0.f, 1.f);

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    if (Spec->Charge.Shape == EChargeShape::Radial)
    {
        const float R = FMath::Lerp(0.f, Spec->Charge.MaxRadius, Alpha);

        // X = thickness, Y/Z = radius.
        ActiveDecal->DecalSize = FVector(TelegraphDecalThickness, R, R);

        FVector Loc = Owner->GetActorLocation();
        Loc.Z -= TelegraphHeightOffset;

        FRotator Rot = Owner->GetActorRotation();
        Rot.Pitch = -90.f;

        ActiveDecal->SetWorldLocationAndRotation(Loc, Rot);
    }
    else if (Spec->Charge.Shape == EChargeShape::FrontalRect)
    {
        const float L = FMath::Lerp(0.f, Spec->Charge.MaxLength, Alpha);
        const float W = FMath::Lerp(0.f, Spec->Charge.MaxWidth, Alpha);

        // X = thickness, Y = half-length, Z = half-width.
        ActiveDecal->DecalSize = FVector(TelegraphDecalThickness, L * 0.5f, W * 0.5f);

        const FVector Fwd = Owner->GetActorForwardVector();
        FVector Loc = Owner->GetActorLocation() + Fwd * (L * 0.5f);
        Loc.Z -= TelegraphHeightOffset;

        FRotator Rot = Owner->GetActorRotation();
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
