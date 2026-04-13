/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Class: AIMovementComponent - Source
 */

#include "Characters/AI/Movements/AIMovementComponent.h"

#include "Characters/AI/BaseAICharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SplineComponent.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "DrawDebugHelpers.h"

UAIMovementComponent::UAIMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

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
		if (OwnerCharacter)
		{
			MovementComp = OwnerCharacter->GetCharacterMovement();
		}
	}
}

void UAIMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ApplySmoothAcceleration(DeltaTime);

	// Tick down grace timer
	if (MoveGraceTimer > 0.f)
	{
		MoveGraceTimer -= DeltaTime;
	}

	if (bIsPatrolling)
	{
		HandlePatrolTick(DeltaTime);
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebugPatrol)
	{
		DrawDebugPatrol();
	}
#endif
}

/* ─────────────────── Public API ─────────────────── */

void UAIMovementComponent::StartPatrol()
{
	if (PatrolMode == EPatrolMode::Stationary) return;

	EnsureInitialized();
	if (!OwnerCharacter) return;

	PatrolOrigin = OwnerCharacter->GetActorLocation();

	bIsPatrolling = true;
	bPatrolFinished = false;
	bIsWaiting = false;
	bMoveRequestActive = false;
	CurrentPatrolIndex = 0;
	PatrolDirection = 1;

	CurrentDestination = GetNextPatrolPoint();

	// Set speed IMMEDIATELY — the core fix
	DesiredMaxSpeed = PatrolSpeed;
	if (MovementComp)
	{
		MovementComp->MaxWalkSpeed = PatrolSpeed;
	}

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

	CurrentDestination = Target;
	OnMovementTargetUpdated.Broadcast(Target);

	// Guarantee non-zero walk speed
	if (MovementComp && MovementComp->MaxWalkSpeed < 1.f)
	{
		MovementComp->MaxWalkSpeed = FMath::Max(DesiredMaxSpeed, PatrolSpeed);
	}

	bool bSuccess = false;

	if (AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
	{
		// bProjectDestinationToNavigation = TRUE — this was the critical fix
		// Without this, any point slightly off the NavMesh causes silent failure
		const EPathFollowingRequestResult::Type Result =
			AIC->MoveToLocation(
				Target,
				AcceptanceRadius,
				true,	// bStopOnOverlap
				true,	// bUsePathfinding
				true,	// bProjectDestinationToNavigation  ← FIX
				true,	// bCanStrafe
				TSubclassOf<UNavigationQueryFilter>(),
				true	// bAllowPartialPath
			);

		bSuccess = (Result == EPathFollowingRequestResult::RequestSuccessful
				 || Result == EPathFollowingRequestResult::AlreadyAtGoal);

		if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			// We're already there — don't set move active
			bMoveRequestActive = false;
			MoveGraceTimer = 0.f;
			return true;
		}

		if (bSuccess)
		{
			bMoveRequestActive = true;
			MoveGraceTimer = MOVE_GRACE_DURATION;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AIMovement: MoveToLocation FAILED for %s -> %s"),
				*OwnerCharacter->GetName(), *Target.ToString());
			bMoveRequestActive = false;
		}
	}

	return bSuccess;
}

void UAIMovementComponent::StopMovement()
{
	DesiredMaxSpeed = 0.f;
	bMoveRequestActive = false;

	EnsureInitialized();
	if (OwnerCharacter)
	{
		if (AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
		{
			AIC->StopMovement();
		}
	}
}

void UAIMovementComponent::FleeFrom(AActor* Threat)
{
	EnsureInitialized();
	if (!OwnerCharacter || !Threat) return;

	const FVector MyLoc = OwnerCharacter->GetActorLocation();
	const FVector ThreatLoc = Threat->GetActorLocation();
	const FVector FleeDir = (MyLoc - ThreatLoc).GetSafeNormal();
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
	}

	DesiredMaxSpeed = FleeSpeed;
	if (MovementComp)
	{
		MovementComp->MaxWalkSpeed = FleeSpeed;
	}

	MoveToLocation(FinalTarget);
}

