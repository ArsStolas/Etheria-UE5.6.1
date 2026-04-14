/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAIController - Header"
 * Notes: State machine, perception, improved flee, idle variation respect.
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Characters/AI/AI_Types.h"
#include "Perception/AIPerceptionTypes.h"
#include "BaseAIController.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class ABaseAICharacter;

UCLASS()
class ETHERIA_API ABaseAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABaseAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION() void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void HandleIdleState(float DeltaTime);
	void HandlePatrolState(float DeltaTime);
	void HandleChaseState(float DeltaTime);
	void HandleAttackState(float DeltaTime);
	void HandleReturnState(float DeltaTime);
	void HandleFleeState(float DeltaTime);
	void HandleStaggerState(float DeltaTime);
	bool CheckLeashAndTeleport();
	void FaceTargetYawOnly(AActor* Target, float DeltaTime);

	/* ── Sight ── */

	/** Maximum detection distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight", meta=(ToolTip="How far this AI can see."))
	float SightRadius = 1500.f;

	/** Distance at which the AI loses track of a previously seen target. Should be > SightRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight", meta=(ToolTip="Range at which the AI loses sight of its target. Must be larger than SightRadius."))
	float LoseSightRadius = 2000.f;

	/** Half angle of the sight cone in degrees. 90 = 180° total FOV. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight", meta=(ToolTip="Field of view half-angle. 90 = 180 degree total cone."))
	float SightFOVDegrees = 90.f;

	/* ── Hearing ── */

	/** Radius for hearing detection (360°). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Hearing", meta=(ToolTip="360-degree hearing radius."))
	float HearingRange = 1200.f;

	/* ── Proximity ── */

	/** Enable 360° close-range detection (ignores sight direction). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(ToolTip="360-degree proximity sphere. Detects the player even behind the AI."))
	bool bUseProximityDetection = true;

	/** Radius of the proximity detection sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(EditCondition="bUseProximityDetection", ClampMin="50", ToolTip="Size of the proximity detection sphere."))
	float ProximityRadius = 400.f;

	/** Interval in seconds between proximity checks. Lower = more CPU. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(EditCondition="bUseProximityDetection", ClampMin="0.05", ClampMax="2.0", ToolTip="How often to run the proximity overlap test."))
	float ProximityCheckInterval = 0.2f;

	/* ── Flee ── */

	/** Distance from the threat at which the AI considers itself safe and stops fleeing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="100", ToolTip="Once this far from the threat, the AI stops fleeing and returns home."))
	float FleeSafeDistance = 2000.f;

	/** How often the fleeing AI re-evaluates its escape direction (seconds). Lower = more responsive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="0.1", ClampMax="3.0", ToolTip="Re-evaluation interval for flee direction. Lower = AI changes course more often."))
	float FleeReevalInterval = 0.5f;

	/* ── Combat ── */

	/** Fallback attack range used when AICombatComponent has no attacks configured. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat", meta=(ToolTip="Distance at which the AI starts attacking. Ignored if AICombatComponent has attacks."))
	float AttackRange = 200.f;

	/** Fallback attack cooldown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat", meta=(ToolTip="Time between attacks when not using AICombatComponent."))
	float AttackCooldown = 1.5f;

	/** How fast the AI rotates to face its target during combat (degrees/sec interpolation speed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ToolTip="Rotation interpolation speed toward the target. Lower = smoother."))
	float FaceTargetRotationSpeed = 8.f;

	/* ── Idle ── */

	/** Base interval between idle variation animations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Idle", meta=(ToolTip="Seconds between idle variation animations."))
	float IdleAnimInterval = 5.f;

	/** Random extra time added to the idle interval for variety. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Idle", meta=(ClampMin="0", ToolTip="Random deviation added to idle interval for natural feel."))
	float IdleAnimRandomDeviation = 3.f;

	/* ── Strafing ── */

	/** If true, the AI will strafe around its target between attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ToolTip="AI moves sideways around its target during combat pauses."))
	bool bStrafeInCombat = false;

	/** How often the strafe direction flips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(EditCondition="bStrafeInCombat", ClampMin="0.5", ToolTip="Interval between strafe direction changes."))
	float StrafeDirectionChangeInterval = 2.f;

private:
	void SetupPerception();
	void CheckProximityDetection();
	void DrawDebugPerception() const;
	float GetEffectiveAttackRange() const;

	UPROPERTY() TObjectPtr<ABaseAICharacter> AICharacter;
	UPROPERTY() TObjectPtr<UAISenseConfig_Sight> SightConfig;
	UPROPERTY() TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	float AttackTimer = 0.f;
	float IdleTimer = 0.f;
	float NextIdleAnimTime = 5.f;
	float ProximityTimer = 0.f;
	float StrafeTimer = 0.f;
	float FleeReevalTimer = 0.f;
	int32 StrafeDirection = 1;
	FVector SpawnOrigin = FVector::ZeroVector;
};
