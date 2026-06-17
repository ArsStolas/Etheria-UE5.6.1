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
	UFUNCTION(BlueprintCallable, Category="AI|Movement") bool MoveToLocation(const FVector& Target, float AcceptanceOverride = -1.f);
	UFUNCTION(BlueprintCallable, Category="AI|Movement") void StopMovement();
	UFUNCTION(BlueprintCallable, Category="AI|Movement") FVector GetNextPatrolPoint();

	/** Flee away from a threat. Returns true if a reachable point that increases distance was found and a move
	 *  issued; false if cornered (no nav escape) so the caller can face/hold instead of shuffling into the threat. */
	UFUNCTION(BlueprintCallable, Category="AI|Movement") bool FleeFrom(AActor* Threat);

	UFUNCTION(BlueprintPure, Category="AI|Movement") bool IsPatrolling() const { return bIsPatrolling; }
	UFUNCTION(BlueprintPure, Category="AI|Movement") bool HasReachedDestination() const;
	UFUNCTION(BlueprintCallable, Category="AI|Movement") void SetDesiredSpeed(float Speed);

	void SetPatrolSpline(USplineComponent* Spline) { PatrolSpline = Spline; }

	/** Stable patrol anchor captured at StartPatrol (Zone center). Used by herd members to cluster on the leader. */
	UFUNCTION(BlueprintPure, Category="AI|Movement") FVector GetPatrolOrigin() const { return PatrolOrigin; }

	UFUNCTION(BlueprintPure, Category="AI|Movement") int32 GetNumPatrolPoints() const;

	/* ── Dispatchers ── */
	UPROPERTY(BlueprintAssignable, Category="AI|Movement") FOnPatrolPointReached OnPatrolPointReached;
	UPROPERTY(BlueprintAssignable, Category="AI|Movement") FOnPatrolCompleted OnPatrolCompleted;
	UPROPERTY(BlueprintAssignable, Category="AI|Movement") FOnMovementTargetUpdated OnMovementTargetUpdated;

	/* ── Config ── */
	
	/* ── Rotation ── */

	/** How fast the AI rotates toward its movement direction (degrees/sec). Lower = smoother turns for humanoids. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement",
		meta=(ClampMin="50", ClampMax="1000", ToolTip="Rotation speed at full (chase) speed. Lower values give smoother turns."))
	float MovementRotationRate = 400.f;

	/** Yaw turn rate when nearly stationary; lerps up to MovementRotationRate at chase speed (avoids tower-pivot look). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="20", ClampMax="1000",
		ToolTip="Turn rate at low speed. Blends up to MovementRotationRate as the AI speeds up."))
	float LowSpeedRotationRate = 220.f;

	/** Walking braking deceleration. ~800-1200 = smooth combat stops; the 2048 engine default reads like hitting a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="200",
		ToolTip="Deceleration when stopping. Lower = smoother. 2048 (default) reads abrupt at the chase→attack halt."))
	float BrakingDeceleration = 1024.f;

	/** How this AI patrols: Stationary, Zone (random within radius), or Path (follows spline). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement") EPatrolMode PatrolMode = EPatrolMode::Stationary;
	
	/** How the path loops: Loop, PingPong, or Once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(EditCondition="PatrolMode==EPatrolMode::Path")) EPatrolLoopMode PatrolLoopMode = EPatrolLoopMode::Loop;

	/** Optional: an actor in the world that carries a SplineComponent. If set, the AI follows THAT spline
	 *  instead of the character's built-in PatrolSpline. Lets level designers draw paths visually in the level. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="AI|Movement|Path",
		meta=(EditCondition="PatrolMode==EPatrolMode::Path",
		ToolTip="Optional. Drag a spline actor from the level here. Its spline overrides the character's built-in PatrolSpline. Leave empty to use the character's own spline."))
	TObjectPtr<AActor> PatrolPathActor;

	/** How long to wait at each patrol point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0", ToolTip="Seconds the AI waits at each patrol point."))
	float WaitTimeAtPoint = 2.f;

	/** Random extra wait on top of WaitTimeAtPoint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0"))
	float WaitTimeRandomDeviation = 1.f;

	/** Chance (0-1) to play a long activity montage (graze/peck/sleep) on arriving at a patrol point. 0 = always plain idle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0", ClampMax="1",
		ToolTip="Chance to play an AIAnimation ActivityMontage (graze/peck) at a patrol point."))
	float ActivityChance = 0.f;

	/** While dwelling at a point, replay an activity/idle about every this many seconds (graze→look up→graze) instead of freezing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="0.3", EditCondition="ActivityChance>0",
		ToolTip="Seconds between dwell activities at a patrol point so the NPC keeps moving naturally during the wait."))
	float ActivityRepeatInterval = 1.5f;
	
	/** Distance to consider 'arrived' at a patrol point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement", meta=(ClampMin="10"))
	float AcceptanceRadius = 100.f;
	
	/** Radius for Zone patrol mode. AI picks random NavMesh points within this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Zone", meta=(EditCondition="PatrolMode==EPatrolMode::Zone", ClampMin="100",
		ToolTip="Random patrol radius. AI picks a reachable point within this distance."))
	float PatrolRadius = 800.f;
	
	/** Walk speed during patrol. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Speed", meta=(ToolTip="Speed when patrolling."))
	float PatrolSpeed = 200.f;

	/** Run speed during chase or return. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Speed", meta=(ToolTip="Speed when chasing a target."))
	float ChaseSpeed = 500.f;

	/** Speed when fleeing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Speed", meta=(ToolTip="Speed when running away from a threat."))
	float FleeSpeed = 450.f;

	/** Acceleration interpolation speed. Higher = snappier speed changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Speed", meta=(ClampMin="0.1", ToolTip="How quickly MaxWalkSpeed interpolates to the desired value."))
	float AccelerationInterpSpeed = 5.f;
	
	/** How far the AI tries to flee per flee request. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Flee", meta=(ClampMin="200", ToolTip="Distance to flee from the threat per move request."))
	float FleeDistance = 1500.f;

	/* ── Repath gating ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Repath", meta=(ClampMin="0",
		ToolTip="Minimum goal movement (cm) before re-pathing to a moving target. Higher = smoother but slightly laggier tracking."))
	float RepathTolerance = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement|Repath", meta=(ClampMin="0",
		ToolTip="Minimum delay between re-paths to a near-identical goal. Caps repath frequency to avoid stutter."))
	float MinRepathInterval = 0.2f;

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

	/** Cached world-space positions of spline points. Snapshotted at StartPatrol. */
	TArray<FVector> CachedSplineWorldPoints;

	FVector CurrentDestination = FVector::ZeroVector;
	FVector PatrolOrigin = FVector::ZeroVector;

	FVector LastRequestedGoal = FVector::ZeroVector;
	float RepathCooldown = 0.f;
	bool bHasLastGoal = false;

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