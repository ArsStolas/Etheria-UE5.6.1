/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ItemPickup" - Header
 * Note: Pickup with visual (StaticMesh / Niagara / none)
 */

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/Inventory/InventoryTypes.h"
#include "Interfaces/Interaction.h"
#include "ItemPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class ETHERIA_API AItemPickup : public AActor, public IInteraction
{
	GENERATED_BODY()
public:
	AItemPickup();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USphereComponent* sphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStaticMeshComponent* mesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UNiagaraComponent* vfx = nullptr;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	class UItemDefinition* itemDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(ClampMin="1"))
	int32 quantity = 1;

	// ---- Visuals 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	bool bShowStaticMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual", meta=(EditCondition="bShowStaticMesh"))
	UStaticMesh* StaticMeshOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	bool bShowNiagaraVFX = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual", meta=(EditCondition="bShowNiagaraVFX"))
	UNiagaraSystem* NiagaraSystem = nullptr;

	UFUNCTION(BlueprintCallable, Category="Item")
	bool OnPickedBy(class UInventoryComponent* Inventory);

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Interact_Implementation(AActor* Interactor) override;
};