FVector UAIMovementComponent::GetNextPatrolPoint()
{
	EnsureInitialized();

	if (PatrolMode == EPatrolMode::Zone)
	{
		return GetRandomPointInZone();
	}

	if (PatrolMode == EPatrolMode::Path && PatrolSpline)
	{
		const int32 NumPoints = PatrolSpline->GetNumberOfSplinePoints();
		if (NumPoints > 0)
		{
			const int32 ClampedIndex = FMath::Clamp(CurrentPatrolIndex, 0, NumPoints - 1);
			return PatrolSpline->GetLocationAtSplinePoint(ClampedIndex, ESplineCoordinateSpace::World);
		}
	}

	return OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
}

int32 UAIMovementComponent::GetNumPatrolPoints() const
{
	if (PatrolSpline)
	{
		return PatrolSpline->GetNumberOfSplinePoints();
	}
	return 0;
}

bool UAIMovementComponent::HasReachedDestination() const
{
	if (!OwnerCharacter) return true;
	return FVector::Dist(OwnerCharacter->GetActorLocation(), CurrentDestination) <= AcceptanceRadius;
}

void UAIMovementComponent::SetDesiredSpeed(float Speed)
{
	DesiredMaxSpeed = Speed;
}

/* ─────────────────── Private — Patrol Logic ─────────────────── */

void UAIMovementComponent::HandlePatrolTick(float DeltaTime)
{
	if (bPatrolFinished) return;

	// Phase 1: Waiting at a patrol point
	if (bIsWaiting)
	{
		WaitTimer -= DeltaTime;
		if (WaitTimer <= 0.f)
		{
			ResumePatrolAfterWait();
		}
		return;
	}

	// Phase 2: Moving to a destination — wait for grace timer to avoid instant re-detection
	if (MoveGraceTimer > 0.f)
	{
		return;
	}

	// Phase 3: Check if the move has completed
	bool bArrived = false;

	if (bMoveRequestActive)
	{
		// Primary check: ask the AI controller if it's still moving
		if (OwnerCharacter)
		{
			if (const AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
			{
				const EPathFollowingStatus::Type Status = AIC->GetMoveStatus();
				bArrived = (Status != EPathFollowingStatus::Moving);
			}
		}
	}
	else
	{
		// MoveToLocation returned AlreadyAtGoal or failed — treat as arrived
		bArrived = true;
	}

	// Fallback: distance check
	if (!bArrived && HasReachedDestination())
	{
		bArrived = true;
	}

	if (bArrived)
	{
		BeginWaitAtPoint();
	}
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
	if (MovementComp)
	{
		MovementComp->MaxWalkSpeed = PatrolSpeed;
	}

	if (!MoveToLocation(CurrentDestination))
	{
		// Move request failed — try next point next tick
		UE_LOG(LogTemp, Warning, TEXT("AIMovement: Patrol move failed, advancing to next point."));
		BeginWaitAtPoint();
	}
}

void UAIMovementComponent::AdvancePatrolIndex()
{
	if (PatrolMode == EPatrolMode::Zone)
	{
		return;
	}

	const int32 NumPoints = GetNumPatrolPoints();
	if (NumPoints == 0) return;

	const int32 NextIndex = CurrentPatrolIndex + PatrolDirection;

	switch (PatrolLoopMode)
	{
	case EPatrolLoopMode::Loop:
		CurrentPatrolIndex = NextIndex % NumPoints;
		if (CurrentPatrolIndex < 0) CurrentPatrolIndex += NumPoints;
		break;

	case EPatrolLoopMode::PingPong:
		if (NextIndex >= NumPoints || NextIndex < 0)
		{
			PatrolDirection *= -1;
			CurrentPatrolIndex += PatrolDirection;
		}
		else
		{
			CurrentPatrolIndex = NextIndex;
		}
		break;

	case EPatrolLoopMode::Once:
		if (NextIndex >= NumPoints)
		{
			bPatrolFinished = true;
			OnPatrolCompleted.Broadcast();
			return;
		}
		CurrentPatrolIndex = NextIndex;
		break;
	}
}

FVector UAIMovementComponent::GetRandomPointInZone() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return OwnerCharacter->GetActorLocation();

	FNavLocation Result;
	const FVector Center = PatrolOrigin.IsZero() ? OwnerCharacter->GetActorLocation() : PatrolOrigin;
	if (NavSys->GetRandomReachablePointInRadius(Center, PatrolRadius, Result))
	{
		return Result.Location;
	}

	return OwnerCharacter->GetActorLocation();
}

