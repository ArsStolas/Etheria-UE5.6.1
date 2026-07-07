/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UPlayerCameraEffectsComponent" - Header
 * Notes: Procedural camera feel for the player:
 *        - Impact shakes when the player's attacks connect (regular + crit).
 *        - Movement tilt/bob while walking, accentuated while sprinting.
 *        - Progressive falling tremor that builds with fall speed and
 *          stabilizes when gliding or landing (with a landing thud).
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerCameraEffectsComponent.generated.h"

class APlayerCharacter;
class UCameraComponent;
class UCameraShakeBase;
class APlayerController;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UPlayerCameraEffectsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerCameraEffectsComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Master switch. Disabling also clears any offset currently applied to the camera. */
	UFUNCTION(BlueprintCallable, Category="Camera Effects")
	void SetEffectsEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="Camera Effects")
	bool AreEffectsEnabled() const { return bEffectsEnabled; }

	/** Current 0..1 falling tremor intensity (useful for UI/audio feedback). */
	UFUNCTION(BlueprintPure, Category="Camera Effects|Fall")
	float GetFallTrauma() const { return FallTrauma; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

// ============================================================
// CONFIG - HIT SHAKES
// ============================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Effects|Hit Shake")
	bool bEnableHitShake = true;

	/** Shake played when a regular attack connects. Defaults to the C++ tuned shake. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Effects|Hit Shake", meta=(EditCondition="bEnableHitShake"))
	TSubclassOf<UCameraShakeBase> HitShakeClass;

	/** Shake played when a critical hit connects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Effects|Hit Shake", meta=(EditCondition="bEnableHitShake"))
	TSubclassOf<UCameraShakeBase> CritShakeClass;

	/** Damage amount that maps to a shake scale of 1.0. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Hit Shake", meta=(ClampMin="1.0", EditCondition="bEnableHitShake"))
	float HitShakeReferenceDamage = 25.f;

	UPROPERTY(EditAnywhere, Category="Camera Effects|Hit Shake", meta=(ClampMin="0.1", EditCondition="bEnableHitShake"))
	float HitShakeMinScale = 0.6f;

	UPROPERTY(EditAnywhere, Category="Camera Effects|Hit Shake", meta=(ClampMin="0.1", EditCondition="bEnableHitShake"))
	float HitShakeMaxScale = 1.75f;

// ============================================================
// CONFIG - MOVEMENT TILT & BOB
// ============================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Effects|Movement Tilt")
	bool bEnableMovementTilt = true;

	/** If false (default), tilt and bob only apply while sprinting; walking stays perfectly steady. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Effects|Movement Tilt")
	bool bApplyWhileWalking = false;

	/** Camera roll (degrees) at full lateral walk speed. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Movement Tilt", meta=(ClampMin="0.0", ClampMax="10.0", EditCondition="bEnableMovementTilt"))
	float TiltRollAngle = 1.6f;

	/** Forward lean (degrees) at full forward walk speed. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Movement Tilt", meta=(ClampMin="0.0", ClampMax="10.0", EditCondition="bEnableMovementTilt"))
	float TiltPitchAngle = 0.8f;

	/** Small rhythmic roll sway synced with the bob cycle, so straight running still feels alive. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Movement Tilt", meta=(ClampMin="0.0", ClampMax="5.0", EditCondition="bEnableMovementTilt"))
	float TiltSwayAngle = 0.35f;

	/** Multiplier applied to tilt and bob while sprinting. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Movement Tilt", meta=(ClampMin="1.0", ClampMax="4.0", EditCondition="bEnableMovementTilt"))
	float SprintTiltMultiplier = 1.9f;

	UPROPERTY(EditAnywhere, Category="Camera Effects|Movement Tilt", meta=(ClampMin="0.5", ClampMax="20.0", EditCondition="bEnableMovementTilt"))
	float TiltInterpSpeed = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Effects|Movement Bob")
	bool bEnableMovementBob = true;

	/** Vertical bob amplitude in cm at full walk speed. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Movement Bob", meta=(ClampMin="0.0", ClampMax="20.0", EditCondition="bEnableMovementBob"))
	float BobAmplitude = 2.0f;

	/** Lateral sway amplitude as a ratio of BobAmplitude. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Movement Bob", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="bEnableMovementBob"))
	float BobLateralRatio = 0.4f;

	/** Distance in cm covered during one full bob cycle (two steps). */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Movement Bob", meta=(ClampMin="50.0", ClampMax="1000.0", EditCondition="bEnableMovementBob"))
	float BobCycleLength = 320.f;

	UPROPERTY(EditAnywhere, Category="Camera Effects|Movement Bob", meta=(ClampMin="1.0", ClampMax="4.0", EditCondition="bEnableMovementBob"))
	float SprintBobMultiplier = 1.6f;

// ============================================================
// CONFIG - FALL SHAKE
// ============================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Effects|Fall Shake")
	bool bEnableFallShake = true;

	/** Fall speed (cm/s) at which the tremor starts. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Fall Shake", meta=(ClampMin="0.0", EditCondition="bEnableFallShake"))
	float FallShakeMinSpeed = 800.f;

	/** Fall speed (cm/s) at which the tremor reaches full intensity. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Fall Shake", meta=(ClampMin="100.0", EditCondition="bEnableFallShake"))
	float FallShakeMaxSpeed = 3200.f;

	/** How fast the tremor builds up (intensity per second). */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Fall Shake", meta=(ClampMin="0.1", ClampMax="10.0", EditCondition="bEnableFallShake"))
	float FallShakeBuildRate = 1.2f;

	/** How fast the tremor stabilizes when gliding, diving or grounded (intensity per second). */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Fall Shake", meta=(ClampMin="0.1", ClampMax="20.0", EditCondition="bEnableFallShake"))
	float FallStabilizeRate = 3.5f;

	/** Max rotation noise (degrees) at full tremor. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Fall Shake", meta=(ClampMin="0.0", ClampMax="15.0", EditCondition="bEnableFallShake"))
	float FallShakeRotAmplitude = 1.7f;

	/** Max location noise (cm) at full tremor. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Fall Shake", meta=(ClampMin="0.0", ClampMax="30.0", EditCondition="bEnableFallShake"))
	float FallShakeLocAmplitude = 2.5f;

	UPROPERTY(EditAnywhere, Category="Camera Effects|Fall Shake", meta=(ClampMin="1.0", ClampMax="60.0", EditCondition="bEnableFallShake"))
	float FallShakeFrequency = 11.f;

// ============================================================
// CONFIG - LANDING
// ============================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera Effects|Landing")
	bool bEnableLandingShake = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera Effects|Landing", meta=(EditCondition="bEnableLandingShake"))
	TSubclassOf<UCameraShakeBase> LandShakeClass;

	/** Minimum fall speed (cm/s) at impact for the landing shake to play. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Landing", meta=(ClampMin="0.0", EditCondition="bEnableLandingShake"))
	float LandShakeMinFallSpeed = 900.f;

	/** Fall speed (cm/s) at impact mapping to LandShakeMaxScale. */
	UPROPERTY(EditAnywhere, Category="Camera Effects|Landing", meta=(ClampMin="100.0", EditCondition="bEnableLandingShake"))
	float LandShakeMaxFallSpeed = 2600.f;

	UPROPERTY(EditAnywhere, Category="Camera Effects|Landing", meta=(ClampMin="0.1", ClampMax="5.0", EditCondition="bEnableLandingShake"))
	float LandShakeMaxScale = 1.5f;

private:
	UFUNCTION() void HandleHit(AActor* HitActor, float Damage);
	UFUNCTION() void HandleHitCrit(AActor* HitActor, float Damage);
	UFUNCTION() void HandleLanded(const FHitResult& Hit);

	void UpdateMovementTilt(float DeltaTime);
	void UpdateFallShake(float DeltaTime);
	void ApplyCameraOffsets();
	void ClearCameraOffsets();
	void PlayShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale) const;
	float ComputeDamageShakeScale(float Damage) const;
	APlayerController* GetPlayerController() const;
	bool IsSprinting() const;
	bool IsRopeActive() const;

	UPROPERTY() TObjectPtr<APlayerCharacter> PlayerOwner = nullptr;
	UPROPERTY() TObjectPtr<UCameraComponent> Camera = nullptr;

	bool bEffectsEnabled = true;

	// Smoothed offsets
	FRotator CurrentTilt = FRotator::ZeroRotator;
	FVector  CurrentBob = FVector::ZeroVector;
	FRotator ShakeRot = FRotator::ZeroRotator;
	FVector  ShakeLoc = FVector::ZeroVector;

	// What we applied to the camera last frame (subtracted before re-applying,
	// so external systems moving the camera are never fought).
	FRotator LastAppliedRot = FRotator::ZeroRotator;
	FVector  LastAppliedLoc = FVector::ZeroVector;

	float BobPhase = 0.f;
	float NoiseTime = 0.f;
	float FallTrauma = 0.f;
	float LastFallSpeedZ = 0.f;
};
