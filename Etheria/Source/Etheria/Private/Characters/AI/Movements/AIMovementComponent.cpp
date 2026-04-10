/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Class: AIMovementComponent - Source
 */

#include "Characters/AI/Movements/AIMovementComponent.h"

#include "Characters/AI/BaseAICharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "AIController.h"

UAIMovementComponent::UAIMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAIMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ABaseAICharacter>(GetOwner());
	if (OwnerCharacter)
	{
		MovementComp = OwnerCharacter->GetCharacterMovement();
	}
}

void UAIMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ApplySmoothAcceleration(DeltaTime);

	if (bIsPatrolling)
	{
		HandlePatrolTick(DeltaTime);
	}
}

/* ─────────────────── Public API ─────────────────── */

void UAIMovementComponent::StartPatrol()
{
	if (PatrolMode == EPatrolMode::Stationary) return;

	bIsPatrolling = true;
	bPatrolFinished = false;
	bIsWaiting = false;
	CurrentPatrolIndex = 0;
	PatrolDirection = 1;
	SetDesiredSpeed(PatrolSpeed);

	CurrentDestination = GetNextPatrolPoint();
	MoveToLocation(CurrentDestination);
}

void UAIMovementComponent::StopPatrol()
{
	bIsPatrolling = false;
	bIsWaiting = false;
	StopMovement();
}

void UAIMovementComponent::MoveToLocation(const FVector& Target)
{
	CurrentDestination = Target;
	OnMovementTargetUpdated.Broadcast(Target);

	if (AAIController* AIC = Cast<AAIController>(OwnerCharacter ? OwnerCharacter->GetController() : nullptr))
	{
		AIC->MoveToLocation(Target, AcceptanceRadius, true, true, false, true);
	}
}

void UAIMovementComponent::StopMovement()
{
	SetDesiredSpeed(0.f);

	if (AAIController* AIC = Cast<AAIController>(OwnerCharacter ? OwnerCharacter->GetController() : nullptr))
	{
		AIC->StopMovement();
	}
}

FVector UAIMovementComponent::GetNextPatrolPoint()
{
	if (PatrolMode == EPatrolMode::Zone)
	{
		return GetRandomPointInZone();
	}

	if (PatrolMode == EPatrolMode::Path && PatrolPoints.Num() > 0)
	{
		return PatrolPoints[FMath::Clamp(CurrentPatrolIndex, 0, PatrolPoints.Num() - 1)];
	}

	return OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;
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

/* ─────────────────── Private ─────────────────── */

void UAIMovementComponent::HandlePatrolTick(float DeltaTime)
{
	if (bPatrolFinished) return;

	// Waiting at point
	if (bIsWaiting)
	{
		WaitTimer -= DeltaTime;
		if (WaitTimer <= 0.f)
		{
			bIsWaiting = false;
			AdvancePatrolIndex();
			CurrentDestination = GetNextPatrolPoint();
			SetDesiredSpeed(PatrolSpeed);
			MoveToLocation(CurrentDestination);
		}
		return;
	}

	// Check arrival
	if (HasReachedDestination())
	{
		OnPatrolPointReached.Broadcast(CurrentPatrolIndex);
		StopMovement();
		bIsWaiting = true;
		WaitTimer = WaitTimeAtPoint + FMath::FRandRange(0.f, WaitTimeRandomDeviation);
	}
}

void UAIMovementComponent::AdvancePatrolIndex()
{
	if (PatrolMode == EPatrolMode::Zone)
	{
		// Zone mode: no index to advance, just pick a new random point
		return;
	}

	if (PatrolPoints.Num() == 0) return;

	const int32 NextIndex = CurrentPatrolIndex + PatrolDirection;

	switch (PatrolLoopMode)
	{
	case EPatrolLoopMode::Loop:
		CurrentPatrolIndex = NextIndex % PatrolPoints.Num();
		if (CurrentPatrolIndex < 0) CurrentPatrolIndex += PatrolPoints.Num();
		break;

	case EPatrolLoopMode::PingPong:
		if (NextIndex >= PatrolPoints.Num() || NextIndex < 0)
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
		if (NextIndex >= PatrolPoints.Num())
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
	if (NavSys->GetRandomReachablePointInRadius(OwnerCharacter->GetActorLocation(), PatrolRadius, Result))
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
