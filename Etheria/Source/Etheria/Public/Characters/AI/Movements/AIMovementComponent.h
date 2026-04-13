/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Class: AIMovementComponent - Header
 * Handles patrol zones, patrol spline paths, flee, and smooth acceleration-based movement.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/AI/AI_Types.h"
#include "AIMovementComponent.generated.h"

class ABaseAICharacter;
class UCharacterMovementComponent;
class USplineComponent;

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

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void StartPatrol();

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void StopPatrol();

	/** Issue a move request. Returns true if the request was accepted. */
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	bool MoveToLocation(const FVector& Target);

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void StopMovement();

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	FVector GetNextPatrolPoint();

	/** Flee away from a threat actor. */
	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void FleeFrom(AActor* Threat);

	UFUNCTION(BlueprintPure, Category = "AI|Movement")
	bool IsPatrolling() const { return bIsPatrolling; }

	UFUNCTION(BlueprintPure, Category = "AI|Movement")
	bool HasReachedDestination() const;

	UFUNCTION(BlueprintCallable, Category = "AI|Movement")
	void SetDesiredSpeed(float Speed);

	void SetPatrolSpline(USplineComponent* Spline) { PatrolSpline = Spline; }

	UFUNCTION(BlueprintPure, Category = "AI|Movement")
	int32 GetNumPatrolPoints() const;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (EditCondition = "PatrolMode==EPatrolMode::Path"))
	EPatrolLoopMode PatrolLoopMode = EPatrolLoopMode::Loop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Zone", meta = (EditCondition = "PatrolMode==EPatrolMode::Zone", ClampMin = "100"))
	float PatrolRadius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (ClampMin = "0"))
	float WaitTimeAtPoint = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (ClampMin = "0"))
	float WaitTimeRandomDeviation = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Speed")
	float PatrolSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Speed")
	float ChaseSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Speed")
	float FleeSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Flee", meta = (ClampMin = "200"))
	float FleeDistance = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement|Speed", meta = (ClampMin = "0.1"))
	float AccelerationInterpSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (ClampMin = "10"))
	float AcceptanceRadius = 100.f;

	/* ── Debug ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Debug")
	bool bShowDebugPatrol = false;

protected:
	virtual void BeginPlay() override;

private:
	void HandlePatrolTick(float DeltaTime);
	void BeginWaitAtPoint();
	void ResumePatrolAfterWait();
	void AdvancePatrolIndex();
	FVector GetRandomPointInZone() const;
	void ApplySmoothAcceleration(float DeltaTime);
	void EnsureInitialized();
	void DrawDebugPatrol() const;

	UPROPERTY()
	TObjectPtr<ABaseAICharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComp;

	UPROPERTY()
	TObjectPtr<USplineComponent> PatrolSpline;

	FVector CurrentDestination = FVector::ZeroVector;
	FVector PatrolOrigin = FVector::ZeroVector;
	float DesiredMaxSpeed = 0.f;
	float WaitTimer = 0.f;
	int32 CurrentPatrolIndex = 0;
	int32 PatrolDirection = 1;
	bool bIsPatrolling = false;
	bool bIsWaiting = false;
	bool bPatrolFinished = false;
	bool bMoveRequestActive = false;

	/** Cooldown after issuing MoveToLocation — prevents instant re-arrival detection. */
	float MoveGraceTimer = 0.f;
	static constexpr float MOVE_GRACE_DURATION = 0.3f;
};
