/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Class: BaseAIController - Source
 */

#include "Characters/AI/Controller/BaseAIController.h"

#include "Characters/AI/BaseAICharacter.h"
#include "Characters/AI/Movements/AIMovementComponent.h"
#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "GameFramework/Character.h"

ABaseAIController::ABaseAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	SetupPerception();
}

void ABaseAIController::SetupPerception()
{
	UAIPerceptionComponent* PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SetPerceptionComponent(*PerceptionComp);

	// Sight
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = SightFOVDegrees;
	SightConfig->SetMaxAge(5.f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	PerceptionComp->ConfigureSense(*SightConfig);

	// Hearing
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(3.f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	PerceptionComp->ConfigureSense(*HearingConfig);

	PerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ABaseAIController::OnPerceptionUpdated);
}

void ABaseAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AICharacter = Cast<ABaseAICharacter>(InPawn);
	if (!AICharacter) return;

	SpawnOrigin = AICharacter->GetActorLocation();

	// Update perception values from config
	if (UAIPerceptionComponent* PerceptionComp = GetPerceptionComponent())
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = SightFOVDegrees;
		HearingConfig->HearingRange = HearingRange;
		PerceptionComp->RequestStimuliListenerUpdate();
	}

	// Start patrol if character has a movement mode configured
	if (UAIMovementComponent* MoveComp = AICharacter->GetAIMovement())
	{
		if (MoveComp->PatrolMode != EPatrolMode::Stationary)
		{
			AICharacter->SetAIState(EAIState::Patrolling);
			MoveComp->StartPatrol();
		}
	}
}

void ABaseAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!AICharacter) return;

	switch (AICharacter->GetCurrentAIState())
	{
	case EAIState::Idle:			HandleIdleState(DeltaTime);		break;
	case EAIState::Patrolling:		HandlePatrolState(DeltaTime);	break;
	case EAIState::Chasing:			HandleChaseState(DeltaTime);	break;
	case EAIState::Attacking:		HandleAttackState(DeltaTime);	break;
	case EAIState::Returning:		HandleReturnState(DeltaTime);	break;
	default: break;
	}
}

/* ─────────────────── Perception ─────────────────── */

void ABaseAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!AICharacter || !Actor) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		AICharacter->OnPerceiveTarget(Actor);
	}
	else
	{
		// Lost sight
		if (AICharacter->GetCurrentTarget() == Actor)
		{
			AICharacter->ClearTarget();
			AICharacter->SetAIState(EAIState::Returning);
		}
	}
}

/* ─────────────────── State Handlers ─────────────────── */

void ABaseAIController::HandleIdleState(float DeltaTime)
{
	IdleTimer += DeltaTime;
	if (IdleTimer >= IdleAnimInterval)
	{
		IdleTimer = 0.f;
		if (UAIAnimationComponent* AnimComp = AICharacter->GetAIAnimation())
		{
			AnimComp->PlayRandomIdle();
		}
	}
}

void ABaseAIController::HandlePatrolState(float DeltaTime)
{
	// Patrol logic is driven by AIMovementComponent's own tick.
	// Controller just plays idle anims when waiting at points.
	if (UAIMovementComponent* MoveComp = AICharacter->GetAIMovement())
	{
		if (MoveComp->HasReachedDestination())
		{
			HandleIdleState(DeltaTime);
		}
		else
		{
			IdleTimer = 0.f;
		}
	}
}

void ABaseAIController::HandleChaseState(float DeltaTime)
{
	AActor* Target = AICharacter->GetCurrentTarget();
	if (!Target)
	{
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

	const float DistToTarget = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());
	const float DistToSpawn = FVector::Dist(AICharacter->GetActorLocation(), SpawnOrigin);

	// Leash check
	if (DistToSpawn > AICharacter->GetLeashRange())
	{
		AICharacter->ClearTarget();
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

	// In attack range?
	if (DistToTarget <= AttackRange)
	{
		AICharacter->SetAIState(EAIState::Attacking);
		if (UAIMovementComponent* MoveComp = AICharacter->GetAIMovement())
		{
			MoveComp->StopMovement();
		}
		return;
	}

	// Keep chasing
	if (UAIMovementComponent* MoveComp = AICharacter->GetAIMovement())
	{
		MoveComp->SetDesiredSpeed(MoveComp->ChaseSpeed);
		MoveComp->MoveToLocation(Target->GetActorLocation());
	}
}

void ABaseAIController::HandleAttackState(float DeltaTime)
{
	AActor* Target = AICharacter->GetCurrentTarget();
	if (!Target)
	{
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

	const float DistToTarget = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());

	// Target ran out of attack range — chase again
	if (DistToTarget > AttackRange * 1.2f)
	{
		AICharacter->SetAIState(EAIState::Chasing);
		return;
	}

	// Face target
	const FVector Direction = (Target->GetActorLocation() - AICharacter->GetActorLocation()).GetSafeNormal();
	AICharacter->SetActorRotation(Direction.Rotation());

	// Attack cooldown
	AttackTimer -= DeltaTime;
	if (AttackTimer <= 0.f)
	{
		AttackTimer = AttackCooldown;
		if (UAIAnimationComponent* AnimComp = AICharacter->GetAIAnimation())
		{
			AnimComp->PlayRandomAttack();
		}
	}
}

void ABaseAIController::HandleReturnState(float DeltaTime)
{
	if (UAIMovementComponent* MoveComp = AICharacter->GetAIMovement())
	{
		MoveComp->SetDesiredSpeed(MoveComp->PatrolSpeed);
		MoveComp->MoveToLocation(SpawnOrigin);

		if (FVector::Dist(AICharacter->GetActorLocation(), SpawnOrigin) < MoveComp->AcceptanceRadius)
		{
			// Back at spawn — resume patrol or idle
			if (MoveComp->PatrolMode != EPatrolMode::Stationary)
			{
				AICharacter->SetAIState(EAIState::Patrolling);
				MoveComp->StartPatrol();
			}
			else
			{
				AICharacter->SetAIState(EAIState::Idle);
			}
		}
	}
}
