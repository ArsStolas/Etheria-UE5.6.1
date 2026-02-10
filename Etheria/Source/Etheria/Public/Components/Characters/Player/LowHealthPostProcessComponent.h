/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "LowHealthPostProcessComponent" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h" // FComponentReference
#include "TimerManager.h"       // FTimerHandle

#include "LowHealthPostProcessComponent.generated.h"

class UHealthComponent;
class UPostProcessComponent;
class UMaterialInterface;

/**
 * Drives a post-process "low health" effect (heartbeat pulse) by updating the BLEND WEIGHT
 * of a post-process material added to a PostProcessComponent.
 *
 * Intended usage: add this component to BP_PlayerCharacter, assign:
 * - PostProcessComponent (or leave empty to auto-find)
 * - PostProcessMaterial (must be a Post Process material)
 * - PulseMinWeight / PulseMaxWeight and ThresholdPercent
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API ULowHealthPostProcessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULowHealthPostProcessComponent();

	// =============================================================
	// Blueprint API
	// =============================================================
	UFUNCTION(BlueprintCallable, Category="PostProcess|LowHealth")
	void SetPulseWeightRange(const float NewMin, const float NewMax);

	UFUNCTION(BlueprintCallable, Category="PostProcess|LowHealth")
	void SetThresholdPercent(const float NewThresholdPercent);

	UFUNCTION(BlueprintCallable, Category="PostProcess|LowHealth")
	void SetHeartbeatBPM(const float NewBPM);

	UFUNCTION(BlueprintCallable, Category="PostProcess|LowHealth")
	void ForceEnableEffect(const bool bEnable);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float InMaxHealth);

	void StartHeartbeat();
	void StopHeartbeat();
	void UpdateHeartbeat();

	UPostProcessComponent* ResolvePostProcessComponent();
	UHealthComponent* ResolveHealthComponent();

	void ApplyBlendWeight(const float NewWeight);
	void EnsureBlendableRegistered();

	/**
	 * Small "double bump" pulse in [0..1] for a more heart-like rhythm than a pure sine.
	 * Uses gaussian peaks across a [0..1) phase.
	 */
	float ComputeHeartbeatAlpha(const float Phase01) const;

	// =============================================================
	// Settings
	// =============================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PostProcess|LowHealth")
	bool bEnableLowHealthEffect = true;

	/** If empty, the component will try to find the first PostProcessComponent on the owner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PostProcess|LowHealth")
	FComponentReference PostProcessComponentRef;

	/** If empty, the component will try to find the first HealthComponent on the owner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PostProcess|LowHealth")
	FComponentReference HealthComponentRef;

	/** Post process material to add as a blendable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PostProcess|LowHealth")
	TObjectPtr<UMaterialInterface> PostProcessMaterial = nullptr;

	/** Effect starts when Health/MaxHealth <= ThresholdPercent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PostProcess|LowHealth", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float ThresholdPercent = 0.2f;


	/** Min blend weight (usually 0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PostProcess|LowHealth", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float PulseMinWeight = 0.0f;

	/** Max blend weight (example 0.15). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PostProcess|LowHealth", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float PulseMaxWeight = 0.15f;

	/** Heartbeat speed (beats per minute). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PostProcess|LowHealth", meta=(ClampMin="10.0", UIMin="30.0", UIMax="200.0"))
	float HeartbeatBPM = 90.f;

	/** Update rate for the pulse (seconds). Lower = smoother. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PostProcess|LowHealth", meta=(ClampMin="0.005", UIMin="0.01", UIMax="0.05"))
	float UpdateInterval = 0.02f;

private:
	TWeakObjectPtr<UPostProcessComponent> CachedPostProcess;
	TWeakObjectPtr<UHealthComponent> CachedHealth;

	int32 BlendableIndex = INDEX_NONE;
	FTimerHandle HeartbeatTimerHandle;
	float HeartbeatStartTime = 0.f;
	bool bHeartbeatActive = false;
};
