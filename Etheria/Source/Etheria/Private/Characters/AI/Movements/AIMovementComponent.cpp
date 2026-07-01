/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: ArsStolas
 * Class: "AIMovementComponent - Source"
 */

#include "Characters/AI/Movements/AIMovementComponent.h"

#include "Characters/AI/BaseAICharacter.h"
#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SplineComponent.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "DrawDebugHelpers.h"

UAIMovementComponent::UAIMovementComponent() { PrimaryComponentTick.bCanEverTick = true; }

void UAIMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureInitialized();
	if (MovementComp)
	{
		// Smooth, non-abrupt stops (the chase→attack halt otherwise reads like hitting a wall at the 2048 default).
		MovementComp->BrakingDecelerationWalking = BrakingDeceleration;
	}
}

void UAIMovementComponent::EnsureInitialized()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ABaseAICharacter>(GetOwner());
		if (OwnerCharacter) MovementComp = OwnerCharacter->GetCharacterMovement();
	}

	/* Lazy-resolve the patrol spline. ──────────────────────────────────────
	 * Why this matters: for placed pawns, AAIController::OnPossess can fire
	 * BEFORE the character's BeginPlay — meaning SetPatrolSpline() hasn't run
	 * yet when the controller calls StartPatrol(). The previous code then
	 * silently fell back to the character's own location as a "patrol point",
	 * leaving the AI immobile.
	 *
	 * Resolution priority:
	 *   1. PatrolSpline was already set explicitly (by character::BeginPlay).
	 *   2. PatrolPathActor (an actor placed in the level) — preferred for
	 *      level-design workflows where designers draw paths in the world.
	 *   3. The character's built-in PatrolSpline subobject.
	 * ──────────────────────────────────────────────────────────────────── */
	if (!PatrolSpline && OwnerCharacter)
	{
		if (PatrolPathActor)
		{
			PatrolSpline = PatrolPathActor->FindComponentByClass<USplineComponent>();
			if (!PatrolSpline)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[%s] PatrolPathActor '%s' has no SplineComponent — falling back to the character's built-in PatrolSpline."),
					*OwnerCharacter->GetName(), *PatrolPathActor->GetName());
			}
		}
		if (!PatrolSpline)
		{
			PatrolSpline = OwnerCharacter->GetPatrolSpline();
		}
	}
}

void UAIMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ApplySmoothAcceleration(DeltaTime);
	if (MoveGraceTimer > 0.f) MoveGraceTimer -= DeltaTime;
	if (RepathCooldown > 0.f) RepathCooldown -= DeltaTime;
	if (bIsPatrolling) HandlePatrolTick(DeltaTime);

#if ENABLE_DRAW_DEBUG
	if (OwnerCharacter && OwnerCharacter->ShouldShowDebugPatrol()) DrawDebugPatrol();
#endif
}

/* ═══════════ Spline Cache ═══════════ */

void UAIMovementComponent::CacheSplineWorldPositions()
{
	EnsureInitialized();

	CachedSplineWorldPoints.Reset();
	if (!PatrolSpline)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] Path patrol mode requested but no PatrolSpline could be found. "
			     "Either define points on the character's PatrolSpline subobject in the BP, "
			     "or set PatrolPathActor to a spline actor in the level."),
			OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("AIMovement"));
		return;
	}

	const int32 Num = PatrolSpline->GetNumberOfSplinePoints();
	CachedSplineWorldPoints.Reserve(Num);

	for (int32 i = 0; i < Num; ++i)
	{
		// Snapshot world position NOW — before the character moves
		CachedSplineWorldPoints.Add(PatrolSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
	}

	if (CachedSplineWorldPoints.Num() < 2)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] PatrolSpline has only %d point(s). Path mode needs at least 2."),
			OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("AIMovement"),
			CachedSplineWorldPoints.Num());
	}
}

/* ═══════════ Public API ═══════════ */

