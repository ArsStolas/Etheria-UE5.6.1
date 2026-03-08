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
#include "Characters/BaseCharacter.h"
#include "Characters/Players/PlayerCharacter.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"

ABaseAI::ABaseAI()
{
    AIType = EAIType::Neutral;
    PrimaryActorTick.bCanEverTick = true;
    AttackRange = 300.f;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxAcceleration = AIMaxAcceleration;
        MoveComp->BrakingDecelerationWalking = AIBrakingDeceleration;
        MoveComp->MaxWalkSpeed = DefaultMaxWalkSpeed;
        MoveComp->bOrientRotationToMovement = true;
    }
}

void ABaseAI::BeginPlay()
{
    Super::BeginPlay();

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxAcceleration = AIMaxAcceleration;
        MoveComp->BrakingDecelerationWalking = AIBrakingDeceleration;
        MoveComp->MaxWalkSpeed = DefaultMaxWalkSpeed;
        MoveComp->bOrientRotationToMovement = true;
    }

    if (!StateComp)
    {
        StateComp = FindComponentByClass<UCharacterStateComponent>();
    }

    if (StateComp)
    {
        StateComp->SetMovementState(EtheriaTags::State_Movement_Patrol);
    }

    CombatComp = FindComponentByClass<UCombatComponent>();

    SetupTeamTags();

    if (IsHostile() && ShouldUseHostileCombatLoop())
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &ABaseAI::StartHostileCombatLoop);
    }
}

void ABaseAI::SetupTeamTags()
{
    TeamTags.Reset(0);

    FName TagName;
    switch (AIType)
    {
        case EAIType::Friendly: TagName = FName("AI.Friendly"); break;
        case EAIType::Neutral:  TagName = FName("AI.Neutral");  break;
        case EAIType::Hostile:  TagName = FName("AI.Hostile");  break;
        default: return;
    }

    TeamTags.AddTag(FGameplayTag::RequestGameplayTag(TagName));
    UE_LOG(LogTemp, Log, TEXT("[BaseAI %s] Team: %s"), *GetName(), *TeamTags.ToString());
}

bool ABaseAI::IsTargetHostile(AActor* InTargetActor) const
{
    if (!InTargetActor || !CombatComp)
    {
        return false;
    }

    const_cast<ABaseAI*>(this)->TargetActor = InTargetActor;

    if (APlayerCharacter* Player = Cast<APlayerCharacter>(InTargetActor))
    {
        return !IsFriendly();
    }

    if (ABaseAI* TargetAI = Cast<ABaseAI>(InTargetActor))
    {
        return !TeamTags.HasAny(TargetAI->TeamTags);
    }

    return false;
}

void ABaseAI::TryAttack(AActor* InTargetActor)
{
    if (!IsTargetHostile(InTargetActor))
    {
        return;
    }

    PerformAIAttack(InTargetActor);
}

void ABaseAI::PerformAIAttack(AActor* InTargetActor)
{
    if (!CombatComp || !InTargetActor)
    {
        return;
    }

    if (CombatComp->IsAttackActive())
    {
        return;
    }

    const FVector ToTarget = InTargetActor->GetActorLocation() - GetActorLocation();
    FVector FlatDir = ToTarget;
    FlatDir.Z = 0.f;
    if (!FlatDir.IsNearlyZero())
    {
        SetActorRotation(FlatDir.Rotation());
    }

    if (CombatComp->IsInCooldown())
    {
        return;
    }

    if (CombatComp->TryAttackPrimary())
    {
        UE_LOG(LogTemp, Warning, TEXT("[BaseAI %s] Primary → %s"),
            *GetName(), *InTargetActor->GetName());
    }
}


void ABaseAI::StartHostileCombatLoop()
{
    if (!TargetActor)
    {
        TargetActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    }

    const bool bHasTarget =
        TargetActor &&
        FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation()) <= AttackRange &&
        IsTargetHostile(TargetActor);

    if (bHasTarget)
    {
        if (!CombatComp || !CombatComp->IsAttackActive())
        {
            TryAttack(TargetActor);
        }

        const float Interval = 0.2f;
        GetWorldTimerManager().SetTimer(
            CombatTimerHandle,
            this,
            &ABaseAI::StartHostileCombatLoop,
            Interval,
            false
        );
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[BaseAI %s] Patrol (no or far target)"), *GetName());
        GetWorldTimerManager().SetTimer(
            CombatTimerHandle,
            this,
            &ABaseAI::StartHostileCombatLoop,
            1.5f,
            false
        );
    }
}