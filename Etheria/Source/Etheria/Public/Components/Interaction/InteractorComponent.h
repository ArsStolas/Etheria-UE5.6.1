/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "InteractorComponent" - Header
 * Note : Overlap cone driven by the character mesh or velocity (nearest pickup wins)
 */
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractorComponent.generated.h"

UCLASS(ClassGroup=(Player), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UInteractorComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UInteractorComponent();

	// Search range forward from the origin
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact")
	float MaxDistance = 250.f;

	// Sphere radius around the mid-point of MaxDistance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact")
	float InteractionRadius = 100.f;

	// If true, use velocity direction when speed is above MinVelocityForVelocityDir
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact")
	bool bUseVelocityDirection = true;

	// Minimum speed (cm/s) to consider velocity as facing direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact", meta=(EditCondition="bUseVelocityDirection"))
	float MinVelocityForVelocityDir = 5.f;

	// Optional mesh socket used as origin; if None, the mesh component location is used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact")
	FName MeshSocketName = NAME_None;

	// Local offset from the chosen origin (applied in mesh/component space if available)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact")
	FVector OriginLocalOffset = FVector(0.f, 0.f, 60.f);

	// Require line of sight from origin to the pickup
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact")
	bool bRequireLineOfSight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact")
	TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

	// Debug draw helpers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interact|Debug")
	bool bDrawDebug = false;

	UFUNCTION(BlueprintCallable, Category="Interact")
	void TryInteract();
};
