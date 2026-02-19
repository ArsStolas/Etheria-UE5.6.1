/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeLengthControllerComponent - Source
*/

#include "Components/Characters/Player/Rope/RopeLengthControllerComponent.h"
#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"
#include "Components/Characters/Player/Rope/RopeSwingComponent.h"
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

    if (!AttachComponent || !AttachComponent->IsAttached())
        return;

    ARopeAttachPoint* Point = AttachComponent->GetAttachedPoint();
    if (!Point) return;

    float CurrentLength = -1.f;

    const bool bIsSwinging = SwingComp && SwingComp->IsSwinging();
    const bool bIsPull = Point->GetAttachType() == ERopeAttachType::Pull;

    if (bIsSwinging)
    {
        CurrentLength = SwingComp->GetSwingRopeLength();
    }
    else if (bIsPull)
    {
        UPrimitiveComponent* PullComp2 = Cast<UPrimitiveComponent>(Point->GetMeshComponent());
        if (PullComp2)
            CurrentLength = FVector::Distance(OwnerCharacter->GetActorLocation(), PullComp2->GetComponentLocation());
    }
    else if (ConstraintComponent && ConstraintComponent->IsActive())
    {
        CurrentLength = ConstraintComponent->GetRopeLength();
    }

    if (CurrentLength > 0.f)
    {
        AttachComponent->UpdateVisualCableLength(CurrentLength, DeltaTime);
    }

    ProcessRopeLength(DeltaTime);
}

void URopeLengthControllerComponent::SetRopeLengthInput(float Value)
{
    RopeLengthInput = FMath::Clamp(Value, -1.f, 1.f);
    const bool bIsAdjusting = !FMath::IsNearlyZero(RopeLengthInput);

    if (IsComponentTickEnabled() != bIsAdjusting)
    {
        SetComponentTickEnabled(bIsAdjusting);
        ROPE_LENGHT_LOG(LogTemp, Verbose,
            TEXT("[RopeLength] Tick %s"),
            bIsAdjusting ? TEXT("ENABLED") : TEXT("DISABLED"));
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
            else
                PullComp->SetPullInput(0.f);
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
        ConstraintComponent->SetCurrentPullRopeLength(NewLength);
    }
    else if (ConstraintComponent && ConstraintComponent->IsActive())
    {
        ConstraintComponent->SetRopeLength(NewLength);
    }
}
