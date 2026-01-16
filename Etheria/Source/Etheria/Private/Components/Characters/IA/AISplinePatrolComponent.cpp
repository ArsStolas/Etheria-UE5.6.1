/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AISplinePatrolComponent - Source
*/

#include "Components/Characters/IA/AISplinePatrolComponent.h"
#include "Characters/AI/Controllers/AIController_Base.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

void UAISplinePatrolComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentIndex = 0;
	Direction = 1;
	bIsMovingToPoint = false;
}

void UAISplinePatrolComponent::StartPatrol()
{
	MoveToNextPoint();
}

void UAISplinePatrolComponent::AdvanceIndex()
{
	if (!SplinePath) return;
	USplineComponent* Spline = SplinePath->GetSpline();
	if (!Spline) return;

	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	if (NumPoints == 0) return;

	const int32 MaxIndex = NumPoints - 1;

	if (PatrolMode == ESplinePatrolMode::Loop)
	{
		CurrentIndex = (CurrentIndex + 1) % NumPoints;
	}
	else
	{
		CurrentIndex += Direction;
		if (CurrentIndex >= MaxIndex)
		{
			CurrentIndex = MaxIndex;
			Direction = -1;
		}
		else if (CurrentIndex <= 0)
		{
			CurrentIndex = 0;
			Direction = 1;
		}
	}
}

void UAISplinePatrolComponent::MoveToNextPoint()
{
	if (!SplinePath || bIsMovingToPoint) return; // si déjà en mouvement, on ne fait rien
	USplineComponent* Spline = SplinePath->GetSpline();
	if (!Spline) return;

	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	if (NumPoints == 0) return;

	FVector Dest = Spline->GetLocationAtSplinePoint(CurrentIndex, ESplineCoordinateSpace::World);

	// Projeté sur le navmesh
	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedDest;
		if (NavSys->ProjectPointToNavigation(Dest, ProjectedDest, FVector(50.f,50.f,200.f)))
		{
			Dest = ProjectedDest.Location;
		}
	}

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (AAIController_Base* AIController = Cast<AAIController_Base>(OwnerPawn->GetController()))
		{
			FRotator LookAt = (Dest - OwnerPawn->GetActorLocation()).Rotation();
			OwnerPawn->SetActorRotation(FRotator(0.f, LookAt.Yaw, 0.f));

			FAIMoveRequest Request;
			Request.SetGoalLocation(Dest);
			Request.SetAcceptanceRadius(50.f);

			bIsMovingToPoint = true;
			AIController->MoveTo(Request);
		}
	}

	DrawDebugSphere(GetWorld(), Dest, 50.f, 12, FColor::Red, false, 5.f);
}

void UAISplinePatrolComponent::SnapToClosestPoint()
{
	if (!SplinePath) return;
	USplineComponent* Spline = SplinePath->GetSpline();
	if (!Spline) return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	float ClosestDistance = FLT_MAX;
	int32 ClosestIndex = 0;

	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	for (int32 i = 0; i < NumPoints; ++i)
	{
		FVector PointLoc = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		float Dist = FVector::Dist(OwnerPawn->GetActorLocation(), PointLoc);
		if (Dist < ClosestDistance)
		{
			ClosestDistance = Dist;
			ClosestIndex = i;
		}
	}

	CurrentIndex = ClosestIndex;

	if (PatrolMode == ESplinePatrolMode::BackAndForth)
	{
		if (CurrentIndex == 0)
			Direction = 1;
		else if (CurrentIndex == NumPoints - 1)
			Direction = -1;
	}
}
