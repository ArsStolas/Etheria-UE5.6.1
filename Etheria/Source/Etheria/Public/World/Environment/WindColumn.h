/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: ArsStolas
 * Class: WindColumn - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WindColumn.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class APlayerCharacter;

UCLASS()
class ETHERIA_API AWindColumn : public AActor
{
	GENERATED_BODY()

public:
	AWindColumn();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// === COMPONENTS ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wind")
	UBoxComponent* WindArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wind|Visual")
	UNiagaraComponent* ColumnNiagaraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Wind|Visual")
	UNiagaraComponent* ExitRingNiagaraComponent;

	// === SETTINGS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Settings", meta=(ClampMin="0.0", UIMin="0.0", UIMax="15000.0"))
	float LiftForce = 5000.f; // Vertical lift force applied while the player stays in the column.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Settings", meta=(ClampMin="0.0", UIMin="0.0", UIMax="1.0"))
	float VerticalDamping = 0.2f; // Reduces falling speed before applying lift; 0 = none, 1 = cancels downward velocity.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Settings", meta=(ClampMin="0.001", UIMin="0.001", UIMax="0.2"))
	float ApplyInterval = 0.02f; // Time between lift updates while a player overlaps the column.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Settings")
	bool bAffectOnlyGliding = true; // If true, only gliding/diving players are lifted.

	// === EXIT ===
	// Small upward pop (cm/s) given at the top so the player rises a few metres above the column. ~800 ≈ +3m. Not a catapult.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Boost", meta=(ClampMin="0.0", UIMin="0.0", UIMax="3000.0"))
	float ExitBoostForce = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Boost", meta=(ClampMin="0.0", UIMin="0.0", UIMax="300.0"))
	float ExitTopMargin = 100.f; // Distance below the top within which the gentle exit cap kicks in.

	// === VISUALS ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual")
	UNiagaraSystem* ColumnNiagaraSystem = nullptr; // Main Niagara used to render the vertical wind column.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual")
	UNiagaraSystem* ExitRingNiagaraSystem = nullptr; // Optional Niagara ring placed at the top exit of the column.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual", meta=(ClampMin="0.01", UIMin="0.01", UIMax="10.0"))
	float ColumnNiagaraComponentScale = 1.f; // Uniform scale applied to the column Niagara component.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual", meta=(ClampMin="0.1", UIMin="0.1", UIMax="3.0"))
	float ColumnVisualRadiusMultiplier = 1.f; // Visual radius multiplier; does not change the gameplay collision box.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual", meta=(ClampMin="0.0", UIMin="0.0", UIMax="200.0"))
	float ColumnWindSpawnRate = 12.f; // Spawn rate forwarded to common User.Wind* Niagara parameters.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual", meta=(ClampMin="0.0", UIMin="0.0", UIMax="200.0"))
	float ColumnLeavesSpawnRate = 0.f; // Optional leaves/debris spawn rate forwarded to common User.Leaves* Niagara parameters.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual", meta=(ClampMin="0.0", UIMin="0.0", UIMax="1.0"))
	float ColumnOpacity = 1.f; // Opacity/intensity value sent to the column Niagara.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual", meta=(ClampMin="0.01", UIMin="0.01", UIMax="10.0"))
	float ExitRingNiagaraScale = 1.f; // Extra scale for the exit ring Niagara without changing gameplay size.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual", meta=(ClampMin="1.0", UIMin="1.0", UIMax="2000.0"))
	float ExitRingReferenceRadius = 200.f; // Column half-width that makes the ring use its authored scale.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual", meta=(ClampMin="0.1", UIMin="0.1", UIMax="3.0"))
	float ExitRingRadiusMultiplier = 1.05f; // Ring radius multiplier based on the widest column half extent.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual")
	float ExitRingVerticalOffset = 0.f; // Local Z offset from the column top for aligning the ring asset.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual")
	FRotator ExitRingRotationOffset = FRotator(90.f, 0.f, 0.f); // Rotation offset used to align ring assets authored in another axis.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Visual", meta=(ClampMin="0.0", UIMin="0.0", UIMax="1.0"))
	float ExitRingOpacity = 0.75f; // Opacity/intensity value sent to the ring Niagara.

	// === DEBUG ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Debug")
	bool bWindDebugMode = false; // Draws helper geometry, shows trigger bounds, and enables WindColumn logs.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind|Debug", meta=(ClampMin="0.0", UIMin="0.0", UIMax="10.0"))
	float DebugDrawThickness = 2.f; // Line thickness used for debug helpers.

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
						const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
					  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ConfigureVisuals();
	void ConfigureColumnNiagara() const;
	void ConfigureExitRingNiagara() const;
	void DrawWindDebug() const;
	void ApplyLift(APlayerCharacter* Player);
	float GetSafeApplyInterval() const;
	FVector GetColumnScaledExtent() const;
	FVector GetColumnWorldTop() const;
	FVector GetExitRingWorldLocation() const;

	UPROPERTY()
	TMap<APlayerCharacter*, FTimerHandle> ActiveTimers;
};
