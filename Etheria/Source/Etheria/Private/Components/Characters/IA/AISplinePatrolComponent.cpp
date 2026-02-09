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
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"
#include "Navigation/PathFollowingComponent.h"

UAISplinePatrolComponent::UAISplinePatrolComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAISplinePatrolComponent::BeginPlay()
{
    Super::BeginPlay();

    CurrentIndex            = 0;
    Direction               = 1;
    bIsMovingToPoint        = false;
    CurrentDistanceOnSpline = 0.f;
    bFollowPaused           = false;
    LastPassedPointIndex    = INDEX_NONE;
    NextPointIndex          = 0;
    bReturningToSpline      = false;
    bIsChasing              = false;

    if (SplinePath)
    {
        if (USplineComponent* Spline = SplinePath->GetSpline())
        {
            const int32 NumPoints = Spline->GetNumberOfSplinePoints();

            if (NumPoints > 0 && WaitTimesPerPoint.Num() < NumPoints)
            {
                WaitTimesPerPoint.SetNum(NumPoints);
                for (int32 i = 0; i < NumPoints; ++i)
                {
                    if (WaitTimesPerPoint[i] <= 0.f)
                    {
                        WaitTimesPerPoint[i] = DefaultWaitTimeAtPoint;
                    }
                }
            }
        }
    }
}

void UAISplinePatrolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!SplinePath) return;

    if (FollowMode == ESplineFollowMode::FollowSpline)
    {
        if (!bIsChasing && !bReturningToSpline)
        {
            UpdateFollowSpline(DeltaTime);
        }
    }
}

void UAISplinePatrolComponent::StartPatrol()
{
    if (FollowMode == ESplineFollowMode::Points)
    {
        MoveToNextPoint();
    }
    else
    {
        InitFollowSplineStart();
    }
}

void UAISplinePatrolComponent::MoveToNextPoint()
{
    if (!SplinePath) return;

    if (FollowMode == ESplineFollowMode::Points)
    {
        MoveToNextPoint_PointsMode();
    }
    else
    {
        InitFollowSplineStart();
    }
}

float UAISplinePatrolComponent::GetWaitTimeForPointIndex(int32 PointIndex) const
{
    if (!SplinePath)
        return DefaultWaitTimeAtPoint;

    USplineComponent* Spline = SplinePath->GetSpline();
    if (!Spline)
        return DefaultWaitTimeAtPoint;

    const int32 NumPoints = Spline->GetNumberOfSplinePoints();
    if (NumPoints <= 0)
        return DefaultWaitTimeAtPoint;

    const int32 SafeIndex = FMath::Clamp(PointIndex, 0, NumPoints - 1);

    if (WaitTimesPerPoint.IsValidIndex(SafeIndex))
    {
        return WaitTimesPerPoint[SafeIndex];
    }

    return DefaultWaitTimeAtPoint;
}

float UAISplinePatrolComponent::GetWaitTimeForCurrentPoint() const
{
    return GetWaitTimeForPointIndex(CurrentIndex);
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

void UAISplinePatrolComponent::MoveToNextPoint_PointsMode()
{
    USplineComponent* Spline = SplinePath->GetSpline();
    if (!Spline) return;

    const int32 NumPoints = Spline->GetNumberOfSplinePoints();
    if (NumPoints == 0) return;

    FVector Dest = Spline->GetLocationAtSplinePoint(CurrentIndex, ESplineCoordinateSpace::World);

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
            Request.SetUsePathfinding(true);

            bIsMovingToPoint = true;
            AIController->MoveTo(Request);
        }
    }

    DrawDebugSphere(GetWorld(), Dest, 50.f, 12, FColor::Red, false, 3.f);
}

