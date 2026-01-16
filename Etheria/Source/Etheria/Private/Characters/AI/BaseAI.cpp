/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BaseAI - Source
*/

#include "Characters/AI/BaseAI.h"

#include "Components/Characters/IA/AISplinePatrolComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/Combat/CombatComponent.h"

ABaseAI::ABaseAI()
{
    AIType = EAIType::Neutral;
}

void ABaseAI::BeginPlay()
{
    Super::BeginPlay();

    if (!StateComp)
        StateComp = FindComponentByClass<UCharacterStateComponent>();

    if (StateComp)
        StateComp->SetMovementState(EtheriaTags::State_Movement_Patrol);

    CombatComp = FindComponentByClass<UCombatComponent>();

    if (UAISplinePatrolComponent* Patrol = FindComponentByClass<UAISplinePatrolComponent>())
    {
        Patrol->bIsMovingToPoint = false;
        Patrol->SnapToClosestPoint();
        Patrol->StartPatrol();
    }
}