void UAIMovementComponent::StartPatrol()
{
	if (PatrolMode == EPatrolMode::Stationary) return;

	EnsureInitialized();
	if (!OwnerCharacter) return;

	// Pack followers are driven by UpdatePackFollow (cluster on the leader), not their own patrol loop —
	// running both would give two competing movement drivers. Only the leader patrols.
	if (OwnerCharacter->GetPackID() != NAME_None && !OwnerCharacter->IsPackLeader()) return;

	PatrolOrigin = OwnerCharacter->GetActorLocation();

	// Cache spline points in world space so they don't drift with the character
	if (PatrolMode == EPatrolMode::Path)
	{
		CacheSplineWorldPositions();

		// Bail out cleanly if the path is unusable, instead of silently moving
		// to the character's own location and looking frozen forever.
		if (CachedSplineWorldPoints.Num() < 2)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[%s] StartPatrol(Path) aborted: spline must have at least 2 points."),
				*OwnerCharacter->GetName());
			bIsPatrolling = false;
			return;
		}
	}

	bIsPatrolling = true;
	bPatrolFinished = false;
	bIsWaiting = false;
	bMoveRequestActive = false;
	CurrentPatrolIndex = 0;
	PatrolDirection = 1;

	CurrentDestination = GetNextPatrolPoint();
	DesiredMaxSpeed = PatrolSpeed;
	if (MovementComp) MovementComp->MaxWalkSpeed = PatrolSpeed;
	MoveToLocation(CurrentDestination);
}

void UAIMovementComponent::StopPatrol()
{
	bIsPatrolling = false;
	bIsWaiting = false;
	bMoveRequestActive = false;
	StopMovement();
}

bool UAIMovementComponent::MoveToLocation(const FVector& Target, float AcceptanceOverride)
{
	EnsureInitialized();
	if (!OwnerCharacter) return false;

	const float Accept = (AcceptanceOverride >= 0.f) ? AcceptanceOverride : AcceptanceRadius;

	// Repath gate: don't re-pathfind every frame toward a near-identical, recently-requested goal.
	if (bMoveRequestActive && bHasLastGoal && RepathCooldown > 0.f
		&& FVector::DistSquared(Target, LastRequestedGoal) < FMath::Square(RepathTolerance))
	{
		CurrentDestination = Target;
		OnMovementTargetUpdated.Broadcast(Target);
		return true;
	}

	CurrentDestination = Target;
	OnMovementTargetUpdated.Broadcast(Target);

	if (MovementComp && MovementComp->MaxWalkSpeed < 1.f)
		MovementComp->MaxWalkSpeed = FMath::Max(DesiredMaxSpeed, PatrolSpeed);

	bool bSuccess = false;
	if (AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
	{
		const EPathFollowingRequestResult::Type Result = AIC->MoveToLocation(
			Target, Accept, true, true, true, true,
			TSubclassOf<UNavigationQueryFilter>(), true);

		bSuccess = (Result == EPathFollowingRequestResult::RequestSuccessful
				 || Result == EPathFollowingRequestResult::AlreadyAtGoal);

		if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			bMoveRequestActive = false;
			bHasLastGoal = false;
			MoveGraceTimer = 0.f;
			return true;
		}

		if (bSuccess)
		{
			bMoveRequestActive = true;
			MoveGraceTimer = MOVE_GRACE_DURATION;
			LastRequestedGoal = Target;
			bHasLastGoal = true;
			RepathCooldown = MinRepathInterval;
		}
		else
		{
			bMoveRequestActive = false;
			bHasLastGoal = false;
		}
	}
	return bSuccess;
}

