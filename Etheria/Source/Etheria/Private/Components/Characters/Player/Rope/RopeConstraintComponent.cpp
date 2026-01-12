/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeConstraintComponent - Source
*/

#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"
#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/Rope/RopeAttachPoint.h"

URopeConstraintComponent::URopeConstraintComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics; // Execute before physics
}

void URopeConstraintComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
    if (!OwnerCharacter) return;

    MoveComp = OwnerCharacter->GetCharacterMovement();
    AttachComponent = OwnerCharacter->GetRopeAttachComponent();
}

void URopeConstraintComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsActive || !Anchor.IsValid()) return;

    ApplyConstraint(DeltaTime);
}

void URopeConstraintComponent::ActivateConstraint(ARopeAttachPoint* InAnchor, float InRopeLength)
{
    if (!InAnchor)
    {
       CONSTRAINT_LOG(LogTemp, Error, TEXT("[RopeConstraint] Activation failed - Anchor is null"));
       return;
    }

    if (InRopeLength <= 0.f)
    {
       CONSTRAINT_LOG(LogTemp, Error, TEXT("[RopeConstraint] Activation failed - Invalid rope length (%.2f)"), InRopeLength);
       return;
    }

    Anchor = InAnchor;
    RopeLength = InRopeLength;
    bIsActive = true;

    PrimaryComponentTick.SetTickFunctionEnable(true);
    
    // Sync visual cable length immediately
    if (AttachComponent)
    {
        AttachComponent->UpdateVisualCableLength(RopeLength, 0.f);
    }
    
    CONSTRAINT_LOG(LogTemp, Log, TEXT("[RopeConstraint] Activated on %s with length: %.2f"), 
       *InAnchor->GetName(), RopeLength);
}

void URopeConstraintComponent::DeactivateConstraint()
{
    CONSTRAINT_LOG(LogTemp, Log, TEXT("[RopeConstraint] Deactivated"));
    
    bIsActive = false;
    Anchor.Reset();
    RopeLength = 0.f;
    PrimaryComponentTick.SetTickFunctionEnable(false);
}

void URopeConstraintComponent::SetRopeLength(float NewLength)
{
    float OldLength = RopeLength;
    RopeLength = FMath::Max(NewLength, 50.f);
    
    // Immediately sync visual with new physics length
    if (AttachComponent)
    {
        AttachComponent->UpdateVisualCableLength(RopeLength, 0.f);
    }
    
    CONSTRAINT_LOG(LogTemp, Verbose, TEXT("[RopeConstraint] Length changed: %.2f -> %.2f"), OldLength, RopeLength);
}

void URopeConstraintComponent::ApplyConstraint(float DeltaTime)
{
    const FVector AnchorLoc = Anchor->GetActorLocation();
    const FVector PlayerLoc = OwnerCharacter->GetActorLocation();

    FVector ToPlayer = PlayerLoc - AnchorLoc;
    const float CurrentDist = ToPlayer.Size();

    // No constraint needed if within rope length
    if (CurrentDist <= RopeLength || CurrentDist <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector RopeDir = ToPlayer / CurrentDist;

    // Broadcast tension event (triggers swing start, etc.)
    OnRopeTensioned.Broadcast();

    // Smooth constraint application to prevent harsh snapping
    const FVector TargetLoc = AnchorLoc + RopeDir * RopeLength;
    const FVector SmoothedLoc = FMath::Lerp(PlayerLoc, TargetLoc, ConstraintSmoothness);
    
    OwnerCharacter->SetActorLocation(SmoothedLoc, true);

    // Remove radial velocity component (prevents stretching)
    if (MoveComp)
    {
       FVector Vel = MoveComp->Velocity;
       float RadialSpeed = FVector::DotProduct(Vel, RopeDir);
       
       // Only remove outward velocity
       if (RadialSpeed > 0.f)
       {
           Vel -= RopeDir * RadialSpeed;
           MoveComp->Velocity = Vel;
       }
    }
    
    CONSTRAINT_LOG(LogTemp, VeryVerbose, TEXT("[RopeConstraint] Applied - Distance: %.2f | Rope Length: %.2f"), 
        CurrentDist, RopeLength);
}
