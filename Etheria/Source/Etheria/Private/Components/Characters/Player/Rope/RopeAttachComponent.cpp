/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeAttachComponent - Source
*/

#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Components/Characters/Player/Rope/RopeLockComponent.h"
#include "World/Rope/RopeAttachPoint.h"
#include "CableComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"
#include "Materials/MaterialInterface.h"

URopeAttachComponent::URopeAttachComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
}

void URopeAttachComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
    if (!OwnerCharacter) return;

    LockComponent = OwnerCharacter->FindComponentByClass<URopeLockComponent>();
    if (LockComponent)
    {
        LockComponent->OnLockedPointChanged.AddDynamic(
            this,
            &URopeAttachComponent::OnLockedPointChanged
        );
    }
    
    if (!CableComponent) return;

    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (!Mesh) return;

    CableComponent->AttachToComponent(
        Mesh,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        RopeStartSocketName
    );

    CableComponent->CableWidth = CableWidth;

    if (RopeMaterial)
    {
        CableComponent->SetMaterial(0, RopeMaterial);
    }

    CableComponent->SetVisibility(false);
    CableComponent->EndLocation = FVector::ZeroVector;
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

    // Visual setup
    CableComponent->CableLength = RopeLength + CableLengthOffset;
    CableComponent->SetAttachEndToComponent(
        TargetPoint->GetRootComponent(),
        NAME_None
    );
    CableComponent->SetVisibility(true);
    
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

    ROPE_LOG(LogTemp, Log, TEXT("[RopeAttach] Rope detached"));
}

void URopeAttachComponent::UpdateVisualCableLength(float TargetLength, float DeltaTime)
{
    if (!CableComponent) return;

    const float VisualTarget =
        TargetLength + CableLengthOffset;

    CableComponent->CableLength = FMath::FInterpTo(
        CableComponent->CableLength,
        VisualTarget,
        DeltaTime,
        CableLengthInterpSpeed
    );
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
    return CableComponent->CableLength - CableLengthOffset;
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