void UAISplinePatrolComponent::InitFollowSplineStart()
{
    USplineComponent* Spline = SplinePath->GetSpline();
    if (!Spline) return;

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    const float InputKey = Spline->FindInputKeyClosestToWorldLocation(OwnerPawn->GetActorLocation());
    CurrentDistanceOnSpline = Spline->GetDistanceAlongSplineAtSplineInputKey(InputKey);

    float ClosestDistSq = FLT_MAX;
    int32 ClosestIndex  = 0;
    const int32 NumPoints = Spline->GetNumberOfSplinePoints();
    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector PointLoc = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        const float DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), PointLoc);
        if (DistSq < ClosestDistSq)
        {
            ClosestDistSq = DistSq;
            ClosestIndex  = i;
        }
    }

    LastPassedPointIndex = ClosestIndex;

    if (PatrolMode == ESplinePatrolMode::Loop)
    {
        NextPointIndex = (ClosestIndex + 1) % NumPoints;
    }
    else
    {
        Direction      = 1;
        NextPointIndex = FMath::Clamp(ClosestIndex + Direction, 0, NumPoints - 1);
    }

    bFollowPaused = false;
}

void UAISplinePatrolComponent::UpdateFollowSpline(float DeltaTime)
{
    if (bFollowPaused) return;

    USplineComponent* Spline = SplinePath->GetSpline();
    if (!Spline) return;

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    const float SplineLength = Spline->GetSplineLength();
    if (SplineLength <= 0.f) return;

    const float DistanceStep = FollowSpeed * DeltaTime;
    if (DistanceStep <= 0.f) return;

    static bool bForward = true;

    if (PatrolMode == ESplinePatrolMode::Loop)
    {
        CurrentDistanceOnSpline = FMath::Fmod(CurrentDistanceOnSpline + DistanceStep, SplineLength);
        if (CurrentDistanceOnSpline < 0.f)
        {
            CurrentDistanceOnSpline += SplineLength;
        }
    }
    else
    {
        if (bForward)
        {
            CurrentDistanceOnSpline += DistanceStep;
            if (CurrentDistanceOnSpline > SplineLength)
            {
                CurrentDistanceOnSpline = SplineLength;
                bForward = false;
            }
        }
        else
        {
            CurrentDistanceOnSpline -= DistanceStep;
            if (CurrentDistanceOnSpline < 0.f)
            {
                CurrentDistanceOnSpline = 0.f;
                bForward = true;
            }
        }
    }

    FVector TargetLocation = Spline->GetLocationAtDistanceAlongSpline(CurrentDistanceOnSpline, ESplineCoordinateSpace::World);

    if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation ProjectedDest;
        if (NavSys->ProjectPointToNavigation(TargetLocation, ProjectedDest, FVector(100.f,100.f,300.f)))
        {
            TargetLocation = ProjectedDest.Location;
        }
    }

    if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
    {
        if (UCapsuleComponent* Capsule = Char->GetCapsuleComponent())
        {
            const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
            TargetLocation.Z += HalfHeight * 0.9f;
        }

        FHitResult Hit;
        Char->SetActorLocation(TargetLocation, true, &Hit, ETeleportType::TeleportPhysics);
    }
    else if (APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        FHitResult Hit;
        Pawn->SetActorLocation(TargetLocation, true, &Hit, ETeleportType::TeleportPhysics);
    }
    else
    {
        GetOwner()->SetActorLocation(TargetLocation);
    }

    FVector TangentDir = Spline->GetDirectionAtDistanceAlongSpline(CurrentDistanceOnSpline, ESplineCoordinateSpace::World);
    FRotator LookAt = TangentDir.Rotation();
    GetOwner()->SetActorRotation(FRotator(0.f, LookAt.Yaw, 0.f));

    USplineComponent* SplineComp = SplinePath->GetSpline();
    const int32 NumPoints = SplineComp->GetNumberOfSplinePoints();
    if (NumPoints > 0 && NextPointIndex >= 0 && NextPointIndex < NumPoints)
    {
        FVector NextPointLoc = SplineComp->GetLocationAtSplinePoint(NextPointIndex, ESplineCoordinateSpace::World);
        float   DistToNext   = FVector::Dist(GetOwner()->GetActorLocation(), NextPointLoc);

        if (DistToNext <= PointProximityRadius)
        {
            PauseFollowAtPoint(NextPointIndex);
            LastPassedPointIndex = NextPointIndex;

            if (PatrolMode == ESplinePatrolMode::Loop)
            {
                NextPointIndex = (NextPointIndex + 1) % NumPoints;
            }
            else
            {
                static int32 DirIndex = 1;
                NextPointIndex += DirIndex;
                if (NextPointIndex >= NumPoints)
                {
                    NextPointIndex = NumPoints - 2;
                    DirIndex = -1;
                }
                else if (NextPointIndex < 0)
                {
                    NextPointIndex = 1;
                    DirIndex = 1;
                }
            }
        }
    }

    DrawDebugSphere(GetWorld(), TargetLocation, 20.f, 8, FColor::Green, false, 0.f);
}

