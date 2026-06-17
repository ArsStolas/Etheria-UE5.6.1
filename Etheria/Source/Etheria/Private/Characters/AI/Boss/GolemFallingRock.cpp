/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemFallingRock - Source"
 */

#include "Characters/AI/Boss/GolemFallingRock.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AGolemFallingRock::AGolemFallingRock()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // cosmetic only — the Golem's AoE deals the damage

	SetActorTickEnabled(false);
}

void AGolemFallingRock::Configure(UStaticMesh* Mesh, float Scale, float SpinSpeed)
{
	if (Mesh) MeshComp->SetStaticMesh(Mesh);
	SetActorScale3D(FVector(FMath::Max(Scale, 0.01f)));

	// Steady tumble around a fixed random axis (dominant yaw) so it reads as weight/inertia, not chaos.
	const float S = FMath::Max(SpinSpeed, 0.f);
	const float Sign = FMath::RandBool() ? 1.f : -1.f;
	SpinRate = FRotator(S * FMath::FRandRange(-0.7f, 0.7f), S * Sign * FMath::FRandRange(0.5f, 1.f), S * FMath::FRandRange(-0.7f, 0.7f));

	bFlying = false;
	SetActorTickEnabled(true); // spin even while held in the hand
}

void AGolemFallingRock::Launch(FVector Start, FVector End, float InDuration, float ArcHeight)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); // release from the hand if it was held

	StartLoc = Start;
	EndLoc = End;
	Duration = FMath::Max(InDuration, 0.05f);
	Elapsed = 0.f;
	Arc = ArcHeight;
	bFlying = true;

	SetActorLocation(Start);
	SetActorTickEnabled(true);
}

void AGolemFallingRock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AddActorLocalRotation(SpinRate * DeltaTime); // spin while held AND in flight

	if (!bFlying) return;

	Elapsed += DeltaTime;
	const float Alpha = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);

	FVector Loc = FMath::Lerp(StartLoc, EndLoc, Alpha);
	Loc.Z += Arc * FMath::Sin(Alpha * PI); // parabolic lift; Arc = 0 gives a straight fall
	SetActorLocation(Loc);

	if (Alpha >= 1.f) Destroy();
}
