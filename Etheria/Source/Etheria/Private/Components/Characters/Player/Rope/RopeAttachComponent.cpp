/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeAttachComponent - Source
*/

#include "Components/Characters/Player/Rope/RopeAttachComponent.h"

#include "CableComponent.h"
#include "Components/Characters/Player/Rope/RopeLockComponent.h"
#include "World/Rope/RopeAttachPoint.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"
#include "Materials/MaterialInterface.h"

URopeAttachComponent::URopeAttachComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URopeAttachComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
    if (!OwnerCharacter) return;

    LockComponent = OwnerCharacter->GetRopeLockComponent();
    if (LockComponent)
    {
        LockComponent->OnLockedPointChanged.AddDynamic(
            this,
            &URopeAttachComponent::OnLockedPointChanged
        );
    }
    
    CableComponent = OwnerCharacter->GetRopeCableComponent();
    
    if (!CableComponent) return;

    // ===== CRITICAL: SET LENGTH FIRST BEFORE SEGMENTS =====
    // Initialize with a safe default length to allocate particle array
    CableComponent->CableLength = 500.f; 
    CableComponent->EndLocation = FVector(0, 0, -500.f);
    
    // ===== CABLE PHYSICS SETUP (REALISTIC ROPE) =====
    // Hard limit to 10 segments to prevent crashes
    CableComponent->NumSegments = FMath::Clamp(NumSegments, 3, 10);
    CableComponent->SubstepTime = SubstepTime;
    CableComponent->SolverIterations = SolverIterations;
    
    CableComponent->CableWidth = RopeWidth;

    if (RopeMaterial)
    {
        CableComponent->SetMaterial(0, RopeMaterial);
    }
    
    // Make cable act like a real soft rope
    CableComponent->bEnableStiffness = false; // Stiffness makes it rigid, we want it soft
    CableComponent->CableForce = CableForce; // Damping force to reduce bouncing
    CableComponent->CableGravityScale = CableGravityScale;
    
    // Collision setup - MUST be properly configured for cable to interact with world
    CableComponent->bEnableCollision = bEnableCollision;
    
    if (bEnableCollision)
    {
        // Ensure visible width for collision
        CableComponent->CableWidth = FMath::Max(RopeWidth, 2.f);
        
        // CRITICAL: Setup collision channels properly
        CableComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        CableComponent->SetCollisionObjectType(ECC_PhysicsBody); // Cable is a physics object
        
        // Block everything by default
        CableComponent->SetCollisionResponseToAllChannels(ECR_Block);
        
        // But ignore pawns and camera to avoid interfering with player
        CableComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        CableComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        CableComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        
        ROPE_LOG(LogTemp, Log, TEXT("[RopeAttach] Cable collision ENABLED"));
    }
    else
    {
        CableComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ROPE_LOG(LogTemp, Log, TEXT("[RopeAttach] Cable collision DISABLED"));
    }
    
    CableComponent->bSkipCableUpdateWhenNotVisible = false;
    CableComponent->bSkipCableUpdateWhenNotOwnerRecentlyRendered = false;

    CableComponent->SetVisibility(false);
}

void URopeAttachComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!CableComponent || !OwnerCharacter || !AttachedPoint.IsValid()) 
    {
        ROPE_LOG(LogTemp, Warning, TEXT("[RopeAttach] Tick DISABLED (guard) — CableComponent=%s OwnerCharacter=%s AttachedPoint=%s"),
            CableComponent      ? TEXT("OK") : TEXT("NULL"),
            OwnerCharacter      ? TEXT("OK") : TEXT("NULL"),
            AttachedPoint.IsValid() ? TEXT("OK") : TEXT("NULL"));
        SetComponentTickEnabled(false);
        return;
    }

    // Get actual distance between player and anchor
    FVector PlayerLoc = OwnerCharacter->GetActorLocation();
    FVector AnchorLoc = AttachedPoint->GetActorLocation();
    float TrueDistance = FVector::Distance(PlayerLoc, AnchorLoc);

    // Smoothly interpolate visual length towards target
    float CurrentVisual = CableComponent->CableLength;
    float TargetVisual = TargetCableLength;
    
    // Allow some slack: if actual distance is larger than target, give some extra length
    // This prevents visual disconnect and makes rope feel more natural
    float SlackAmount = FMath::Max(0.f, TrueDistance - TargetCableLength);
    if (SlackAmount > 10.f) // Only add slack if significant
    {
        TargetVisual = TrueDistance + RopeLengthOffset;
    }
    
    // Interpolate smoothly
    CableComponent->CableLength = FMath::FInterpTo(
        CurrentVisual,
        TargetVisual,
        DeltaTime,
        CableLengthInterpSpeed
    );

    // Update tension-based width
    float TensionRatio = FMath::Clamp(
        (TrueDistance - MinRopeLength) / (MaxRopeLength - MinRopeLength),
        0.f, 1.f
    );

    float NewCableWidth = FMath::Lerp(MinRopeWidth, MaxRopeWidth, TensionRatio * TensionWidthMultiplier);
    CableComponent->CableWidth = NewCableWidth;
    
    ROPE_SCREEN_MSG(200, FColor::Cyan, TEXT("Cable: Target=%.0f Current=%.0f TrueDist=%.0f"), 
        TargetCableLength, CableComponent->CableLength, TrueDistance);
}

void URopeAttachComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DetachRope();
    Super::EndPlay(EndPlayReason);
}

void URopeAttachComponent::AttachRope(ARopeAttachPoint* TargetPoint)
{
    if (!TargetPoint || !CableComponent || !OwnerCharacter) return;

    AttachedPoint = TargetPoint;

    const FVector PlayerLoc = OwnerCharacter->GetActorLocation();
    const FVector AnchorLoc = TargetPoint->GetActorLocation();

    const float RopeLength = FVector::Dist(PlayerLoc, AnchorLoc);

    // Visual setup - Attach to the point's root with an offset
    TargetCableLength = RopeLength + RopeLengthOffset;
    CableComponent->CableLength = TargetCableLength;
    
    // Attach to the anchor point
    CableComponent->SetAttachEndToComponent(TargetPoint->GetRootComponent(), NAME_None);
    
    // Apply vertical offset to raise the attachment point
    CableComponent->EndLocation = FVector(0.f, 0.f, AnchorAttachmentOffset);
    
    CableComponent->SetVisibility(true);
    
    // Enable tick for smooth updates
    SetComponentTickEnabled(true);
    ROPE_LOG(LogTemp, Log, TEXT("[RopeAttach] Tick ENABLED — attached to %s | Length: %.2f"), 
        *TargetPoint->GetName(), RopeLength);
    
    // Physics setup
    OwnerCharacter->GetRopeConstraintComponent()->ActivateConstraint(
        TargetPoint,
        RopeLength
    );

    // Lock setup
    if (LockComponent)
    {
        LockComponent->SetPhysicallyAttached(true);
    }

    ROPE_LOG(LogTemp, Log, TEXT("[RopeAttach] Rope attached to %s - Length: %.2f"), 
        *TargetPoint->GetName(), RopeLength);
}

void URopeAttachComponent::DetachRope()
{
    if (LockComponent)
    {
        LockComponent->SetPhysicallyAttached(false);
    }

    AttachedPoint.Reset();

    if (CableComponent)
    {
        CableComponent->SetVisibility(false);
    }

    if (OwnerCharacter)
    {
        OwnerCharacter->GetRopeConstraintComponent()->DeactivateConstraint();
    }
    
    SetComponentTickEnabled(false);
    ROPE_LOG(LogTemp, Log, TEXT("[RopeAttach] Tick DISABLED — rope detached"));

    ROPE_LOG(LogTemp, Log, TEXT("[RopeAttach] Rope detached"));
}

void URopeAttachComponent::UpdateVisualCableLength(float NewTargetLength, float DeltaTime)
{
    if (!CableComponent || !OwnerCharacter || !AttachedPoint.IsValid()) return;

    // Update target length (Tick will interpolate towards it)
    TargetCableLength = NewTargetLength + RopeLengthOffset;
    
    // Ensure tick is enabled for interpolation
    if (!IsComponentTickEnabled())
    {
        SetComponentTickEnabled(true);
        ROPE_LOG(LogTemp, Log, TEXT("[RopeAttach] Tick ENABLED — re-armed by UpdateVisualCableLength (TargetLength: %.2f)"), TargetCableLength);
    }

    ROPE_LOG(LogTemp, VeryVerbose, TEXT("[RopeAttach] Target length set to: %.2f"), TargetCableLength);
}

void URopeAttachComponent::SetRopeMesh(USkeletalMesh* NewMesh)
{
    if (!CableComponent || !NewMesh) return;
    RopeMesh = NewMesh;
}

void URopeAttachComponent::SetRopeMaterial(UMaterialInterface* NewMaterial)
{
    if (!CableComponent || !NewMaterial) return;

    RopeMaterial = NewMaterial;
    CableComponent->SetMaterial(0, RopeMaterial);
}

float URopeAttachComponent::GetCurrentRopeLength() const
{
    if (!CableComponent) return 0.f;
    return CableComponent->CableLength - RopeLengthOffset;
}

void URopeAttachComponent::OnLockedPointChanged(ARopeAttachPoint* NewLockedPoint)
{
    if (NewLockedPoint)
    {
        AttachRope(NewLockedPoint);
    }
    else
    {
        DetachRope();
    }
}