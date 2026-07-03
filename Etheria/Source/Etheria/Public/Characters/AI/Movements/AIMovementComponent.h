/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: ArsStolas
 * Class: "AIMovementComponent - Header"
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

	UFUNCTION(BlueprintCallable, Category="AI|Movement") void StartPatrol();
	UFUNCTION(BlueprintCallable, Category="AI|Movement") void StopPatrol();

	UFUNCTION(BlueprintCallable, Category="AI|Movement") bool MoveToLocation(const FVector& Target, float AcceptanceOverride = -1.f, bool bExactGoal = false);

	UFUNCTION(BlueprintCallable, Category="AI|Movement") bool MoveToActorDirect(AActor* Goal, float InAcceptanceRadius);

	UFUNCTION(BlueprintCallable, Category="AI|Movement") void CancelPathMove();

	UFUNCTION(BlueprintPure, Category="AI|Movement") bool IsPathMoveActive() const;
	UFUNCTION(BlueprintCallable, Category="AI|Movement") void StopMovement();
	UFUNCTION(BlueprintCallable, Category="AI|Movement") FVector GetNextPatrolPoint();

	UFUNCTION(BlueprintCallable, Category="AI|Movement") bool FleeFrom(AActor* Threat);

	UFUNCTION(BlueprintPure, Category="AI|Movement") bool IsPatrolling() const { return bIsPatrolling; }
	UFUNCTION(BlueprintPure, Category="AI|Movement") bool HasReachedDestination() const;
	UFUNCTION(BlueprintCallable, Category="AI|Movement") void SetDesiredSpeed(float Speed);

	void SetPatrolSpline(USplineComponent* Spline) { PatrolSpline = Spline; }

	UFUNCTION(BlueprintPure, Category="AI|Movement") FVector GetPatrolOrigin() const { return PatrolOrigin; }

	UFUNCTION(BlueprintPure, Category="AI|Movement") int32 GetNumPatrolPoints() const;

	UPROPERTY(BlueprintAssignable, Category="AI|Movement") FOnPatrolPointReached OnPatrolPointReached;
	UPROPERTY(BlueprintAssignable, Category="AI|Movement") FOnPatrolCompleted OnPatrolCompleted;
	UPROPERTY(BlueprintAssignable, Category="AI|Movement") FOnMovementTargetUpdated OnMovementTargetUpdated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="200", ClampMax="4000",
		ToolTip="Max ground acceleration (cm/s^2). Lower = softer, more animal starts; the engine default 2048 reads twitchy."))
	float MaxAcceleration = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement",
		meta=(ClampMin="50", ClampMax="1000", ToolTip="Rotation speed at full (chase) speed. Lower values give smoother turns."))
	float MovementRotationRate = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="20", ClampMax="1000",
		ToolTip="Turn rate at low speed. Blends up to MovementRotationRate as the AI speeds up."))
	float LowSpeedRotationRate = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="200",
		ToolTip="Deceleration when stopping. Lower = smoother. 2048 (default) reads abrupt at the chase→attack halt."))
	float BrakingDeceleration = 1024.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement") EPatrolMode PatrolMode = EPatrolMode::Stationary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(EditCondition="PatrolMode==EPatrolMode::Path")) EPatrolLoopMode PatrolLoopMode = EPatrolLoopMode::Loop;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="AI|Movement|Path",
		meta=(EditCondition="PatrolMode==EPatrolMode::Path",
		ToolTip="Optional. Drag a spline actor from the level here. Its spline overrides the character's built-in PatrolSpline. Leave empty to use the character's own spline."))
	TObjectPtr<AActor> PatrolPathActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0", ToolTip="Seconds the AI waits at each patrol point."))
	float WaitTimeAtPoint = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0"))
	float WaitTimeRandomDeviation = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0", ClampMax="1",
		ToolTip="Chance to play an AIAnimation ActivityMontage (graze/peck) at a patrol point."))
	float ActivityChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0.3", EditCondition="ActivityChance>0",
		ToolTip="Seconds between dwell activities at a patrol point so the NPC keeps moving naturally during the wait."))
	float ActivityRepeatInterval = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="10"))
	float AcceptanceRadius = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Zone", meta=(EditCondition="PatrolMode==EPatrolMode::Zone", ClampMin="100",
		ToolTip="Random patrol radius. AI picks a reachable point within this distance."))
	float PatrolRadius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Speed", meta=(ToolTip="Speed when patrolling."))
	float PatrolSpeed = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Speed", meta=(ToolTip="Speed when chasing a target."))
	float ChaseSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Speed", meta=(ToolTip="Speed when running away from a threat."))
	float FleeSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Speed", meta=(ClampMin="0.1", ToolTip="How quickly MaxWalkSpeed interpolates to the desired value."))
	float AccelerationInterpSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Flee", meta=(ClampMin="200", ToolTip="Distance to flee from the threat per move request."))
	float FleeDistance = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Flee", meta=(ClampMin="0", ClampMax="1",
		ToolTip="How strongly a fleeing AI commits to its current heading vs re-picking the furthest escape point. Higher = smoother panic arcs."))
	float FleeTurnCommitment = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Flee", meta=(ClampMin="0", ClampMax="60",
		ToolTip="Max degrees each individual's escape heading is randomly offset, so a startled herd fans out instead of overlapping."))
	float FleeScatterSpread = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Repath", meta=(ClampMin="0",
		ToolTip="Minimum goal movement (cm) before re-pathing to a moving target. Higher = smoother but slightly laggier tracking."))
	float RepathTolerance = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Repath", meta=(ClampMin="0",
		ToolTip="Minimum delay between re-paths to a near-identical goal. Caps repath frequency to avoid stutter."))
	float MinRepathInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Repath", meta=(ClampMin="0.1",
		ToolTip="Max seconds a path is kept before a forced refresh even if the goal stayed in the dead-zone."))
	float RepathMaxStale = 0.4f;

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
	void CacheSplineWorldPositions();
	void DrawDebugPatrol() const;

	UPROPERTY() TObjectPtr<ABaseAICharacter> OwnerCharacter;
	UPROPERTY() TObjectPtr<UCharacterMovementComponent> MovementComp;
	UPROPERTY() TObjectPtr<USplineComponent> PatrolSpline;

	TArray<FVector> CachedSplineWorldPoints;

	FVector CurrentDestination = FVector::ZeroVector;
	FVector PatrolOrigin = FVector::ZeroVector;

	FVector LastRequestedGoal = FVector::ZeroVector;
	float RepathCooldown = 0.f;
	float RepathStaleTimer = 0.f;
	bool bHasLastGoal = false;

	TWeakObjectPtr<AActor> CurrentMoveGoalActor;
	float CurrentMoveGoalAcceptance = -1.f;

	float FleeScatterAngle = 0.f;
	bool bFleeScatterRolled = false;

	float DesiredMaxSpeed = 0.f;
	float WaitTimer = 0.f;
	float ActivityRepeatTimer = 0.f;
	int32 CurrentPatrolIndex = 0;
	int32 PatrolDirection = 1;
	int32 PatrolRetryCount = 0;
	static constexpr int32 MaxPatrolRetries = 3;
	bool bIsPatrolling = false;
	bool bIsWaiting = false;
	bool bPatrolFinished = false;
	bool bMoveRequestActive = false;
	float MoveGraceTimer = 0.f;
	static constexpr float MOVE_GRACE_DURATION = 0.3f;
};
