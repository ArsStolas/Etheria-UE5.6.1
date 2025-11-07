/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "InteractorComponent" - Source
 * Note: Overlap cone driven by the character mesh or velocity (nearest pickup wins)
 */

#include "Components/Interaction/InteractorComponent.h"
#include "Components/Inventory/InventoryComponent.h"
#include "Items/ItemPickup.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UInteractorComponent::UInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractorComponent::TryInteract()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	UInventoryComponent* Inv = Owner->FindComponentByClass<UInventoryComponent>();
	if (!Inv) return;

	// Compute origin and forward from mesh and/or velocity
	FVector Origin = Owner->GetActorLocation();
	FVector Forward = Owner->GetActorForwardVector();

	const ACharacter* Char = Cast<ACharacter>(Owner);
	const USkeletalMeshComponent* Mesh = Char ? Char->GetMesh() : nullptr;

	if (Mesh)
	{
		// Mesh-based origin
		Origin = MeshSocketName != NAME_None ? Mesh->GetSocketLocation(MeshSocketName) : Mesh->GetComponentLocation();

		// Mesh-based forward by default
		Forward = Mesh->GetForwardVector();

		// Optional velocity direction if moving
		if (bUseVelocityDirection)
		{
			const FVector Vel = Char->GetVelocity();
			if (Vel.SizeSquared() > FMath::Square(MinVelocityForVelocityDir))
			{
				Forward = Vel.GetSafeNormal();
			}
		}

		// Apply local-space offset relative to the mesh/component
		Origin += Mesh->GetComponentTransform().TransformVector(OriginLocalOffset);
	}
	else
	{
		// Fallback to actor transform if no mesh is present
		if (bUseVelocityDirection)
		{
			const FVector Vel = Owner->GetVelocity();
			if (Vel.SizeSquared() > FMath::Square(MinVelocityForVelocityDir))
			{
				Forward = Vel.GetSafeNormal();
			}
		}
		Origin += Owner->GetTransform().TransformVector(OriginLocalOffset);
	}

	// ---- Build the overlap center in front of the mesh/velocity direction 
	const float MidDist = FMath::Clamp(MaxDistance * 0.5f, 0.f, MaxDistance);
	const FVector Center = Origin + Forward * MidDist;

	// Overlap pickups (WorldDynamic) within a sphere around Center
	TArray<AActor*> OutActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
	ObjTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

	UKismetSystemLibrary::SphereOverlapActors(
		this, Center, InteractionRadius,
		ObjTypes, AItemPickup::StaticClass(), { Owner }, OutActors
	);

	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), Center, InteractionRadius, 16, FColor::Green, false, 1.0f, 0, 1.0f);
		DrawDebugDirectionalArrow(GetWorld(), Origin, Origin + Forward * MidDist, 25.f, FColor::Cyan, false, 1.0f, 0, 1.5f);
	}

	// ---- Pick the nearest pickup to the player (not to the center)
	AItemPickup* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	const FVector PlayerLoc = Owner->GetActorLocation();

	for (AActor* A : OutActors)
	{
		AItemPickup* P = Cast<AItemPickup>(A);
		if (!P) continue;

		// Optional LOS from origin
		if (bRequireLineOfSight)
		{
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractLOS), false, Owner);
			if (GetWorld()->LineTraceSingleByChannel(Hit, Origin, P->GetActorLocation(), LineOfSightChannel, Params))
			{
				if (Hit.GetActor() != P) continue; // Something blocks LOS
			}
			if (bDrawDebug)
			{
				DrawDebugLine(GetWorld(), Origin, P->GetActorLocation(), FColor::Yellow, false, 0.2f, 0, 0.5f);
			}
		}

		const float DistSq = FVector::DistSquared(PlayerLoc, P->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = P;
		}
	}

	if (Best)
	{
		Best->OnPickedBy(Inv);
	}
}
