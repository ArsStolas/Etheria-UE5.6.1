/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeLengthControllerComponent - Source
*/

#include "Components/Characters/Player/Rope/RopeLengthControllerComponent.h"
#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"
#include "Components/Characters/Player/Rope/Swinging/RopeSwingComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/Characters/Player/Rope/Pulling/RopePullComponent.h"
#include "World/Rope/RopeAttachPoint.h"

URopeLengthControllerComponent::URopeLengthControllerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void URopeLengthControllerComponent::BeginPlay()
{
    Super::BeginPlay();
    
    SetComponentTickEnabled(false);

    OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
    if (OwnerCharacter)
    {
        AttachComponent = OwnerCharacter->GetRopeAttachComponent();
        ConstraintComponent = OwnerCharacter->GetRopeConstraintComponent();
        SwingComp = OwnerCharacter->GetRopeSwingComponent();
        PullComp = OwnerCharacter->GetRopePullComponent();
    }
}

void URopeLengthControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Si le joueur est détaché et que le tick tourne encore (input relâché tard, edge case),
    // on se désactive proprement plutôt que de tourner dans le vide
    if (!AttachComponent || !AttachComponent->IsAttached())
    {
        SetComponentTickEnabled(false);
        ROPE_LENGHT_LOG(LogTemp, Warning, TEXT("[RopeLength] Tick DISABLED (guard) — not attached, shutting down"));
        return;
    }

    ARopeAttachPoint* Point = AttachComponent->GetAttachedPoint();
    if (!Point) return;

    // IMPORTANT: ProcessRopeLength en premier pour que la longueur soit déjà
    // mise à jour avant qu'on la lise pour le visuel — évite un frame de décalage
    ProcessRopeLength(DeltaTime);

    float CurrentLength = -1.f;

    const bool bIsSwinging = SwingComp && SwingComp->IsSwinging();
    const bool bIsPull = Point->GetAttachType() == ERopeAttachType::Pull;

    if (bIsSwinging)
    {
        CurrentLength = SwingComp->GetSwingRopeLength();
    }
    else if (bIsPull)
    {
        // Utiliser CurrentPullRopeLength comme longueur visuelle et non la distance réelle joueur↔objet.
        // Quand on allonge la corde, CurrentPullRopeLength augmente immédiatement mais l'objet ne bouge pas,
        // donc la distance physique resterait identique et le câble visuel ne changerait pas.
        if (ConstraintComponent && ConstraintComponent->IsActive())
            CurrentLength = ConstraintComponent->GetCurrentPullRopeLength();
    }
    else if (ConstraintComponent && ConstraintComponent->IsActive())
    {
        CurrentLength = ConstraintComponent->GetRopeLength();
    }

    if (CurrentLength > 0.f)
    {
        AttachComponent->UpdateVisualCableLength(CurrentLength, DeltaTime);
    }
}

void URopeLengthControllerComponent::SetRopeLengthInput(float Value)
{
    RopeLengthInput = FMath::Clamp(Value, -1.f, 1.f);
    const bool bIsAdjusting = !FMath::IsNearlyZero(RopeLengthInput);

    if (IsComponentTickEnabled() != bIsAdjusting)
    {
        SetComponentTickEnabled(bIsAdjusting);
        ROPE_LENGHT_LOG(LogTemp, Log, TEXT("[RopeLength] Tick %s — input=%.2f"),
            bIsAdjusting ? TEXT("ENABLED") : TEXT("DISABLED"),
            RopeLengthInput);
    }

    if (SwingComp && SwingComp->IsSwinging())
    {
        SwingComp->SetClimbActive(bIsAdjusting);
    }

    if (PullComp)
    {
        if (AttachComponent && AttachComponent->IsAttached())
        {
            ARopeAttachPoint* Point = AttachComponent->GetAttachedPoint();
            if (Point && Point->GetAttachType() == ERopeAttachType::Pull)
                PullComp->SetPullInput(RopeLengthInput);
        }
    }
}

void URopeLengthControllerComponent::ProcessRopeLength(float DeltaTime)
{
    if (FMath::IsNearlyZero(RopeLengthInput))
        return;

    if (!AttachComponent || !AttachComponent->IsAttached())
        return;

    ARopeAttachPoint* Point = AttachComponent->GetAttachedPoint();
    if (!Point)
        return;

    const bool bIsSwinging = SwingComp && SwingComp->IsSwinging();
    const bool bIsPull = Point->GetAttachType() == ERopeAttachType::Pull;

    if (bIsPull && PullComp && PullComp->bObjectTooHeavy && RopeLengthInput > 0.f)
        return;

    float CurrentLength = 0.f;

    if (bIsSwinging)
    {
        CurrentLength = SwingComp->GetSwingRopeLength();
    }
    else if (bIsPull && ConstraintComponent)
    {
        CurrentLength = ConstraintComponent->GetCurrentPullRopeLength();
    }
    else if (ConstraintComponent && ConstraintComponent->IsActive())
    {
        CurrentLength = ConstraintComponent->GetRopeLength();
    }

    if (CurrentLength <= 0.f)
        return;

    float NewLength = CurrentLength - (RopeLengthInput * RopeAdjustSpeed * DeltaTime);

    if (bIsSwinging)
    {
        float MinLength = AttachComponent->GetMinRopeLength();
        float MaxLength = AttachComponent->GetMaxRopeLength();
        NewLength = FMath::Clamp(NewLength, MinLength, MaxLength);
        SwingComp->SetBaseRopeLength(NewLength);
    }
    else if (bIsPull && ConstraintComponent)
    {
        float MinLength = AttachComponent->GetPullMinRopeLength();
        float MaxLength = AttachComponent->GetMaxRopeLength();
        NewLength = FMath::Clamp(NewLength, MinLength, MaxLength);
        // Seul ConstraintComponent écrit CurrentPullRopeLength — PullComponent ne doit pas y toucher
        ConstraintComponent->SetCurrentPullRopeLength(NewLength);
    }
    else if (ConstraintComponent && ConstraintComponent->IsActive())
    {
        ConstraintComponent->SetRopeLength(NewLength);
    }
}