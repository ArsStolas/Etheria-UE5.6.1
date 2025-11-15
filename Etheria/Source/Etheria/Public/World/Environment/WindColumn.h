/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: WindColumn - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindColumn.generated.h"

class UBoxComponent;
class APlayerCharacter;

UCLASS()
class ETHERIA_API AWindColumn : public AActor
{
	GENERATED_BODY()

public:
	AWindColumn();

protected:
	virtual void BeginPlay() override;

	// === COMPONENTS ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wind")
	UBoxComponent* WindArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wind")
	UStaticMeshComponent* VisualMesh;

	// === SETTINGS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Settings", meta=(ClampMin="0.0", UIMin="0.0", UIMax="15000.0"))
	float LiftForce = 5000.f; // Force de portance appliquée verticalement

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Settings", meta=(ClampMin="0.0", UIMin="0.0", UIMax="1.0"))
	float VerticalDamping = 0.2f; // Réduit la vitesse de chute (0 = aucun effet, 1 = bloque totalement)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Settings")
	float ApplyInterval = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Settings")
	bool bAffectOnlyGliding = true;

	// === EXIT BOOST ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Boost", meta=(ClampMin="0.0", UIMin="0.0", UIMax="15000.0"))
	float ExitBoostForce = 10000.f; // Force du boost à la sortie par le haut

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Boost", meta=(ClampMin="0.0", UIMin="0.0", UIMax="300.0"))
	float ExitTopMargin = 100.f; // Tolérance (distance au sommet de la colonne pour déclencher le boost)

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
						const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
					  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyLift(APlayerCharacter* Player);

	UPROPERTY()
	TMap<APlayerCharacter*, FTimerHandle> ActiveTimers;

	TSet<APlayerCharacter*> BoostedThisStay;
};
