/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAIController - Header"
 * Notes: State machine, perception, robust flee, idle variation respect.
 *        Initial patrol kickoff is deferred via timer so it runs AFTER the character's
 *        BeginPlay (which is when the patrol spline is wired up).
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight", meta=(ToolTip="How far this AI can see.")) float SightRadius = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight", meta=(ToolTip="Distance at which AI loses sight. Should be > SightRadius.")) float LoseSightRadius = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight", meta=(ToolTip="Half-angle of the sight cone in degrees.")) float SightFOVDegrees = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Hearing", meta=(ToolTip="360-degree hearing radius.")) float HearingRange = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(ToolTip="Enable 360-degree close range detection.")) bool bUseProximityDetection = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(EditCondition="bUseProximityDetection", ClampMin="50")) float ProximityRadius = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(EditCondition="bUseProximityDetection", ClampMin="0.05", ClampMax="2.0")) float ProximityCheckInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="100", ToolTip="Distance from threat at which AI considers itself safe.")) float FleeSafeDistance = 2000.f;

	/** When the threat is closer than this, the AI re-evaluates its flee path every tick instead of waiting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="50", ToolTip="Panic radius. AI recalculates flee direction every tick when threat is this close."))
	float FleePanicRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="0.1", ClampMax="3.0", ToolTip="Normal interval between flee direction recalculations.")) float FleeReevalInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat", meta=(ToolTip="Fallback attack range if no CombatComponent attacks.")) float AttackRange = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat") float AttackCooldown = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ToolTip="Rotation interpolation speed toward target. Lower = smoother.")) float FaceTargetRotationSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Idle") float IdleAnimInterval = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Idle", meta=(ClampMin="0")) float IdleAnimRandomDeviation = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ToolTip="Strafe around target between attacks.")) bool bStrafeInCombat = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(EditCondition="bStrafeInCombat", ClampMin="0.5")) float StrafeDirectionChangeInterval = 2.f;

	/** Delay (seconds) between OnPossess and the first patrol kickoff. Gives BeginPlay time to wire up the
	 *  patrol spline and gives the navmesh time to be ready. Lower = snappier; too low and Path mode breaks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Init", meta=(ClampMin="0.0", ClampMax="2.0",
		ToolTip="Delay before initial patrol kicks off after the controller possesses the pawn. Required for Path mode."))
	float InitialPatrolDelay = 0.3f;

private:
	void SetupPerception();
	void CheckProximityDetection();
	void DrawDebugPerception() const;
	float GetEffectiveAttackRange() const;

	/** Deferred patrol kickoff. Called via timer after OnPossess so the character has finished its own BeginPlay. */
	void TryStartInitialPatrol();

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

	FTimerHandle InitialPatrolTimerHandle;
};