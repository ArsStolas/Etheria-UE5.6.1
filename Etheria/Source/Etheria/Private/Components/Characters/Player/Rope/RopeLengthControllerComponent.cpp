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
    }
}

void URopeLengthControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    float CurrentLength = -1.f;

    if (SwingComp && SwingComp->IsSwinging())
    {
        CurrentLength = SwingComp->GetSwingRopeLength();
    }
    else if (ConstraintComponent && ConstraintComponent->IsActive())
    {
        CurrentLength = ConstraintComponent->GetRopeLength();
    }

    if (CurrentLength > 0.f && AttachComponent)
    {
        AttachComponent->UpdateVisualCableLength(CurrentLength, DeltaTime);
    }
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("[RopeClimb] Current Rope Length: %.2f"), CurrentLength);
    
    ProcessClimbing(DeltaTime);
}

void URopeLengthControllerComponent::SetClimbInput(float Value)
{
    ClimbInput = FMath::Clamp(Value, -1.f, 1.f);
    const bool bIsClimbing = !FMath::IsNearlyZero(ClimbInput);
    
    if (IsComponentTickEnabled() != bIsClimbing)
    {
        SetComponentTickEnabled(bIsClimbing);
        CLIMB_LOG(LogTemp, Verbose, TEXT("[RopeClimb] Tick %s"), bIsClimbing ? TEXT("ENABLED") : TEXT("DISABLED"));
    }
    
    if (SwingComp && SwingComp->IsSwinging())
    {
        SwingComp->SetClimbActive(bIsClimbing);
    }
}

void URopeLengthControllerComponent::ProcessClimbing(float DeltaTime)
{
    if (FMath::IsNearlyZero(ClimbInput)) return;

    if (!AttachComponent || !AttachComponent->IsAttached())
    {
        return;
    }

    const bool bIsSwinging = SwingComp && SwingComp->IsSwinging();
    float CurrentLength = 0.f;

    if (bIsSwinging) 
    {
        CurrentLength = SwingComp->GetSwingRopeLength();
    } 
    else if (ConstraintComponent && ConstraintComponent->IsActive()) 
    {
        CurrentLength = ConstraintComponent->GetRopeLength();
    } 
    else 
    {
        return; 
    }

    // Compute new length
    float NewLength = CurrentLength - (ClimbInput * ClimbSpeed * DeltaTime);
    
    // Clamp to min/max
    if (AttachComponent)
    {
        NewLength = FMath::Clamp(
            NewLength,
            AttachComponent->GetMinRopeLength(),
            AttachComponent->GetMaxRopeLength()
        );
    }
    
    // Apply to physics systems
    if (bIsSwinging) 
    {
        SwingComp->SetBaseRopeLength(NewLength);
    } 
    else if (ConstraintComponent)
    {
        // SetRopeLength now handles visual sync internally
        ConstraintComponent->SetRopeLength(NewLength);
    }

    FColor DebugColor = bIsSwinging ? FColor::Cyan : FColor::Green;
    CLIMB_SCREEN_MSG(104, DebugColor, TEXT("Climb: %.2f (Mode: %s)"), 
        NewLength, bIsSwinging ? TEXT("SWING") : TEXT("STATIC"));
}
