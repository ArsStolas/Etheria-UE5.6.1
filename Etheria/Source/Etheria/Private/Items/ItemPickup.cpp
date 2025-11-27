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
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(false);

	sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SetRootComponent(sphere);
	sphere->InitSphereRadius(40.f);
	sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	sphere->SetCollisionObjectType(ECC_WorldDynamic);
	sphere->SetCollisionResponseToAllChannels(ECR_Overlap);

	mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	mesh->SetupAttachment(RootComponent);
	mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	vfx = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFX"));
	vfx->SetupAttachment(RootComponent);
	vfx->SetAutoActivate(false);
	vfx->SetHiddenInGame(true);
	vfx->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AItemPickup::OnConstruction(const FTransform& Transform)
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
		vfx->SetAsset(NiagaraSystem);
		vfx->SetHiddenInGame(false);
		if (!vfx->IsActive()) vfx->Activate(true);
	}
	else
	{
		if (vfx->IsActive()) vfx->Deactivate();
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
