/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BaseAI - Source
*/

#include "Characters/AI/BaseAI.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/Combat/CombatComponent.h"

ABaseAI::ABaseAI()
{
    AIType = EAIType::Neutral;
}

void ABaseAI::BeginPlay()
{
    Super::BeginPlay();

    if (Tags.Contains("Enemy"))
    {
        AIType = EAIType::Hostile;
    }

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
    GetCharacterMovement()->MaxWalkSpeed = 300.f;

    StateComp = FindComponentByClass<UCharacterStateComponent>();
    CombatComp = FindComponentByClass<UCombatComponent>();
}
