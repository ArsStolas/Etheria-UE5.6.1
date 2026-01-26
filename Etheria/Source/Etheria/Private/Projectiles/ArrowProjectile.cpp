/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ArrowProjectile" - Source
 */

#include "Projectiles/ArrowProjectile.h"

#include "Components/Combat/CombatComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"

AArrowProjectile::AArrowProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);

    Collision->InitSphereRadius(4.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Block);
    Collision->SetNotifyRigidBodyCollision(true);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->ProjectileGravityScale = 1.f;
    ProjectileMovement->InitialSpeed = 3000.f;
    ProjectileMovement->MaxSpeed = 3000.f;

    Collision->OnComponentHit.AddDynamic(this, &AArrowProjectile::HandleHit);
}

void AArrowProjectile::BeginPlay()
{
    Super::BeginPlay();

    SetLifeSpan(LifeSeconds);
}

void AArrowProjectile::InitProjectile(UCombatComponent* InSourceCombat, FName InAttackId, float InChargeAlpha, float InDamageScale, float InSpeed, float InGravityScale)
{
    SourceCombat = InSourceCombat;
    AttackId = InAttackId;
    ChargeAlpha = InChargeAlpha;
    DamageScale = InDamageScale;

    if (ProjectileMovement)
    {
        ProjectileMovement->ProjectileGravityScale = InGravityScale;
        ProjectileMovement->InitialSpeed = InSpeed;
        ProjectileMovement->MaxSpeed = InSpeed;
        ProjectileMovement->Velocity = GetActorForwardVector() * InSpeed;
    }

    // Avoid immediately colliding with the owner/instigator.
    if (AActor* OwningActor = GetOwner())
    {
        Collision->IgnoreActorWhenMoving(OwningActor, true);
    }
    if (APawn* Inst = GetInstigator())
    {
        Collision->IgnoreActorWhenMoving(Inst, true);
    }
}

void AArrowProjectile::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == GetOwner())
    {
        if (bDestroyOnHit) Destroy();
        return;
    }

    if (SourceCombat.IsValid())
    {
        SourceCombat->HandleRangedProjectileImpact(OtherActor, Hit, AttackId, ChargeAlpha, DamageScale);
    }

    if (bStickToHitComponent && OtherComp)
    {
        AttachToComponent(OtherComp, FAttachmentTransformRules::KeepWorldTransform);
        SetActorEnableCollision(false);

        // Stop movement when stuck.
        if (ProjectileMovement)
        {
            ProjectileMovement->StopMovementImmediately();
            ProjectileMovement->SetComponentTickEnabled(false);
        }

        // Don't destroy if we want to leave it stuck in the world.
        return;
    }

    if (bDestroyOnHit)
    {
        Destroy();
    }
}
