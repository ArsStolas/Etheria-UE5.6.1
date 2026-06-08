/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: ArsStolas
 * Class: "BaseAIController - Header"
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
class UAICombatComponent;
class UAICombatDirectorSubsystem;

UCLASS()
class ETHERIA_API ABaseAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABaseAIController(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION() void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION() void HandleAIStateChanged(EAIState OldState, EAIState NewState);
	UFUNCTION() void HandleDormancyChanged();

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Range", meta=(ClampMin="0.3", ClampMax="1.0",
		ToolTip="Fraction of attack range the AI closes to before attacking. 0.85 = stop at 85% of range, comfortably inside."))
	float CombatEngageRangeRatio = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Range", meta=(ClampMin="1.05", ClampMax="3.0",
		ToolTip="How far past attack range the AI keeps re-approaching before switching back to a full chase."))
	float CombatDisengageRangeRatio = 1.4f;

	/* ── Crowd avoidance (Detour) ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Avoidance",
		meta=(ToolTip="Enable Detour Crowd avoidance so groups of AI flow around each other instead of overlapping."))
	bool bUseCrowdAvoidance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Avoidance", meta=(EditCondition="bUseCrowdAvoidance", ClampMin="0",
		ToolTip="How strongly AI push apart from each other. Higher = more personal space."))
	float CrowdSeparationWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Avoidance", meta=(EditCondition="bUseCrowdAvoidance", ClampMin="0.1",
		ToolTip="Scales how far ahead the AI looks to avoid others."))
	float CrowdAvoidanceRangeMultiplier = 1.0f;

	/* ── Combat director (attack tokens) ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director",
		meta=(ToolTip="Limit how many AI may attack the SAME target at once. Others circle and wait their turn."))
	bool bUseAttackTokens = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bUseAttackTokens", ClampMin="1",
		ToolTip="Max number of AI attacking the same target simultaneously."))
	int32 MaxSimultaneousAttackers = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bUseAttackTokens", ClampMin="0.5",
		ToolTip="Safety auto-release for an attack turn if the AI never releases it (death, dormancy, etc.)."))
	float AttackTokenLeaseDuration = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Idle") float IdleAnimInterval = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Idle", meta=(ClampMin="0")) float IdleAnimRandomDeviation = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ToolTip="Circle the target while waiting for an attack turn (also used by the attack-token director).")) bool bStrafeInCombat = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="0.5", ToolTip="How often the circling direction flips.")) float StrafeDirectionChangeInterval = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="50", ToolTip="Speed while circling the target.")) float StrafeSpeed = 320.f;

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

	float GetChaseSpeed() const;
	void ApproachTarget(AActor* Target, float DesiredDistance);
	float GetCombatApproachDistance(float Range) const;

	/** Orbit the target at engage distance while waiting for an attack turn. */
	void CircleTarget(AActor* Target, float DeltaTime);

	void ConfigureCrowdAvoidance();

	UAICombatDirectorSubsystem* GetCombatDirector() const;
	bool TryTakeAttackTurn(AActor* Target);
	void ReleaseAttackTokenHeld();
	bool HasUsableAttack(const UAICombatComponent* Combat, float Distance) const;

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

	bool bHoldingAttackToken = false;
	TWeakObjectPtr<AActor> TokenTarget;

	FTimerHandle InitialPatrolTimerHandle;
};