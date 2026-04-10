/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Class: BaseAIController - Header
 * Controller with perception (sight/hearing) and state-driven behavior.
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

	/* ── Perception ── */

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/* ── Behavior ── */

	void HandleIdleState(float DeltaTime);
	void HandlePatrolState(float DeltaTime);
	void HandleChaseState(float DeltaTime);
	void HandleAttackState(float DeltaTime);
	void HandleReturnState(float DeltaTime);

	/* ── Perception Config ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float SightRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float LoseSightRadius = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float SightFOVDegrees = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float HearingRange = 1200.f;

	/** Distance at which AI starts attacking instead of chasing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float AttackRange = 200.f;

	/** Cooldown between attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float AttackCooldown = 1.5f;

	/** Idle timer: how long to wait in idle before playing a random idle anim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Idle")
	float IdleAnimInterval = 5.f;

private:
	void SetupPerception();

	UPROPERTY()
	TObjectPtr<ABaseAICharacter> AICharacter;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	float AttackTimer = 0.f;
	float IdleTimer = 0.f;
	FVector SpawnOrigin = FVector::ZeroVector;
};
