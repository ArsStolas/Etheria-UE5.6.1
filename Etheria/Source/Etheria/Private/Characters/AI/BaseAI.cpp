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

ABaseAI::ABaseAI()
{
    PrimaryActorTick.bCanEverTick = true;
    AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    IdleSpline = CreateDefaultSubobject<USplineComponent>(TEXT("IdleSpline"));

    IdlePoints.SetNum(4);
    for (int i = 0; i < 4; ++i)
    {
        FString PointName = FString::Printf(TEXT("IdlePoint%d"), i);
        ATargetPoint* IdlePoint = CreateDefaultSubobject<ATargetPoint>(*PointName);
        IdlePoints[i] = IdlePoint;
    }
}

void ABaseAI::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("BaseAI::BeginPlay called"));

    if (IdleMoveType == EAIIdleMoveType::Spline && IdleSpline)
    {
        CurrentSplineProgress = 0.f;
        SplineDirection = 1;
        SplineCurrentTarget = IdleSpline->GetLocationAtDistanceAlongSpline(CurrentSplineProgress + SplineOffset, ESplineCoordinateSpace::World);
        bSplineActive = true;
        UE_LOG(LogTemp, Warning, TEXT("BaseAI: Spline mode enabled, first target set to %s"), *SplineCurrentTarget.ToString());

        if (AAIController* AICon = Cast<AAIController>(GetController()))
        {
            UE_LOG(LogTemp, Warning, TEXT("BaseAI: MoveToLocation on spline first target"));
            AICon->MoveToLocation(SplineCurrentTarget);
        }
    }
}

void ABaseAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    HandlePerception();
    HandleDecisionMaking();

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

void ABaseAI::HandlePerception()
{
    UE_LOG(LogTemp, Warning, TEXT("BaseAI::HandlePerception called"));
}

void ABaseAI::HandleDecisionMaking()
{
    UE_LOG(LogTemp, Warning, TEXT("BaseAI::HandleDecisionMaking called"));
}

void ABaseAI::MoveToIdlePoint()
{
    UE_LOG(LogTemp, Warning, TEXT("BaseAI::MoveToIdlePoint called"));
    if (IdlePoints.Num() == 0 || !IdlePoints[CurrentIdlePointIndex]) return;
    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        UE_LOG(LogTemp, Warning, TEXT("Moving to IdlePoint %d at %s"), CurrentIdlePointIndex, *IdlePoints[CurrentIdlePointIndex]->GetActorLocation().ToString());
        AICon->MoveToActor(IdlePoints[CurrentIdlePointIndex]);
    }
    CurrentIdlePointIndex = (CurrentIdlePointIndex + 1) % IdlePoints.Num();

    PlayIdleAnimation();
    PlayIdleVoiceLine();
}

void ABaseAI::UpdateSplineMove(float DeltaTime)
{
    if (!IdleSpline || !bSplineActive) return;

    float Dist = FVector::Dist(GetActorLocation(), SplineCurrentTarget);

    UE_LOG(LogTemp, Warning, TEXT("UpdateSplineMove: SplineCurrentTarget=%s, ActorLocation=%s, Dist=%.1f"), 
        *SplineCurrentTarget.ToString(), *GetActorLocation().ToString(), Dist);

    AAIController* AICon = Cast<AAIController>(GetController());
    if (!AICon) {
        UE_LOG(LogTemp, Error, TEXT("BaseAI: No AIController detected! Check Pawn AutoPossessAI and ControllerClass."));
        return;
    }

    if (Dist < SplinePointReachDist || Dist > 10000.f)
    {
        float SplineLength = IdleSpline->GetSplineLength();
        CurrentSplineProgress += SplineFollowSpeed * DeltaTime * SplineDirection;

        if (SplinePatrolMode == ESplinePatrolMode::Loop)
        {
            if (CurrentSplineProgress > SplineLength) CurrentSplineProgress = 0.f;
            if (CurrentSplineProgress < 0.f) CurrentSplineProgress = SplineLength;
        }
        else if (SplinePatrolMode == ESplinePatrolMode::BackAndForth)
        {
            if (CurrentSplineProgress > SplineLength) {
                CurrentSplineProgress = SplineLength;
                SplineDirection = -1;
                UE_LOG(LogTemp, Warning, TEXT("Spline reached end. Reverse direction!"));
            }
            else if (CurrentSplineProgress < 0.f) {
                CurrentSplineProgress = 0.f;
                SplineDirection = 1;
                UE_LOG(LogTemp, Warning, TEXT("Spline reached start. Forward direction!"));
            }
        }

        SplineCurrentTarget = IdleSpline->GetLocationAtDistanceAlongSpline(CurrentSplineProgress + SplineOffset, ESplineCoordinateSpace::World);
        EPathFollowingRequestResult::Type Result = AICon->MoveToLocation(SplineCurrentTarget);

        UE_LOG(LogTemp, Warning, TEXT("[Spline] Mode %d | Progress %.2f | Dir %d | Target %s | MoveResult %d"), (int)SplinePatrolMode, CurrentSplineProgress, SplineDirection, *SplineCurrentTarget.ToString(), Result);

        PlayIdleAnimation();
        PlayIdleVoiceLine();
    }
}