void UAIMovementComponent::ApplySmoothAcceleration(float DeltaTime)
{
	if (!MovementComp) return;

	const float Current = MovementComp->MaxWalkSpeed;
	const float Interped = FMath::FInterpTo(Current, DesiredMaxSpeed, DeltaTime, AccelerationInterpSpeed);
	MovementComp->MaxWalkSpeed = Interped;
}

/* ─────────────────── Debug ─────────────────── */

void UAIMovementComponent::DrawDebugPatrol() const
{
#if ENABLE_DRAW_DEBUG
	if (!OwnerCharacter) return;

	const UWorld* World = GetWorld();
	if (!World) return;

	const FVector MyLoc = OwnerCharacter->GetActorLocation();

	if (PatrolMode == EPatrolMode::Zone)
	{
		const FVector Center = PatrolOrigin.IsZero() ? MyLoc : PatrolOrigin;
		DrawDebugSphere(World, Center, PatrolRadius, 24, FColor::Cyan, false, -1.f, 0, 2.f);

		if (bIsPatrolling && !bIsWaiting)
		{
			DrawDebugLine(World, MyLoc, CurrentDestination, FColor::Green, false, -1.f, 0, 2.f);
			DrawDebugSphere(World, CurrentDestination, 30.f, 8, FColor::Green, false, -1.f, 0, 2.f);
		}
	}
	else if (PatrolMode == EPatrolMode::Path && PatrolSpline)
	{
		const int32 NumPoints = PatrolSpline->GetNumberOfSplinePoints();
		const float SplineLen = PatrolSpline->GetSplineLength();
		const int32 Segments = FMath::Max(static_cast<int32>(SplineLen / 20.f), NumPoints * 10);

		for (int32 i = 0; i < Segments; ++i)
		{
			const float Alpha0 = static_cast<float>(i) / static_cast<float>(Segments);
			const float Alpha1 = static_cast<float>(i + 1) / static_cast<float>(Segments);
			const FVector P0 = PatrolSpline->GetLocationAtDistanceAlongSpline(Alpha0 * SplineLen, ESplineCoordinateSpace::World);
			const FVector P1 = PatrolSpline->GetLocationAtDistanceAlongSpline(Alpha1 * SplineLen, ESplineCoordinateSpace::World);
			DrawDebugLine(World, P0, P1, FColor::Orange, false, -1.f, 0, 3.f);
		}

		for (int32 i = 0; i < NumPoints; ++i)
		{
			const FVector Pt = PatrolSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			const FColor Col = (i == CurrentPatrolIndex) ? FColor::Yellow : FColor::Orange;
			DrawDebugSphere(World, Pt, 35.f, 8, Col, false, -1.f, 0, 3.f);
			DrawDebugString(World, Pt + FVector(0, 0, 60.f), FString::Printf(TEXT("[%d]"), i), nullptr, Col, -1.f, true);
		}

		if (bIsPatrolling && !bIsWaiting)
		{
			DrawDebugLine(World, MyLoc, CurrentDestination, FColor::Green, false, -1.f, 0, 2.f);
		}
	}

	// State text
	FString StateStr;
	if (bIsWaiting) StateStr = FString::Printf(TEXT("WAITING %.1f"), WaitTimer);
	else if (bMoveRequestActive) StateStr = TEXT("MOVING");
	else if (bIsPatrolling) StateStr = TEXT("PATROL (idle)");
	else StateStr = TEXT("STOPPED");

	DrawDebugString(World, MyLoc + FVector(0, 0, 120.f), StateStr, nullptr, FColor::White, -1.f, true);
#endif
}
