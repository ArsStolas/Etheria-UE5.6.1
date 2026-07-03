/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ItemPickup" - Source
 * Note: Pickup with visuals (StaticMesh / Niagara / None)
 */
#include "Items/ItemPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/Items/ItemDefinition.h"
#include "Components/Inventory/InventoryComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

AItemPickup::AItemPickup()
{
	// Tick is enabled so the mesh can spin and bob.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetReplicates(false);

	sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SetRootComponent(sphere);
	sphere->InitSphereRadius(40.f);
	sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	sphere->SetCollisionObjectType(ECC_WorldDynamic);

	// Default: ignore everything, then opt-in:
	//  - BLOCK Visibility so InteractionComponent's line trace can detect the pickup.
	//  - OVERLAP Pawn in case you also want to react to pawn proximity later.
	sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	sphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	sphere->SetCollisionResponseToChannel(ECC_Pawn,       ECR_Overlap);

	mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	mesh->SetupAttachment(RootComponent);
	mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	vfx = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFX"));
	vfx->SetupAttachment(RootComponent);
	vfx->SetAutoActivate(false);
	vfx->SetHiddenInGame(true);
	vfx->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AItemPickup::BeginPlay()
{
	Super::BeginPlay();

	// Cache the mesh's starting relative location so the bobbing oscillates around it
	// instead of slowly drifting (this also respects any offset set in the editor).
	if (mesh)
	{
		MeshBaseRelativeLocation = mesh->GetRelativeLocation();
	}

	// Re-apply visuals at runtime: Niagara won't reliably activate when triggered
	// from OnConstruction during world initialization, so we ensure it here.
	ApplyVisuals();
}

void AItemPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisuals();
}

void AItemPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bAnimateMesh || !mesh) return;

	AnimationTime += DeltaSeconds;

	// Spin around local Z (yaw). Using AddLocalRotation keeps the increment frame-rate independent
	// and respects any base rotation set in the editor.
	if (!FMath::IsNearlyZero(RotationSpeed))
	{
		mesh->AddLocalRotation(FRotator(0.f, RotationSpeed * DeltaSeconds, 0.f));
	}

	// Vertical bobbing around the cached base location.
	if (FloatAmplitude > KINDA_SMALL_NUMBER && FloatSpeed > KINDA_SMALL_NUMBER)
	{
		const float Offset = FMath::Sin(AnimationTime * FloatSpeed * 2.f * PI) * FloatAmplitude;
		FVector NewLoc = MeshBaseRelativeLocation;
		NewLoc.Z += Offset;
		mesh->SetRelativeLocation(NewLoc);
	}
}

void AItemPickup::ApplyVisuals()
{
	// Static Mesh
	if (bShowStaticMesh)
	{
		UStaticMesh* M = StaticMeshOverride;
		if (!M && itemDef) { M = itemDef->worldMesh; }
		mesh->SetStaticMesh(M);
		mesh->SetHiddenInGame(M == nullptr);
	}
	else
	{
		mesh->SetStaticMesh(nullptr);
		mesh->SetHiddenInGame(true);
	}

	// Niagara VFX
	if (bShowNiagaraVFX && NiagaraSystem)
	{
		// Only re-set the asset if it actually changed (avoid restarting the system needlessly).
		if (vfx->GetAsset() != NiagaraSystem)
		{
			vfx->SetAsset(NiagaraSystem);
		}
		vfx->SetVisibility(true, true);
		vfx->SetHiddenInGame(false);

		// Force a clean (re)activation. Reset=true ensures particles spawn even
		// if the component thinks it's already active from a previous pass.
		vfx->Activate(true);
	}
	else
	{
		if (vfx->IsActive())
		{
			vfx->Deactivate();
		}
		vfx->SetVisibility(false, true);
		vfx->SetHiddenInGame(true);
	}
}

bool AItemPickup::OnPickedBy(UInventoryComponent* Inventory)
{
	if (!Inventory || !itemDef) return false;
	if (Inventory->TryAddItem(itemDef, quantity))
	{
		Destroy();
		return true;
	}
	return false;
}