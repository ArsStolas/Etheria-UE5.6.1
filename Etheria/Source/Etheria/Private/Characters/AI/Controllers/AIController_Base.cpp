/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ChatGPT
* Class: AIController_Base - Source
*/

#include "Characters/AI/Controllers/AIController_Base.h"
#include "Characters/AI/BaseAI.h"
#include "Components/Characters/IA/AISplinePatrolComponent.h"
#include "Components/Characters/IA/AIWanderComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

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
    if (PerceptionComp)
        PerceptionComp->RequestStimuliListenerUpdate();
}

void AAIController_Base::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (!InPawn) return;

    if (UAISplinePatrolComponent* Patrol = InPawn->FindComponentByClass<UAISplinePatrolComponent>())
        Patrol->StartPatrol();

    if (UAIWanderComponent* Wander = InPawn->FindComponentByClass<UAIWanderComponent>())
        Wander->StartWander();
}

void AAIController_Base::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ABaseAI* AI = Cast<ABaseAI>(GetPawn());
    if (!AI || !AI->StateComp) return;

    if (!TargetActor) return;

    const float Dist = FVector::Dist(TargetActor->GetActorLocation(), AI->GetActorLocation());
    const float FollowDistance = 50.f;

    if (Dist > FollowDistance)
    {
        MoveToActor(TargetActor, FollowDistance);
        AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Chase);
    }
    else
    {
        AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Fight);
        StopMovement();
    }
}

void AAIController_Base::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    if (UAISplinePatrolComponent* Patrol = ControlledPawn->FindComponentByClass<UAISplinePatrolComponent>())
    {
        Patrol->bIsMovingToPoint = false;
        Patrol->AdvanceIndex();

        FTimerDelegate Del;
        Del.BindUFunction(Patrol, FName("MoveToNextPoint"));

        GetWorld()->GetTimerManager().SetTimer(
            Patrol->PatrolTimerHandle,
            Del,
            Patrol->WaitTimeAtPoint,
            false
        );
    }
}

void AAIController_Base::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor) return;

    ABaseAI* AI = Cast<ABaseAI>(GetPawn());
    if (!AI || !AI->StateComp) return;

    if (Stimulus.WasSuccessfullySensed())
    {
        TargetActor = Actor;

        AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Chase);

        if (UAISplinePatrolComponent* Patrol = AI->FindComponentByClass<UAISplinePatrolComponent>())
        {
            GetWorld()->GetTimerManager().ClearTimer(Patrol->PatrolTimerHandle);
        }

        if (UAIWanderComponent* Wander = AI->FindComponentByClass<UAIWanderComponent>())
        {
            GetWorld()->GetTimerManager().ClearTimer(Wander->WanderTimer);
        }
    }
    else
    {
        TargetActor = nullptr;
        AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Wander);

        if (UAISplinePatrolComponent* Patrol = AI->FindComponentByClass<UAISplinePatrolComponent>())
        {
            Patrol->SnapToClosestPoint();

            if (!Patrol->bIsMovingToPoint)
                Patrol->MoveToNextPoint();
        }

        if (UAIWanderComponent* Wander = AI->FindComponentByClass<UAIWanderComponent>())
            Wander->StartWander();
    }
}
