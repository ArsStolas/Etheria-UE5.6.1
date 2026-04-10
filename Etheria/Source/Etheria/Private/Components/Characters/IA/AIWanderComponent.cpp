/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AIWanderComponent - Source
*/

#include "Components/Characters/IA/AIWanderComponent.h"
#include "Characters/AI/BaseAI.h"
#include "Characters/AI/Controllers/AIController_Base.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "Core/System/EtheriaGameplayTags.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

void UAIWanderComponent::BeginPlay()
{
    Super::BeginPlay();

    // Démarrer la marche au hasard après un court délai (controller possédé, nav prête).
    GetWorld()->GetTimerManager().SetTimer(
        WanderTimer,
        this,
        &UAIWanderComponent::StartWander,
        StartWanderDelay,
        false
    );
}

void UAIWanderComponent::StartWander()
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return;

    ABaseAI* AI = Cast<ABaseAI>(OwnerPawn);
    if (!AI) return;

    // Ne pas demander un nouveau point si on est en chase (le controller gère la cible).
    if (AAIController_Base* AIController = Cast<AAIController_Base>(OwnerPawn->GetController()))
    {
        if (AIController->GetTargetActor() != nullptr)
        {
            return;
        }
    }

    if (AI->StateComp)
    {
        AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Wander);
    }

    FVector Dest = GetRandomPointInRadius();
    if (AAIController_Base* AIController = Cast<AAIController_Base>(OwnerPawn->GetController()))
    {
        AIController->RequestMoveToLocation(Dest);
    }

    // Prochain point uniquement quand la destination est atteinte (voir OnDestinationReached).
}

void UAIWanderComponent::OnDestinationReached()
{
    if (WaitTime > 0.f)
    {
        GetWorld()->GetTimerManager().SetTimer(
            WanderTimer,
            this,
            &UAIWanderComponent::StartWander,
            WaitTime,
            false
        );
    }
    else
    {
        StartWander();
    }
}

void UAIWanderComponent::StopWander()
{
    GetWorld()->GetTimerManager().ClearTimer(WanderTimer);
}

FVector UAIWanderComponent::GetRandomPointInRadius()
{
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    FNavLocation Result;
    FVector Origin = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

    if (NavSys)
    {
        NavSys->GetRandomReachablePointInRadius(Origin, WanderRadius, Result);
    }

    return Result.Location;
}