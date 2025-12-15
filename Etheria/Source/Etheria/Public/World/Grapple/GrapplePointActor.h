/**
* Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GrapplePointActor - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrapplePointActor.generated.h"

UENUM(BlueprintType)
enum class EGrapplePointType : uint8
{
	Swing   UMETA(DisplayName = "Swing Point"),
	Pull    UMETA(DisplayName = "Pull Object")
};

UCLASS()
class ETHERIA_API AGrapplePointActor : public AActor
{
	GENERATED_BODY()

public:
	AGrapplePointActor();

	/* ===== Grapple Settings ===== */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grapple")
	EGrapplePointType GrappleMode = EGrapplePointType::Swing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grapple")
	float MaxAttachDistance = 2000.f;

	/* ===== Components ===== */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USceneComponent* AttachPoint;

protected:
	virtual void BeginPlay() override;
};
