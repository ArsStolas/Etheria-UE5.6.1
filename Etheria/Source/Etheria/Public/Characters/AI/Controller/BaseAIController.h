/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAIController - Header"
 * Notes: State machine, perception, proximity, yaw-only rotation, leash teleport.
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight") float SightRadius = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight") float LoseSightRadius = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight") float SightFOVDegrees = 90.f;

	/* ── Hearing ── */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Hearing") float HearingRange = 1200.f;

	/* ── Proximity ── */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity") bool bUseProximityDetection = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(EditCondition="bUseProximityDetection", ClampMin="50")) float ProximityRadius = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(EditCondition="bUseProximityDetection", ClampMin="0.05", ClampMax="2.0")) float ProximityCheckInterval = 0.2f;

	/* ── Flee ── */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="100")) float FleeSafeDistance = 2000.f;

	/* ── Combat ── */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat") float AttackRange = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat") float AttackCooldown = 1.5f;

	/** Rotation speed when facing target (degrees/sec). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement") float FaceTargetRotationSpeed = 8.f;

	/* ── Idle ── */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Idle") float IdleAnimInterval = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Idle", meta=(ClampMin="0")) float IdleAnimRandomDeviation = 3.f;

	/* ── Strafing ── */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement") bool bStrafeInCombat = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(EditCondition="bStrafeInCombat", ClampMin="0.5")) float StrafeDirectionChangeInterval = 2.f;

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
	int32 StrafeDirection = 1;
	FVector SpawnOrigin = FVector::ZeroVector;
};
