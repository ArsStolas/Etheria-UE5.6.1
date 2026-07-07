/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemFallingRock - Header"
 * Notes: Cosmetic projectile for the Golem — flies a configurable static mesh from a start point to a
 *        target over a fixed time (arc for thrown rocks, straight for falling boulders), spins, then
 *        self-destructs on arrival. Damage stays authoritative in UGolemBossComponent's AoE; this is
 *        purely the visible rock so designers can assign a mesh instead of wiring it in Blueprint.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GolemFallingRock.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UDecalComponent;

UCLASS()
class ETHERIA_API AGolemFallingRock : public AActor
{
	GENERATED_BODY()

public:
	AGolemFallingRock();

	/** Set the visible mesh + scale and start a steady tumble (SpinSpeed deg/s) for weight/inertia — even while held. */
	UFUNCTION(BlueprintCallable, Category = "Golem")
	void Configure(UStaticMesh* Mesh, float Scale, float SpinSpeed);

	/** Detach (if held) and fly from Start to End over Duration, lifted by ArcHeight at mid-flight, then self-destruct. */
	UFUNCTION(BlueprintCallable, Category = "Golem")
	void Launch(FVector Start, FVector End, float Duration, float ArcHeight);

	/** Project a ground decal at the impact point (End) that GROWS from small to full Radius as the rock approaches. */
	UFUNCTION(BlueprintCallable, Category = "Golem")
	void SetImpactDecal(UMaterialInterface* Material, float Radius, float ProjectionDepth);

protected:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Golem") TObjectPtr<UStaticMeshComponent> MeshComp;

private:
	FVector StartLoc = FVector::ZeroVector;
	FVector EndLoc = FVector::ZeroVector;
	float Duration = 1.f;
	float Elapsed = 0.f;
	float Arc = 0.f;
	bool bFlying = false; // false = held (spin only), true = in flight (spin + travel)
	FRotator SpinRate = FRotator::ZeroRotator;

	UPROPERTY() TObjectPtr<UDecalComponent> ImpactDecal; // ground warning at End, grown as the rock approaches
	float DecalFullRadius = 0.f;
	float DecalDepth = 400.f;
};
