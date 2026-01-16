/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AIController_Base - Source
*/

#include "Characters/AI/Controllers/AIController_Base.h"
#include "Characters/AI/BaseAI.h"
#include "Components/Combat/CombatComponent.h"
#include "Components/Characters/IA/AISplinePatrolComponent.h"
#include "TimerManager.h"
#include "Navigation/PathFollowingComponent.h" 
#include "NavigationSystem.h"
#include "Engine/World.h"

AAIController_Base::AAIController_Base()
{
	PrimaryActorTick.bCanEverTick = true;

	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SightConfig->SightRadius = 1500.f;
	SightConfig->LoseSightRadius = 1800.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
	SightConfig->SetMaxAge(5.f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

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

void AAIController_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!TargetActor) return;

	if (ABaseAI* AI = Cast<ABaseAI>(GetPawn()))
	{
		if (!AI->CombatComp) return;

		float Dist = FVector::Dist(TargetActor->GetActorLocation(), AI->GetActorLocation());
		float AttackRange = AI->CombatComp->GetCurrentAttackRange();

		if (Dist < AttackRange)
		{
			AI->CombatComp->SetExternalTarget(TargetActor);
			AI->CombatComp->TryAttackPrimary();
		}
		else
		{
			RequestMoveTo(TargetActor->GetActorLocation());
		}
	}
}

void AAIController_Base::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn) return;

	if (UAISplinePatrolComponent* Patrol = InPawn->FindComponentByClass<UAISplinePatrolComponent>())
	{
		Patrol->StartPatrol();
	}
}

void AAIController_Base::RequestMoveTo(const FVector& Destination)
{
	FAIMoveRequest Request;
	Request.SetGoalLocation(Destination);
	Request.SetAcceptanceRadius(50.f);
	Request.SetUsePathfinding(true);
	Request.SetAllowPartialPath(true);

	MoveTo(Request);
}

void AAIController_Base::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (APawn* OwnerPawn = GetPawn())
	{
		if (UAISplinePatrolComponent* Patrol = OwnerPawn->FindComponentByClass<UAISplinePatrolComponent>())
		{
			Patrol->bIsMovingToPoint = false;
			Patrol->AdvanceIndex();

			FTimerDelegate TimerDel;
			TimerDel.BindUFunction(Patrol, FName("MoveToNextPoint"));
			GetWorld()->GetTimerManager().SetTimer(
				Patrol->PatrolTimerHandle,
				TimerDel,
				Patrol->WaitTimeAtPoint,
				false
			);
		}
	}
}

void AAIController_Base::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (ABaseAI* AI = Cast<ABaseAI>(GetPawn()))
	{
		if (!AI->StateComp) return;

		if (Stimulus.WasSuccessfullySensed())
		{
			TargetActor = Actor;
			AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Chase);

			if (UAISplinePatrolComponent* Patrol = AI->FindComponentByClass<UAISplinePatrolComponent>())
			{
				GetWorld()->GetTimerManager().ClearTimer(Patrol->PatrolTimerHandle);
			}
		}
		else
		{
			TargetActor = nullptr;
			AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Wander);

			if (UAISplinePatrolComponent* Patrol = AI->FindComponentByClass<UAISplinePatrolComponent>())
			{
				Patrol->SnapToClosestPoint();
				Patrol->MoveToNextPoint();
			}
		}
	}
}
