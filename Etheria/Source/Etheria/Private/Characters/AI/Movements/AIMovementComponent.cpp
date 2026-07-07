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
#include "Components/CapsuleComponent.h"
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
		MovementComp->BrakingDecelerationWalking = BrakingDeceleration;
		MovementComp->MaxAcceleration = MaxAcceleration;
	}
}

void UAIMovementComponent::EnsureInitialized()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ABaseAICharacter>(GetOwner());
		if (OwnerCharacter) MovementComp = OwnerCharacter->GetCharacterMovement();
	}

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
	if (RepathStaleTimer > 0.f) RepathStaleTimer -= DeltaTime;
	if (bIsPatrolling) HandlePatrolTick(DeltaTime);

#if ENABLE_DRAW_DEBUG
	if (OwnerCharacter && OwnerCharacter->ShouldShowDebugPatrol()) DrawDebugPatrol();
#endif
}

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

void UAIMovementComponent::StartPatrol()
{
	if (PatrolMode == EPatrolMode::Stationary) return;

	EnsureInitialized();
	if (!OwnerCharacter) return;

	if (OwnerCharacter->GetPackID() != NAME_None && !OwnerCharacter->IsPackLeader()) return;

	PatrolOrigin = OwnerCharacter->GetActorLocation();

	if (PatrolMode == EPatrolMode::Path)
	{
		CacheSplineWorldPositions();

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

bool UAIMovementComponent::MoveToLocation(const FVector& Target, float AcceptanceOverride, bool bExactGoal)
{
	EnsureInitialized();
	if (!OwnerCharacter) return false;

	if (MovementComp)
	{
		if (!MovementComp->IsActive()) MovementComp->Activate(true);
		if (MovementComp->MovementMode == MOVE_None) MovementComp->SetMovementMode(MOVE_Walking);
	}

	const float Accept = (AcceptanceOverride >= 0.f) ? AcceptanceOverride : AcceptanceRadius;

	if (bMoveRequestActive && OwnerCharacter)
		if (const AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
			if (AIC->GetMoveStatus() == EPathFollowingStatus::Idle)
				bMoveRequestActive = false;

	if (bMoveRequestActive && bHasLastGoal
		&& FVector::DistSquared(Target, LastRequestedGoal) < FMath::Square(RepathTolerance)
		&& (RepathCooldown > 0.f || RepathStaleTimer > 0.f))
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
		CurrentMoveGoalActor = nullptr;

		const EPathFollowingRequestResult::Type Result = AIC->MoveToLocation(
			Target, Accept, !bExactGoal, true, true, true,
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
			RepathStaleTimer = RepathMaxStale;
		}
		else
		{
			bMoveRequestActive = false;
			bHasLastGoal = false;
		}
	}
	return bSuccess;
}

bool UAIMovementComponent::MoveToActorDirect(AActor* Goal, float InAcceptanceRadius)
{
	EnsureInitialized();
	if (!OwnerCharacter || !Goal) return false;

	if (MovementComp)
	{
		if (!MovementComp->IsActive()) MovementComp->Activate(true);
		if (MovementComp->MovementMode == MOVE_None) MovementComp->SetMovementMode(MOVE_Walking);
	}

	AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController());
	if (!AIC) return false;

	CurrentDestination = Goal->GetActorLocation();

	float Accept = InAcceptanceRadius;
	if (const ACharacter* GoalChar = Cast<ACharacter>(Goal))
		if (const UCapsuleComponent* GoalCap = GoalChar->GetCapsuleComponent())
			Accept = FMath::Max(InAcceptanceRadius - GoalCap->GetScaledCapsuleRadius(), 40.f);

	if (CurrentMoveGoalActor.Get() == Goal
		&& FMath::IsNearlyEqual(CurrentMoveGoalAcceptance, Accept, 1.f)
		&& AIC->GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		OnMovementTargetUpdated.Broadcast(CurrentDestination);
		return true;
	}

	const EPathFollowingRequestResult::Type Result = AIC->MoveToActor(
		Goal, Accept, false, true, true,
		TSubclassOf<UNavigationQueryFilter>(), true);

	const bool bMoving = (Result == EPathFollowingRequestResult::RequestSuccessful);
	CurrentMoveGoalActor = bMoving ? Goal : nullptr;
	CurrentMoveGoalAcceptance = Accept;
	bMoveRequestActive = bMoving;
	bHasLastGoal = false;
	OnMovementTargetUpdated.Broadcast(CurrentDestination);
	return bMoving || Result == EPathFollowingRequestResult::AlreadyAtGoal;
}

void UAIMovementComponent::CancelPathMove()
{
	EnsureInitialized();
	if (OwnerCharacter)
		if (AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
			if (AIC->GetMoveStatus() != EPathFollowingStatus::Idle)
				AIC->StopMovement();
	bMoveRequestActive = false;
	bHasLastGoal = false;
	CurrentMoveGoalActor = nullptr;
}

bool UAIMovementComponent::IsPathMoveActive() const
{
	if (!OwnerCharacter) return false;
	if (const AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
		return AIC->GetMoveStatus() != EPathFollowingStatus::Idle;
	return false;
}

void UAIMovementComponent::StopMovement()
{
	DesiredMaxSpeed = 0.f;
	bMoveRequestActive = false;
	bHasLastGoal = false;
	CurrentMoveGoalActor = nullptr;
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

	if (!bFleeScatterRolled) { FleeScatterAngle = FMath::FRandRange(-FleeScatterSpread, FleeScatterSpread); bFleeScatterRolled = true; }
	AwayDir = AwayDir.RotateAngleAxis(FleeScatterAngle, FVector::UpVector);

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const float CurDistSq = FVector::DistSquared2D(MyLoc, ThreatLoc);

	static const float Angles[] = { 0.f, -25.f, 25.f, -50.f, 50.f, -75.f, 75.f, -90.f, 90.f };
	FVector BestTarget = MyLoc + AwayDir * FleeDistance;
	float BestScore = TNumericLimits<float>::Lowest();
	bool bFound = false;
	const FVector CurHeading = OwnerCharacter->GetVelocity().GetSafeNormal2D();

	const float TurnBias = FleeTurnCommitment * FleeDistance;
	for (const float Ang : Angles)
	{
		const FVector CandDir = AwayDir.RotateAngleAxis(Ang, FVector::UpVector);
		FVector Cand = MyLoc + CandDir * FleeDistance;
		if (NavSys)
		{
			FNavLocation NavRes;
			if (!NavSys->ProjectPointToNavigation(Cand, NavRes, FVector(400.f, 400.f, 250.f))) continue;
			Cand = NavRes.Location;
		}
		const float DistSq = FVector::DistSquared2D(Cand, ThreatLoc);
		if (DistSq <= CurDistSq) continue;
		float Score = FMath::Sqrt(DistSq);
		if (!CurHeading.IsNearlyZero()) Score += FVector::DotProduct(CandDir, CurHeading) * TurnBias;
		if (Score > BestScore) { BestScore = Score; BestTarget = Cand; bFound = true; }
	}

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

	FNavLocation Here;
	const bool bNavUsableHere = NavSys && NavSys->ProjectPointToNavigation(MyLoc, Here, FVector(200.f, 200.f, 300.f));
	if (bNavUsableHere) return false;

	if (AAIController* AIC = Cast<AAIController>(OwnerCharacter->GetController()))
	{
		DesiredMaxSpeed = FleeSpeed;
		if (MovementComp) MovementComp->MaxWalkSpeed = FleeSpeed;
		CurrentDestination = MyLoc + AwayDir * FleeDistance;
		OnMovementTargetUpdated.Broadcast(CurrentDestination);
		AIC->MoveToLocation(CurrentDestination, AcceptanceRadius, true,
			false, false, true,
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

void UAIMovementComponent::HandlePatrolTick(float DeltaTime)
{
	if (bPatrolFinished) return;

	if (bIsWaiting)
	{
		WaitTimer -= DeltaTime;

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

	if (HasReachedDestination())
	{
		PatrolRetryCount = 0;
		BeginWaitAtPoint();
		return;
	}

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

	ActivityRepeatTimer = ActivityRepeatInterval;
	if (ActivityChance > 0.f && FMath::FRand() < ActivityChance && OwnerCharacter)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->PlayRandomActivity();
}

void UAIMovementComponent::ResumePatrolAfterWait()
{
	bIsWaiting = false;
	AdvancePatrolIndex();
	if (bPatrolFinished) return;

	if (OwnerCharacter)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->StopCurrentAction();

	CurrentDestination = GetNextPatrolPoint();
	DesiredMaxSpeed = PatrolSpeed;
	if (MovementComp) MovementComp->MaxWalkSpeed = PatrolSpeed;

	if (!MoveToLocation(CurrentDestination))
		BeginWaitAtPoint();
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

	if (MovementComp->bOrientRotationToMovement)
	{
		const float SpeedFrac = FMath::Clamp(MovementComp->Velocity.Size2D() / FMath::Max(ChaseSpeed, 1.f), 0.f, 1.f);
		MovementComp->RotationRate.Yaw = FMath::Lerp(LowSpeedRotationRate, MovementRotationRate, SpeedFrac);
	}

	if (!FMath::IsNearlyEqual(MovementComp->MaxWalkSpeed, DesiredMaxSpeed, 0.5f))
		MovementComp->MaxWalkSpeed = FMath::FInterpTo(MovementComp->MaxWalkSpeed, DesiredMaxSpeed, DeltaTime, AccelerationInterpSpeed);
}

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
