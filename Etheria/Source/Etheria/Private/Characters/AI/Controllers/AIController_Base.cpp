/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AIController_Base - Source
*/

#include "Characters/AI/Controllers/AIController_Base.h"
#include "Characters/AI/BaseAI.h"
#include "Components/Characters/IA/AISplinePatrolComponent.h"
#include "Components/Characters/IA/AIWanderComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavigationData.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

AAIController_Base::AAIController_Base()
{
    PrimaryActorTick.bCanEverTick = true;

    PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SightConfig   = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

    SightConfig->SightRadius = 1500.f;
    SightConfig->LoseSightRadius = 1800.f;
    SightConfig->PeripheralVisionAngleDegrees = 90.f;
    SightConfig->SetMaxAge(5.f);
    SightConfig->DetectionByAffiliation.bDetectEnemies    = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals   = true;

    PerceptionComp->ConfigureSense(*SightConfig);
    PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
    PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AAIController_Base::OnTargetPerceptionUpdated);
}

void AAIController_Base::BeginPlay()
{
    Super::BeginPlay();
}

void AAIController_Base::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ABaseAI* AI = Cast<ABaseAI>(GetPawn());
    if (!AI || !AI->StateComp) return;

    const bool bChasing = (TargetActor != nullptr);
    if (bChasing)
    {
        AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Chase);
        PathUpdateTimer += DeltaTime;
        if (PathUpdateTimer >= PathUpdateInterval)
        {
            PathUpdateTimer = 0.f;
            UpdateChasePath();
        }
    }
    else if (bHasMoveToLocationDestination)
    {
        PathUpdateTimer += DeltaTime;
        if (PathUpdateTimer >= PathUpdateInterval)
        {
            PathUpdateTimer = 0.f;
            UpdateMoveToLocationPath();
        }
    }

    if (bChasing || bHasMoveToLocationDestination)
    {
        const bool bMoveToLocationCompleted = TickMovement(DeltaTime);
        if (bMoveToLocationCompleted)
        {
            bHasMoveToLocationDestination = false;
            NotifyMoveToLocationCompleted();
        }
    }
}

void AAIController_Base::RequestMoveToLocation(const FVector& Destination)
{
    MoveToLocationDestination = Destination;
    bHasMoveToLocationDestination = true;
    PathPoints.Empty();
    PathPointIndex = 0;
    PathUpdateTimer = 0.f;
    UpdateMoveToLocationPath();
}

void AAIController_Base::AbortMoveToLocation()
{
    bHasMoveToLocationDestination = false;
    PathPoints.Empty();
    PathPointIndex = 0;
}

void AAIController_Base::UpdateChasePath()
{
    APawn* P = GetPawn();
    if (!P || !TargetActor) return;

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys) return;

    UNavigationPath* NavPath = NavSys->FindPathToActorSynchronously(
        GetWorld(),
        P->GetActorLocation(),
        TargetActor,
        50.f,
        P,
        nullptr
    );
    if (!NavPath || !NavPath->IsValid()) return;

    FNavPathSharedPtr PathPtr = NavPath->GetPath();
    if (!PathPtr.IsValid()) return;

    const TArray<FNavPathPoint>& Points = PathPtr->GetPathPoints();
    PathPoints.Reset(Points.Num());
    for (const FNavPathPoint& Pt : Points)
    {
        PathPoints.Add(Pt.Location);
    }
    PathPointIndex = 0;
}

void AAIController_Base::UpdateMoveToLocationPath()
{
    APawn* P = GetPawn();
    if (!P || !bHasMoveToLocationDestination) return;

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavSys) return;

    UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(
        GetWorld(),
        P->GetActorLocation(),
        MoveToLocationDestination,
        P,
        nullptr
    );
    if (!NavPath || !NavPath->IsValid()) return;

    FNavPathSharedPtr PathPtr = NavPath->GetPath();
    if (!PathPtr.IsValid()) return;

    const TArray<FNavPathPoint>& Points = PathPtr->GetPathPoints();
    PathPoints.Reset(Points.Num());
    for (const FNavPathPoint& Pt : Points)
    {
        PathPoints.Add(Pt.Location);
    }
    PathPointIndex = 0;
}

