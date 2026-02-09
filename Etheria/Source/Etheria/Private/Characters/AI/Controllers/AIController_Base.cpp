/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AIController_Base - Source
*/

#include "Characters/AI/Controllers/AIController_Base.h"
#include "Characters/AI/BaseAI.h"
#include "Components/Characters/IA/AISplinePatrolComponent.h"
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
}

void AAIController_Base::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (TargetActor)
    {
        ABaseAI* AI = Cast<ABaseAI>(GetPawn());
        if (!AI || !AI->StateComp) return;

        AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Chase);
        MoveToActor(TargetActor, 50.f);
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

    if (UAISplinePatrolComponent* Patrol = AI->FindComponentByClass<UAISplinePatrolComponent>())
    {
        if (Stimulus.WasSuccessfullySensed())
        {
            TargetActor = Actor;
            AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Chase);
            Patrol->SetChasing(true);
        }
        else
        {
            TargetActor = nullptr;
            AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Patrol);
            Patrol->SetChasing(false);
            Patrol->RequestReturnToSpline();
        }
    }
}
