/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Class: BaseAICharacter - Header
 * Base class for all AI-driven characters: NPCs, neutral mobs, and aggressive enemies.
 */

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Characters/AI/AI_Types.h"

#include "BaseAICharacter.generated.h"

class UAIMovementComponent;
class UAIAnimationComponent;
class ABaseAIController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIStateChanged, EAIState, OldState, EAIState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetAcquired, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetLost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIDamaged, AActor*, Instigator);

UCLASS()
class ETHERIA_API ABaseAICharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	ABaseAICharacter();

	/* ── Getters ── */

	UFUNCTION(BlueprintPure, Category = "AI")
	UAIMovementComponent* GetAIMovement() const { return AIMovementComponent; }

	UFUNCTION(BlueprintPure, Category = "AI")
	UAIAnimationComponent* GetAIAnimation() const { return AIAnimationComponent; }

	UFUNCTION(BlueprintPure, Category = "AI")
	EAIHostilityType GetHostilityType() const { return HostilityType; }

	UFUNCTION(BlueprintPure, Category = "AI")
	EAIRank GetRank() const { return Rank; }

	UFUNCTION(BlueprintPure, Category = "AI")
	EAIState GetCurrentAIState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "AI")
	AActor* GetCurrentTarget() const { return CurrentTarget; }

	UFUNCTION(BlueprintPure, Category = "AI")
	float GetLeashRange() const { return LeashRange; }

	/* ── State ── */

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAIState(EAIState NewState);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ClearTarget();

	/** Called by perception or damage system. Handles hostility logic. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void OnPerceiveTarget(AActor* PerceivedActor);

	/** Called when AI takes damage — may provoke neutrals. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void OnReceiveDamage(AActor* DamageInstigator, float DamageAmount);

	/* ── Dispatchers ── */

	UPROPERTY(BlueprintAssignable, Category = "AI")
	FOnAIStateChanged OnAIStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "AI")
	FOnTargetAcquired OnTargetAcquired;

	UPROPERTY(BlueprintAssignable, Category = "AI")
	FOnTargetLost OnTargetLost;

	UPROPERTY(BlueprintAssignable, Category = "AI")
	FOnAIDamaged OnAIDamaged;

protected:
	virtual void BeginPlay() override;

	/* ── Identity ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Identity")
	EAIHostilityType HostilityType = EAIHostilityType::Passive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Identity", meta = (EditCondition = "HostilityType==EAIHostilityType::Aggressive"))
	EAIRank Rank = EAIRank::Basic;

	/* ── Perception ── */

	/** Max distance at which an aggressive AI will chase before giving up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception", meta = (ClampMin = "0"))
	float LeashRange = 2000.f;

	/** If true, this NPC can be spoken to / interacted with. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Interaction")
	bool bIsInteractable = false;

	/* ── Components ── */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAIMovementComponent> AIMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAIAnimationComponent> AIAnimationComponent;

private:
	UPROPERTY()
	EAIState CurrentState = EAIState::Idle;

	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	FVector SpawnLocation;
};