bool AAIController_Base::TickMovement(float DeltaTime)
{
    ACharacter* Char = Cast<ACharacter>(GetPawn());
    if (!Char) return false;

    const FVector MyLoc = Char->GetActorLocation();
    const float AcceptRadius = MoveAcceptRadius;

    if (TargetActor)
    {
        if (PathPoints.Num() == 0)
        {
            const FVector ToTarget = TargetActor->GetActorLocation() - MyLoc;
            const float Dist2D = ToTarget.Size2D();
            if (Dist2D > AcceptRadius)
            {
                const FVector Dir = ToTarget.GetSafeNormal2D();
                Char->AddMovementInput(Dir, 1.f);
            }
        }
        else
        {
            PathPointIndex = FMath::Clamp(PathPointIndex, 0, PathPoints.Num() - 1);
            FVector TargetPoint = PathPoints[PathPointIndex];
            while (PathPointIndex < PathPoints.Num() - 1)
            {
                const float DistToPoint = FVector::Dist2D(MyLoc, TargetPoint);
                if (DistToPoint > AcceptRadius) break;
                ++PathPointIndex;
                TargetPoint = PathPoints[PathPointIndex];
            }
            const FVector ToPoint = TargetPoint - MyLoc;
            const float DistToPoint = ToPoint.Size2D();
            if (DistToPoint > 2.f)
            {
                const FVector MoveDir = ToPoint.GetSafeNormal2D();
                Char->AddMovementInput(MoveDir, 1.f);
            }
        }
        return false;
    }

    if (bHasMoveToLocationDestination)
    {
        if (PathPoints.Num() == 0)
        {
            const FVector ToDest = MoveToLocationDestination - MyLoc;
            const float Dist2D = ToDest.Size2D();
            if (Dist2D > AcceptRadius)
            {
                const FVector Dir = ToDest.GetSafeNormal2D();
                Char->AddMovementInput(Dir, 1.f);
            }
            else
            {
                return true;
            }
        }
        else
        {
            PathPointIndex = FMath::Clamp(PathPointIndex, 0, PathPoints.Num() - 1);
            FVector TargetPoint = PathPoints[PathPointIndex];
            while (PathPointIndex < PathPoints.Num() - 1)
            {
                const float DistToPoint = FVector::Dist2D(MyLoc, TargetPoint);
                if (DistToPoint > AcceptRadius) break;
                ++PathPointIndex;
                TargetPoint = PathPoints[PathPointIndex];
            }
            const FVector ToPoint = TargetPoint - MyLoc;
            const float DistToPoint = ToPoint.Size2D();
            if (DistToPoint > 2.f)
            {
                const FVector MoveDir = ToPoint.GetSafeNormal2D();
                Char->AddMovementInput(MoveDir, 1.f);
            }
            else if (PathPointIndex >= PathPoints.Num() - 1)
            {
                const float DistToDest = FVector::Dist2D(MyLoc, MoveToLocationDestination);
                if (DistToDest <= AcceptRadius)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void AAIController_Base::NotifyMoveToLocationCompleted()
{
    if (APawn* ControlledPawn = GetPawn())
    {
        if (UAISplinePatrolComponent* Patrol = ControlledPawn->FindComponentByClass<UAISplinePatrolComponent>())
        {
            Patrol->OnMoveCompleted();
        }
        else if (UAIWanderComponent* WanderComp = ControlledPawn->FindComponentByClass<UAIWanderComponent>())
        {
            WanderComp->OnDestinationReached();
        }
    }
}

void AAIController_Base::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
}

void AAIController_Base::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    if (!Result.IsSuccess())
    {
        return;
    }

    if (APawn* ControlledPawn = GetPawn())
    {
        if (UAISplinePatrolComponent* Patrol = ControlledPawn->FindComponentByClass<UAISplinePatrolComponent>())
        {
            Patrol->OnMoveCompleted();
        }
    }
}

void AAIController_Base::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor) return;

    ABaseAI* AI = Cast<ABaseAI>(GetPawn());
    if (!AI || !AI->StateComp) return;

    UAISplinePatrolComponent* Patrol = AI->FindComponentByClass<UAISplinePatrolComponent>();
    UAIWanderComponent* WanderComp = AI->FindComponentByClass<UAIWanderComponent>();

    if (Patrol)
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            TargetActor = Actor;
            AbortMoveToLocation();
            AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Chase);
            Patrol->SetChasing(true);
        }
        else
        {
            TargetActor = nullptr;
            PathPoints.Empty();
            PathPointIndex = 0;
            PathUpdateTimer = 0.f;
            AbortMoveToLocation();
            AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Patrol);
            Patrol->SetChasing(false);
            Patrol->RequestReturnToSpline();
        }
    }
    else if (WanderComp)
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            TargetActor = Actor;
            AbortMoveToLocation();
            AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Chase);
        }
        else
        {
            TargetActor = nullptr;
            PathPoints.Empty();
            PathPointIndex = 0;
            PathUpdateTimer = 0.f;
            AbortMoveToLocation();
            AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Wander);
            WanderComp->StartWander();
        }
    }
}
