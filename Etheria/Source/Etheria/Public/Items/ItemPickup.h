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

	virtual void BeginPlay() override;

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

	// ---- Idle animation (mesh only)
	/** Master toggle for the spinning + floating idle animation applied to the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual|Animation")
	bool bAnimateMesh = true;

	/** Yaw rotation speed in degrees per second. Negative values rotate the other way. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual|Animation", meta=(EditCondition="bAnimateMesh"))
	float RotationSpeed = 60.f;

	/** Vertical bobbing amplitude in world units (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual|Animation", meta=(EditCondition="bAnimateMesh", ClampMin="0.0"))
	float FloatAmplitude = 5.f;

	/** Vertical bobbing frequency (full cycles per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual|Animation", meta=(EditCondition="bAnimateMesh", ClampMin="0.0"))
	float FloatSpeed = 1.5f;

	UFUNCTION(BlueprintCallable, Category="Item")
	bool OnPickedBy(class UInventoryComponent* Inventory);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void Interact_Implementation(AActor* Interactor) override;

private:
	/** Applies static mesh and Niagara VFX based on current properties. Called in editor (OnConstruction) and at runtime (BeginPlay). */
	void ApplyVisuals();

	/** Mesh relative location captured at BeginPlay; bobbing oscillates around this. */
	FVector MeshBaseRelativeLocation = FVector::ZeroVector;

	/** Accumulated time used to drive the bobbing/rotation animation. */
	float AnimationTime = 0.f;
};