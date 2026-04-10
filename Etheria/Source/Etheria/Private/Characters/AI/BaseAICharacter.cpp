/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Class: BaseAICharacter - Source
 */

#include "Characters/AI/BaseAICharacter.h"

#include "Characters/AI/Movements/AIMovementComponent.h"
#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Characters/AI/Controller/BaseAIController.h"

ABaseAICharacter::ABaseAICharacter()
{
	AIMovementComponent = CreateDefaultSubobject<UAIMovementComponent>(TEXT("AIMovementComponent"));
	AIAnimationComponent = CreateDefaultSubobject<UAIAnimationComponent>(TEXT("AIAnimationComponent"));

	// Garantir que le controller est assigné et auto-possédé
	AIControllerClass = ABaseAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ABaseAICharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnLocation = GetActorLocation();
}

/* ─────────────────── State ─────────────────── */

void ABaseAICharacter::SetAIState(EAIState NewState)
{
	if (CurrentState == NewState) return;

	const EAIState OldState = CurrentState;
	CurrentState = NewState;
	OnAIStateChanged.Broadcast(OldState, NewState);
}

void ABaseAICharacter::SetTarget(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
	if (NewTarget)
	{
		OnTargetAcquired.Broadcast(NewTarget);
	}
}

void ABaseAICharacter::ClearTarget()
{
	CurrentTarget = nullptr;
	OnTargetLost.Broadcast();
}

/* ─────────────────── Perception / Damage ─────────────────── */

void ABaseAICharacter::OnPerceiveTarget(AActor* PerceivedActor)
{
	if (!PerceivedActor) return;
	if (CurrentState == EAIState::Dead) return;

	switch (HostilityType)
	{
	case EAIHostilityType::Passive:
		// Passive NPCs never engage
		break;

	case EAIHostilityType::Neutral:
		// Neutral only engages if already provoked (handled in OnReceiveDamage)
		break;

	case EAIHostilityType::Aggressive:
		if (CurrentState != EAIState::Chasing && CurrentState != EAIState::Attacking)
		{
			SetTarget(PerceivedActor);
			SetAIState(EAIState::Chasing);
			if (AIMovementComponent)
			{
				AIMovementComponent->StopPatrol();
				AIMovementComponent->SetDesiredSpeed(AIMovementComponent->ChaseSpeed);
				AIMovementComponent->MoveToLocation(PerceivedActor->GetActorLocation());
			}
		}
		break;
	}
}

void ABaseAICharacter::OnReceiveDamage(AActor* DamageInstigator, float DamageAmount)
{
	if (CurrentState == EAIState::Dead) return;

	OnAIDamaged.Broadcast(DamageInstigator);

	// Neutral becomes aggressive when attacked
	if (HostilityType == EAIHostilityType::Neutral && DamageInstigator)
	{
		SetTarget(DamageInstigator);
		SetAIState(EAIState::Chasing);
		if (AIMovementComponent)
		{
			AIMovementComponent->StopPatrol();
			AIMovementComponent->SetDesiredSpeed(AIMovementComponent->ChaseSpeed);
			AIMovementComponent->MoveToLocation(DamageInstigator->GetActorLocation());
		}
	}

	// Play hit reaction
	if (AIAnimationComponent)
	{
		AIAnimationComponent->PlayHitReaction();
	}
}
