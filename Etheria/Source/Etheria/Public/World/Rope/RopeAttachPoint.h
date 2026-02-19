/**
* Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeAttachPoint - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RopeAttachPoint.generated.h"

UENUM(BlueprintType)
enum class ERopeAttachType : uint8
{
	Swing UMETA(DisplayName = "Swing"),
	Pull  UMETA(DisplayName = "Pull")
};

UCLASS()
class ETHERIA_API ARopeAttachPoint : public AActor
{
	GENERATED_BODY()

public:
	ARopeAttachPoint();

	// === GLOBAL CACHE ===
	static TArray<TWeakObjectPtr<ARopeAttachPoint>> AllAttachPoints;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// === DATA ===
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rope")
	ERopeAttachType AttachType = ERopeAttachType::Swing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rope")
	float DetectionRadius = 150.f;

	// Debug permanent du point
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bDrawBaseDebug = true;

	FORCEINLINE ERopeAttachType GetAttachType() const { return AttachType; }
	FORCEINLINE UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

protected:
	/** Unique mesh for both Pull & Swing */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rope|Visual")
	UStaticMeshComponent* MeshComponent;
};
