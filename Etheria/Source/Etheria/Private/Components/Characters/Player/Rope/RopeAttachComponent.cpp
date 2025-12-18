#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Components/Characters/Player/Rope/RopeLockComponent.h"
#include "World/Rope/RopeAttachPoint.h"
#include "CableComponent.h"
#include "Characters/Players/PlayerCharacter.h"
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
}

void URopeAttachComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DetachRope();
    Super::EndPlay(EndPlayReason);
}

void URopeAttachComponent::AttachRope(ARopeAttachPoint* TargetPoint)
{
    if (!TargetPoint || !CableComponent) return;

    AttachedPoint = TargetPoint;
    CableComponent->SetVisibility(true);

    // Attache au joueur
    CableComponent->SetAttachEndToComponent(
        TargetPoint->GetRootComponent(),
        NAME_None
    );
    
    LockComponent->SetPhysicallyAttached(true);
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
        CableComponent->EndLocation = FVector::ZeroVector;
    }
}

void URopeAttachComponent::UpdateRope()
{
    if (!AttachedPoint.IsValid() || !CableComponent) return;

    CableComponent->SetWorldLocation(OwnerCharacter->GetActorLocation());
    CableComponent->EndLocation = AttachedPoint->GetActorLocation() - OwnerCharacter->GetActorLocation();
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

void URopeAttachComponent::SetRopeMesh(USkeletalMesh* NewMesh)
{
    if (!CableComponent || !NewMesh) return;

    RopeMesh = NewMesh;
    // CableComponent ne gère pas directement un mesh, pour ça il faudrait un SkeletalMeshComponent
    // Tu peux soit remplacer CableComponent par SkeletalMesh + physics, ou juste garder CableComponent
}

void URopeAttachComponent::SetRopeMaterial(UMaterialInterface* NewMaterial)
{
    if (!CableComponent || !NewMaterial) return;

    RopeMaterial = NewMaterial;
    CableComponent->SetMaterial(0, RopeMaterial);
}
