/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BaseAI - Source
*/

#include "Characters/AI/BaseAI.h"
#include "Perception/AIPerceptionComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/TargetPoint.h"
#include "AIController.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/CharacterMovementComponent.h"

ABaseAI::ABaseAI()
{
    PrimaryActorTick.bCanEverTick = true;

    AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 2000.f;
    SightConfig->LoseSightRadius = 2200.f;
    SightConfig->PeripheralVisionAngleDegrees = 180.f;
    SightConfig->SetMaxAge(5.f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    AIPerception->ConfigureSense(*SightConfig);
    AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());

    IdleSpline = CreateDefaultSubobject<USplineComponent>(TEXT("IdleSpline"));

    IdleMoveType = EAIIdleMoveType::Spline;
    SplinePatrolMode = ESplinePatrolMode::Loop;
    SplinePointReachDist = 100.f;
    SplineOffset = 0.f;
    bSplineActive = false;
    SplineDirection = 1;

    GetCharacterMovement()->MaxWalkSpeed = 300.f;
}

void ABaseAI::BeginPlay()
{
    Super::BeginPlay();

    if (IdleMoveType == EAIIdleMoveType::Spline && IdleSpline)
    {
        InitSplineWaypoints(SplinePointReachDist);
        bSplineActive = true;
        MoveToCurrentSplineWaypoint();
    }
}

void ABaseAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    HandlePerception();
    HandleDecisionMaking();

    if (!CanIdleMove()) return;

    switch (IdleMoveType)
    {
        case EAIIdleMoveType::Points:
            MoveToIdlePoint(DeltaTime);
            break;

        case EAIIdleMoveType::Spline:
            UpdateSplineMove(DeltaTime);
            break;

        default:
            break;
    }
}

void ABaseAI::InitSplineWaypoints(float Step)
{
    SplineWaypoints.Empty();
    if (!IdleSpline) return;

    float Length = IdleSpline->GetSplineLength();
    for (float d = 0; d < Length; d += Step)
        SplineWaypoints.Add(d);

    SplineWaypoints.Add(Length);
    CurrentWaypointIndex = 0;
    SplineDirection = 1;
}

void ABaseAI::MoveToCurrentSplineWaypoint()
{
    if (!IdleSpline || !bSplineActive || SplineWaypoints.Num() == 0) return;

    float TargetDist = SplineWaypoints[CurrentWaypointIndex];
    SplineCurrentTarget = IdleSpline->GetLocationAtDistanceAlongSpline(TargetDist + SplineOffset, ESplineCoordinateSpace::World);
}

void ABaseAI::UpdateSplineMove(float DeltaTime)
{
    if (!IdleSpline || !bSplineActive || SplineWaypoints.Num() == 0) return;

    FVector TargetLocation = SplineCurrentTarget;
    MoveTowards(TargetLocation, DeltaTime);

    if (FVector::Dist(GetActorLocation(), TargetLocation) < SplinePointReachDist)
    {
        CurrentWaypointIndex += SplineDirection;

        if (SplinePatrolMode == ESplinePatrolMode::Loop)
        {
            if (CurrentWaypointIndex >= SplineWaypoints.Num())
                CurrentWaypointIndex = 0;
            else if (CurrentWaypointIndex < 0)
                CurrentWaypointIndex = SplineWaypoints.Num() - 1;
        }
        else if (SplinePatrolMode == ESplinePatrolMode::BackAndForth)
        {
            if (CurrentWaypointIndex >= SplineWaypoints.Num())
            {
                CurrentWaypointIndex = SplineWaypoints.Num() - 2;
                SplineDirection = -1;
            }
            else if (CurrentWaypointIndex < 0)
            {
                CurrentWaypointIndex = 1;
                SplineDirection = 1;
            }
        }

        MoveToCurrentSplineWaypoint();
        PlayIdleVoiceLine();
    }
}

void ABaseAI::MoveToIdlePoint(float DeltaTime)
{
    if (IdlePoints.Num() == 0 || !IdlePoints.IsValidIndex(CurrentIdlePointIndex) || !IdlePoints[CurrentIdlePointIndex]) 
        return;

    FVector TargetLocation = IdlePoints[CurrentIdlePointIndex]->GetActorLocation();
    MoveTowards(TargetLocation, DeltaTime);

    if (FVector::Dist(GetActorLocation(), TargetLocation) < SplinePointReachDist)
    {
        CurrentIdlePointIndex = (CurrentIdlePointIndex + 1) % IdlePoints.Num();
        PlayIdleVoiceLine();
    }
}

void ABaseAI::MoveTowards(const FVector& TargetLocation, float DeltaTime)
{
    FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal();
    FVector DesiredVelocity = Direction * GetCharacterMovement()->MaxWalkSpeed;

    GetCharacterMovement()->Velocity = DesiredVelocity;

    FRotator TargetRotation = Direction.Rotation();
    SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 5.f));
}

void ABaseAI::HandlePerception() {}
void ABaseAI::HandleDecisionMaking() {}
