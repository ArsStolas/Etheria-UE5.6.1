/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: ArsStolas
 * Class: "AIMovementComponent - Source"
 */

#include "Characters/AI/Movements/AIMovementComponent.h"

#include "Characters/AI/BaseAICharacter.h"
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

bool UAIMovementComponent::MoveToLocation(const FVector& Target)
{
	EnsureInitialized();
	if (!OwnerCharacter) return false;

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
			Target, AcceptanceRadius, true, true, true, true,
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

void UAIMovementComponent::FleeFrom(AActor* Threat)
{
	EnsureInitialized();
	if (!OwnerCharacter || !Threat) return;

	const FVector MyLoc = OwnerCharacter->GetActorLocation();
	const FVector ThreatLoc = Threat->GetActorLocation();
	const FVector AwayDir = (MyLoc - ThreatLoc).GetSafeNormal2D();

	// Add perpendicular random offset (30-60°) to avoid running straight into walls
	const float RandomAngle = FMath::FRandRange(-60.f, 60.f);
	const FVector FleeDir = AwayDir.RotateAngleAxis(RandomAngle, FVector::UpVector);
	const FVector FleeTarget = MyLoc + FleeDir * FleeDistance;

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FVector FinalTarget = FleeTarget;

	if (NavSys)
	{
		FNavLocation NavResult;
		if (NavSys->ProjectPointToNavigation(FleeTarget, NavResult, FVector(500.f, 500.f, 250.f)))
		{
			FinalTarget = NavResult.Location;
		}
		else
		{
			// Projection failed — try pure opposite direction without random angle
			const FVector FallbackTarget = MyLoc + AwayDir * FleeDistance * 0.5f;
			if (NavSys->ProjectPointToNavigation(FallbackTarget, NavResult, FVector(500.f, 500.f, 250.f)))
				FinalTarget = NavResult.Location;
			else
				FinalTarget = MyLoc + AwayDir * 200.f; // Desperate: just move a bit away
		}
	}

	DesiredMaxSpeed = FleeSpeed;
	if (MovementComp) MovementComp->MaxWalkSpeed = FleeSpeed;
	MoveToLocation(FinalTarget);
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
		if (WaitTimer <= 0.f) ResumePatrolAfterWait();
		return;
	}

	if (MoveGraceTimer > 0.f) return;

	bool bArrived = false;
	if (bMoveRequestActive)
	{
		if (OwnerCharacter)
			if (const AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
				bArrived = (AIC->GetMoveStatus() != EPathFollowingStatus::Moving);
	}
	else
	{
		bArrived = true;
	}

	if (!bArrived && HasReachedDestination()) bArrived = true;
	if (bArrived) BeginWaitAtPoint();
}

void UAIMovementComponent::BeginWaitAtPoint()
{
	bMoveRequestActive = false;
	OnPatrolPointReached.Broadcast(CurrentPatrolIndex);
	DesiredMaxSpeed = 0.f;
	bIsWaiting = true;
	WaitTimer = WaitTimeAtPoint + FMath::FRandRange(0.f, WaitTimeRandomDeviation);
}

void UAIMovementComponent::ResumePatrolAfterWait()
{
	bIsWaiting = false;
	AdvancePatrolIndex();
	if (bPatrolFinished) return;

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

	const FVector Center = PatrolOrigin.IsZero() ? OwnerCharacter->GetActorLocation() : PatrolOrigin;
	FNavLocation Res;
	if (Nav->GetRandomReachablePointInRadius(Center, PatrolRadius, Res)) return Res.Location;
	return OwnerCharacter->GetActorLocation();
}

void UAIMovementComponent::ApplySmoothAcceleration(float DeltaTime)
{
	if (!MovementComp) return;
	if (FMath::IsNearlyEqual(MovementComp->MaxWalkSpeed, DesiredMaxSpeed, 0.5f)) return;
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