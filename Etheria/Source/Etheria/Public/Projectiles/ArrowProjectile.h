/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ArrowProjectile" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArrowProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UCombatComponent;

UCLASS()
class ETHERIA_API AArrowProjectile : public AActor
{
    GENERATED_BODY()

public:
    AArrowProjectile();

    /** Initializes runtime params (call right after SpawnActor). */
    void InitProjectile(UCombatComponent* InSourceCombat, FName InAttackId, float InChargeAlpha, float InDamageScale, float InSpeed, float InGravityScale);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
    UPROPERTY(VisibleAnywhere, Category="Components")
    USphereComponent* Collision = nullptr;

    UPROPERTY(VisibleAnywhere, Category="Components")
    UStaticMeshComponent* Mesh = nullptr;

    UPROPERTY(VisibleAnywhere, Category="Components")
    UProjectileMovementComponent* ProjectileMovement = nullptr;

    UPROPERTY(EditAnywhere, Category="Projectile", meta=(ClampMin="0.1"))
    float LifeSeconds = 10.f;

    UPROPERTY(EditAnywhere, Category="Projectile")
    bool bDestroyOnHit = true;

    UPROPERTY(EditAnywhere, Category="Projectile")
    bool bStickToHitComponent = false;

    TWeakObjectPtr<UCombatComponent> SourceCombat;
    FName AttackId = NAME_None;
    float ChargeAlpha = 0.f;
    float DamageScale = 1.f;
};