void UAISplinePatrolComponent::PauseFollowAtPoint(int32 PointIndex)
{
    const float WaitTime = GetWaitTimeForPointIndex(PointIndex);

    if (WaitTime <= 0.f)
    {
        return;
    }

    bFollowPaused = true;

    GetWorld()->GetTimerManager().SetTimer(
        PatrolTimerHandle,
        [this]()
        {
            bFollowPaused = false;
        },
        WaitTime,
        false
    );
}

void UAISplinePatrolComponent::OnMoveCompleted()
{
    bIsMovingToPoint = false;

    if (bReturningToSpline)
    {
        bReturningToSpline = false;

        if (FollowMode == ESplineFollowMode::FollowSpline)
        {
            bFollowPaused = false;
            return;
        }
    }

    if (FollowMode == ESplineFollowMode::Points)
    {
        AdvanceIndex();

        const float WaitTime = GetWaitTimeForCurrentPoint();

        if (WaitTime <= 0.f)
        {
            MoveToNextPoint();
        }
        else
        {
            GetWorld()->GetTimerManager().SetTimer(
                PatrolTimerHandle,
                this,
                &UAISplinePatrolComponent::MoveToNextPoint,
                WaitTime,
                false
            );
        }
    }
}

void UAISplinePatrolComponent::RequestReturnToSpline()
{
    if (!SplinePath) return;

    USplineComponent* Spline = SplinePath->GetSpline();
    if (!Spline) return;

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    float ClosestDistSq = FLT_MAX;
    int32 ClosestIndex  = 0;
    const int32 NumPoints = Spline->GetNumberOfSplinePoints();

    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector PointLoc = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        float   DistSq   = FVector::DistSquared(OwnerPawn->GetActorLocation(), PointLoc);
        if (DistSq < ClosestDistSq)
        {
            ClosestDistSq = DistSq;
            ClosestIndex  = i;
        }
    }

    FVector Dest = Spline->GetLocationAtSplinePoint(ClosestIndex, ESplineCoordinateSpace::World);

    if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation ProjectedDest;
        if (NavSys->ProjectPointToNavigation(Dest, ProjectedDest, FVector(100.f,100.f,300.f)))
        {
            Dest = ProjectedDest.Location;
        }
    }

    if (AAIController_Base* AIController = Cast<AAIController_Base>(OwnerPawn->GetController()))
    {
        FAIMoveRequest Request;
        Request.SetGoalLocation(Dest);
        Request.SetAcceptanceRadius(50.f);
        Request.SetUsePathfinding(true);

        bReturningToSpline = true;
        bIsMovingToPoint   = true;
        AIController->MoveTo(Request);

        CurrentIndex         = ClosestIndex;
        LastPassedPointIndex = ClosestIndex;
        NextPointIndex       = (ClosestIndex + 1) % NumPoints;

        const float InputKey = Spline->FindInputKeyClosestToWorldLocation(Dest);
        CurrentDistanceOnSpline = Spline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
    }
}

void UAISplinePatrolComponent::SnapToClosestPoint()
{
    if (!SplinePath) return;
    USplineComponent* Spline = SplinePath->GetSpline();
    if (!Spline) return;

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    float ClosestDistance = FLT_MAX;
    int32 ClosestIndex    = 0;

    const int32 NumPoints = Spline->GetNumberOfSplinePoints();
    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector PointLoc = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        float   Dist     = FVector::Dist(OwnerPawn->GetActorLocation(), PointLoc);
        if (Dist < ClosestDistance)
        {
            ClosestDistance = Dist;
            ClosestIndex    = i;
        }
    }

    CurrentIndex         = ClosestIndex;
    LastPassedPointIndex = ClosestIndex;

    if (PatrolMode == ESplinePatrolMode::BackAndForth)
    {
        if (CurrentIndex == 0)
            Direction = 1;
        else if (CurrentIndex == NumPoints - 1)
            Direction = -1;
    }
}
