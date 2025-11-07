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
#include "Kismet/KismetSystemLibrary.h"
#include "Perception/AISenseConfig_Sight.h"

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

    if (CanIdleMove()) {
        switch(IdleMoveType)
        {
        case EAIIdleMoveType::Points:
            MoveToIdlePoint();
            break;
        case EAIIdleMoveType::Spline:
            UpdateSplineMove(DeltaTime);
            break;
        default:
            break;
        }
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
    if (AAIController* AICon = Cast<AAIController>(GetController()))
        AICon->MoveToLocation(SplineCurrentTarget);
}

void ABaseAI::UpdateSplineMove(float DeltaTime)
{
    if (!IdleSpline || !bSplineActive || SplineWaypoints.Num() == 0) return;

    FVector ActorLocation = GetActorLocation();
    float DistToTarget = FVector::Dist(ActorLocation, SplineCurrentTarget);

    if (DistToTarget < SplinePointReachDist)
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
        PlayIdleAnimation();
        PlayIdleVoiceLine();
    }
}

void ABaseAI::HandlePerception() {}
void ABaseAI::HandleDecisionMaking() {}

void ABaseAI::MoveToIdlePoint()
{
    if (IdlePoints.Num() == 0 || !IdlePoints.IsValidIndex(CurrentIdlePointIndex) || !IdlePoints[CurrentIdlePointIndex]) 
        return;

    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        FVector TargetLocation = IdlePoints[CurrentIdlePointIndex]->GetActorLocation();
        AICon->MoveToActor(IdlePoints[CurrentIdlePointIndex]);

        CurrentIdlePointIndex = (CurrentIdlePointIndex + 1) % IdlePoints.Num();

        PlayIdleAnimation();
        PlayIdleVoiceLine();
    }
}
