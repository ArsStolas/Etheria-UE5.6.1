/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Class: AIMovementComponent - Header
 * Handles patrol zones, patrol paths, and smooth acceleration-based movement.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/AI/AI_Types.h"
#include "AIMovementComponent.generated.h"

class ABaseAICharacter;
class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPatrolPointReached, int32, PointIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPatrolCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovementTargetUpdated, FVector, NewTarget);

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UAIMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAIMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* ── API ── */

	/** Start patrolling based on current PatrolMode. */
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void StartPatrol();

	/** Stop patrolling and stand still. */
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void StopPatrol();

	/** Move toward a world location using smooth acceleration. */
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void MoveToLocation(const FVector& Target);

	/** Stop all movement immediately. */
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void StopMovement();

	/** Get the next patrol destination (depends on mode). */
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	FVector GetNextPatrolPoint();

	UFUNCTION(BlueprintPure, Category = "AI|Movement")
	bool IsPatrolling() const { return bIsPatrolling; }

	UFUNCTION(BlueprintPure, Category = "AI|Movement")
	bool HasReachedDestination() const;

	/* ── Dispatchers ── */

	UPROPERTY(BlueprintAssignable, Category = "AI|Movement")
	FOnPatrolPointReached OnPatrolPointReached;

	UPROPERTY(BlueprintAssignable, Category = "AI|Movement")
	FOnPatrolCompleted OnPatrolCompleted;

	UPROPERTY(BlueprintAssignable, Category = "AI|Movement")
	FOnMovementTargetUpdated OnMovementTargetUpdated;

	/* ── Config ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	EPatrolMode PatrolMode = EPatrolMode::Stationary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	EPatrolLoopMode PatrolLoopMode = EPatrolLoopMode::Loop;

	/** Radius for Zone patrol mode. Random nav-mesh point picked within. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Zone", meta = (EditCondition = "PatrolMode==EPatrolMode::Zone", ClampMin = "100"))
	float PatrolRadius = 800.f;

	/** Waypoints for Path patrol mode (world-space actors or vectors). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Path", meta = (EditCondition = "PatrolMode==EPatrolMode::Path", MakeEditWidget=true))
	TArray<FVector> PatrolPoints;

	/** How long to wait at each patrol point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (ClampMin = "0"))
	float WaitTimeAtPoint = 2.f;

	/** Random extra wait added on top of WaitTimeAtPoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (ClampMin = "0"))
	float WaitTimeRandomDeviation = 1.f;

	/** Walk speed while patrolling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Speed")
	float PatrolSpeed = 200.f;

	/** Run speed while chasing / returning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Speed")
	float ChaseSpeed = 500.f;

	/** How quickly the AI accelerates toward target speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Speed", meta = (ClampMin = "0.1"))
	float AccelerationInterpSpeed = 5.f;

	/** Distance threshold to consider "arrived" at a patrol point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (ClampMin = "10"))
	float AcceptanceRadius = 100.f;

	/** Set desired speed and let component interpolate smoothly. */
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void SetDesiredSpeed(float Speed);

protected:
	virtual void BeginPlay() override;

private:
	void HandlePatrolTick(float DeltaTime);
	void AdvancePatrolIndex();
	FVector GetRandomPointInZone() const;
	void ApplySmoothAcceleration(float DeltaTime);

	UPROPERTY()
	TObjectPtr<ABaseAICharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComp;

	FVector CurrentDestination = FVector::ZeroVector;
	float DesiredMaxSpeed = 0.f;
	float WaitTimer = 0.f;
	int32 CurrentPatrolIndex = 0;
	int32 PatrolDirection = 1; // 1 forward, -1 backward (ping-pong)
	bool bIsPatrolling = false;
	bool bIsWaiting = false;
	bool bPatrolFinished = false;
};