void UAIMovementComponent::StopMovement()
{
	DesiredMaxSpeed = 0.f;
	bMoveRequestActive = false;
	bHasLastGoal = false;
	RepathCooldown = 0.f;
	EnsureInitialized();
	if (OwnerCharacter)
		if (AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
			AIC->StopMovement();
}

bool UAIMovementComponent::FleeFrom(AActor* Threat)
{
	EnsureInitialized();
	if (!OwnerCharacter || !Threat) return false;

	const FVector MyLoc = OwnerCharacter->GetActorLocation();
	const FVector ThreatLoc = Threat->GetActorLocation();
	FVector AwayDir = (MyLoc - ThreatLoc).GetSafeNormal2D();
	if (AwayDir.IsNearlyZero()) AwayDir = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const float CurDistSq = FVector::DistSquared2D(MyLoc, ThreatLoc);

	// Sample candidate directions in the away-hemisphere; pick the reachable one that gets us FURTHEST from the
	// threat (so we never randomly bolt sideways into the predator or hug a wall toward it).
	static const float Angles[] = { 0.f, -25.f, 25.f, -50.f, 50.f, -75.f, 75.f, -90.f, 90.f }; // wider fan, but stay in the away-hemisphere
	FVector BestTarget = MyLoc + AwayDir * FleeDistance;
	float BestScore = TNumericLimits<float>::Lowest();
	bool bFound = false;
	for (const float Ang : Angles)
	{
		FVector Cand = MyLoc + AwayDir.RotateAngleAxis(Ang, FVector::UpVector) * FleeDistance;
		if (NavSys)
		{
			FNavLocation NavRes;
			if (!NavSys->ProjectPointToNavigation(Cand, NavRes, FVector(400.f, 400.f, 250.f))) continue;
			Cand = NavRes.Location;
		}
		const float Score = FVector::DistSquared2D(Cand, ThreatLoc);
		if (Score > CurDistSq && Score > BestScore) { BestScore = Score; BestTarget = Cand; bFound = true; }
	}

	// Nothing in the fan: retry straight along the away-vector with a WIDER projection extent before giving up
	// (rescues prey sitting on a thin/edge navmesh tile that the tight extent above missed).
	if (!bFound && NavSys)
	{
		FNavLocation NavRes;
		if (NavSys->ProjectPointToNavigation(MyLoc + AwayDir * FleeDistance, NavRes, FVector(800.f, 800.f, 500.f))
			&& FVector::DistSquared2D(NavRes.Location, ThreatLoc) > CurDistSq)
		{ BestTarget = NavRes.Location; bFound = true; }
	}

	if (bFound)
	{
		DesiredMaxSpeed = FleeSpeed;
		if (MovementComp) MovementComp->MaxWalkSpeed = FleeSpeed;
		MoveToLocation(BestTarget);
		return true;
	}

	// No reachable nav point. Tell "genuinely cornered on a working navmesh" (let the caller face/hold the threat)
	// apart from "no usable navmesh here at all" (nav not baked/streamed): in the latter, degrade to a DIRECT,
	// non-pathfinding flee so prey still visibly bolts away instead of freezing and staring at the threat.
	FNavLocation Here;
	const bool bNavUsableHere = NavSys && NavSys->ProjectPointToNavigation(MyLoc, Here, FVector(200.f, 200.f, 300.f));
	if (bNavUsableHere) return false; // cornered on a real navmesh — let the caller face/hold

	if (AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
	{
		DesiredMaxSpeed = FleeSpeed;
		if (MovementComp) MovementComp->MaxWalkSpeed = FleeSpeed;
		CurrentDestination = MyLoc + AwayDir * FleeDistance;
		OnMovementTargetUpdated.Broadcast(CurrentDestination);
		AIC->MoveToLocation(CurrentDestination, AcceptanceRadius, true,
			/*bUsePathfinding=*/false, /*bProjectDestinationToNavigation=*/false, true,
			TSubclassOf<UNavigationQueryFilter>(), true);
		return true;
	}
	return false;
}

FVector UAIMovementComponent::GetNextPatrolPoint()
{
	EnsureInitialized();

	if (PatrolMode == EPatrolMode::Zone)
		return GetRandomPointInZone();

	if (PatrolMode == EPatrolMode::Path)
	{
		// Use CACHED world positions — not live spline (which moves with the character)
		if (CachedSplineWorldPoints.Num() > 0)
		{
			const int32 Idx = FMath::Clamp(CurrentPatrolIndex, 0, CachedSplineWorldPoints.Num() - 1);
			return CachedSplineWorldPoints[Idx];
		}
	}

	return OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
}

int32 UAIMovementComponent::GetNumPatrolPoints() const
{
	if (PatrolMode == EPatrolMode::Path)
		return CachedSplineWorldPoints.Num();
	return 0;
}

bool UAIMovementComponent::HasReachedDestination() const
{
	if (!OwnerCharacter) return true;
	return FVector::Dist(OwnerCharacter->GetActorLocation(), CurrentDestination) <= AcceptanceRadius;
}

void UAIMovementComponent::SetDesiredSpeed(float Speed) { DesiredMaxSpeed = Speed; }

/* ═══════════ Patrol Logic ═══════════ */

void UAIMovementComponent::HandlePatrolTick(float DeltaTime)
{
	if (bPatrolFinished) return;

	if (bIsWaiting)
	{
		WaitTimer -= DeltaTime;

		// Dwell loop: replay an activity/idle through the wait (graze→look up→graze) instead of freezing in one pose.
		if (ActivityChance > 0.f && OwnerCharacter)
		{
			ActivityRepeatTimer -= DeltaTime;
			if (ActivityRepeatTimer <= 0.f && WaitTimer > ActivityRepeatInterval * 0.5f)
			{
				ActivityRepeatTimer = ActivityRepeatInterval + FMath::FRandRange(0.f, ActivityRepeatInterval * 0.5f);
				if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
					if (!Anim->IsPlayingAction())
						(FMath::FRand() < 0.6f) ? Anim->PlayRandomActivity() : Anim->PlayRandomIdle();
			}
		}

		if (WaitTimer <= 0.f) ResumePatrolAfterWait();
		return;
	}

	if (MoveGraceTimer > 0.f) return;

	// Real arrival = actually NEAR the point.
	if (HasReachedDestination())
	{
		PatrolRetryCount = 0;
		BeginWaitAtPoint();
		return;
	}

	// The path-follower stopped (GetMoveStatus is Idle on FAILURE too, not just success) but we're not near →
	// the path failed/aborted. Retry, then skip the unreachable point — never broadcast a false "reached".
	bool bMoveStopped = !bMoveRequestActive;
	if (bMoveRequestActive && OwnerCharacter)
		if (const AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
			bMoveStopped = (AIC->GetMoveStatus() != EPathFollowingStatus::Moving);

	if (bMoveStopped)
	{
		if (++PatrolRetryCount > MaxPatrolRetries)
		{
			PatrolRetryCount = 0;
			AdvancePatrolIndex();
			if (bPatrolFinished) return;
			CurrentDestination = GetNextPatrolPoint();
		}
		MoveToLocation(CurrentDestination);
		MoveGraceTimer = MOVE_GRACE_DURATION;
	}
}

void UAIMovementComponent::BeginWaitAtPoint()
{
	bMoveRequestActive = false;
	OnPatrolPointReached.Broadcast(CurrentPatrolIndex);
	DesiredMaxSpeed = 0.f;
	bIsWaiting = true;
	WaitTimer = WaitTimeAtPoint + FMath::FRandRange(0.f, WaitTimeRandomDeviation);

	// Optionally play a dwell activity (graze/peck/sleep) while waiting at the point.
	ActivityRepeatTimer = ActivityRepeatInterval; // schedule the next dwell beat (loop runs in HandlePatrolTick)
	if (ActivityChance > 0.f && FMath::FRand() < ActivityChance && OwnerCharacter)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->PlayRandomActivity();
}

void UAIMovementComponent::ResumePatrolAfterWait()
{
	bIsWaiting = false;
	AdvancePatrolIndex();
	if (bPatrolFinished) return;

	// Stop any dwell activity montage so DirectPlayback locomotion isn't gated (the AI would slide otherwise).
	if (OwnerCharacter)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->StopCurrentAction();

	CurrentDestination = GetNextPatrolPoint();
	DesiredMaxSpeed = PatrolSpeed;
	if (MovementComp) MovementComp->MaxWalkSpeed = PatrolSpeed;

	if (!MoveToLocation(CurrentDestination))
		BeginWaitAtPoint(); // Failed — try again next cycle
}

void UAIMovementComponent::AdvancePatrolIndex()
{
	if (PatrolMode == EPatrolMode::Zone) return;

	const int32 Num = GetNumPatrolPoints();
	if (Num == 0) return;

	const int32 Next = CurrentPatrolIndex + PatrolDirection;

	switch (PatrolLoopMode)
	{
	case EPatrolLoopMode::Loop:
		CurrentPatrolIndex = Next % Num;
		if (CurrentPatrolIndex < 0) CurrentPatrolIndex += Num;
		break;
	case EPatrolLoopMode::PingPong:
		if (Next >= Num || Next < 0) { PatrolDirection *= -1; CurrentPatrolIndex += PatrolDirection; }
		else CurrentPatrolIndex = Next;
		break;
	case EPatrolLoopMode::Once:
		if (Next >= Num) { bPatrolFinished = true; OnPatrolCompleted.Broadcast(); return; }
		CurrentPatrolIndex = Next;
		break;
	}
}

FVector UAIMovementComponent::GetRandomPointInZone() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;
	const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav) return OwnerCharacter->GetActorLocation();

	FVector Center = PatrolOrigin.IsZero() ? OwnerCharacter->GetActorLocation() : PatrolOrigin;
	// Herd: non-leaders cluster around the leader's STABLE patrol anchor (not its live position, which would
	// make the whole herd drift). UpdatePackFollow already handles tight cohesion.
	if (OwnerCharacter->GetPackID() != NAME_None)
		if (ABaseAICharacter* Leader = OwnerCharacter->GetPackLeader())
			if (Leader != OwnerCharacter && Leader->GetAIMovement())
			{
				const FVector LeaderOrigin = Leader->GetAIMovement()->GetPatrolOrigin();
				Center = LeaderOrigin.IsZero() ? Leader->GetActorLocation() : LeaderOrigin;
			}

	FNavLocation Res;
	if (Nav->GetRandomReachablePointInRadius(Center, PatrolRadius, Res)) return Res.Location;
	return OwnerCharacter->GetActorLocation();
}

void UAIMovementComponent::ApplySmoothAcceleration(float DeltaTime)
{
	if (!MovementComp) return;

	// Turn rate scales with speed (locomotion only — combat facing is driven by the controller with orient-off):
	// slow shuffles turn tightly, fast runs sweep wide instead of pivoting like a turret.
	if (MovementComp->bOrientRotationToMovement)
	{
		const float SpeedFrac = FMath::Clamp(MovementComp->Velocity.Size2D() / FMath::Max(ChaseSpeed, 1.f), 0.f, 1.f);
		MovementComp->RotationRate.Yaw = FMath::Lerp(LowSpeedRotationRate, MovementRotationRate, SpeedFrac);
	}

	if (!FMath::IsNearlyEqual(MovementComp->MaxWalkSpeed, DesiredMaxSpeed, 0.5f))
		MovementComp->MaxWalkSpeed = FMath::FInterpTo(MovementComp->MaxWalkSpeed, DesiredMaxSpeed, DeltaTime, AccelerationInterpSpeed);
}

/* ═══════════ Debug ═══════════ */

void UAIMovementComponent::DrawDebugPatrol() const
{
#if ENABLE_DRAW_DEBUG
	if (!OwnerCharacter) return;
	const UWorld* W = GetWorld();
	if (!W) return;
	const FVector Loc = OwnerCharacter->GetActorLocation();

	if (PatrolMode == EPatrolMode::Zone)
	{
		const FVector Center = PatrolOrigin.IsZero() ? Loc : PatrolOrigin;
		DrawDebugSphere(W, Center, PatrolRadius, 24, FColor::Cyan, false, -1.f, 0, 2.f);
		if (bIsPatrolling && !bIsWaiting)
		{
			DrawDebugLine(W, Loc, CurrentDestination, FColor::Green, false, -1.f, 0, 2.f);
			DrawDebugSphere(W, CurrentDestination, 30.f, 8, FColor::Green, false, -1.f, 0, 2.f);
		}
	}
	else if (PatrolMode == EPatrolMode::Path && CachedSplineWorldPoints.Num() > 0)
	{
		// Draw cached world points and lines between them
		for (int32 i = 0; i < CachedSplineWorldPoints.Num(); ++i)
		{
			const FVector& Pt = CachedSplineWorldPoints[i];
			const FColor Col = (i == CurrentPatrolIndex) ? FColor::Yellow : FColor::Orange;
			DrawDebugSphere(W, Pt, 35.f, 8, Col, false, -1.f, 0, 3.f);
			DrawDebugString(W, Pt + FVector(0, 0, 60.f), FString::Printf(TEXT("[%d]"), i), nullptr, Col, -1.f, true);

			if (i + 1 < CachedSplineWorldPoints.Num())
				DrawDebugLine(W, Pt, CachedSplineWorldPoints[i + 1], FColor::Orange, false, -1.f, 0, 2.f);
		}

		if (bIsPatrolling && !bIsWaiting)
			DrawDebugLine(W, Loc, CurrentDestination, FColor::Green, false, -1.f, 0, 2.f);
	}

	FString S;
	if (bIsWaiting) S = FString::Printf(TEXT("WAITING %.1f"), WaitTimer);
	else if (bMoveRequestActive) S = TEXT("MOVING");
	else if (bIsPatrolling) S = TEXT("PATROL");
	else S = TEXT("STOPPED");
	DrawDebugString(W, Loc + FVector(0, 0, 120.f), S, nullptr, FColor::White, -1.f, true);
#endif
}