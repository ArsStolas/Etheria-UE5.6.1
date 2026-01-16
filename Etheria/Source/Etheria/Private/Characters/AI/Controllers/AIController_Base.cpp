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
	SightConfig->DetectionByAffiliation.bDetectEnemies   = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals   = true;

	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AAIController_Base::OnTargetPerceptionUpdated);
}

void AAIController_Base::BeginPlay()
{
	Super::BeginPlay();

	if (ABaseAI* AI = Cast<ABaseAI>(GetPawn()))
	{
		if (AI->StateComp && AI->StateComp->IsInMovementState(EtheriaTags::State_Movement_Patrol))
		{
			if (UAISplinePatrolComponent* Patrol = AI->FindComponentByClass<UAISplinePatrolComponent>())
			{
				Patrol->bIsMovingToPoint = false;
				Patrol->SnapToClosestPoint();
				Patrol->StartPatrol();
			}
		}
	}
}

void AAIController_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetActor)
	{
		ABaseAI* AI = Cast<ABaseAI>(GetPawn());
		if (!AI || !AI->StateComp) return;

		const float Dist = FVector::Dist(TargetActor->GetActorLocation(), AI->GetActorLocation());

		AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Chase);
		MoveToActor(TargetActor, 50.f);
	}
	else
	{
		ABaseAI* AI = Cast<ABaseAI>(GetPawn());
		if (AI && AI->StateComp && AI->StateComp->IsInMovementState(EtheriaTags::State_Movement_Patrol))
		{
			if (UAISplinePatrolComponent* Patrol = AI->FindComponentByClass<UAISplinePatrolComponent>())
			{
				if (!Patrol->bIsMovingToPoint)
				{
					Patrol->SnapToClosestPoint();
					Patrol->StartPatrol();
				}
			}
		}
	}
}


void AAIController_Base::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn) return;

	ABaseAI* AI = Cast<ABaseAI>(InPawn);
	if (!AI || !AI->StateComp) return;

	if (AI->StateComp->IsInMovementState(EtheriaTags::State_Movement_Patrol))
	{
		if (UAISplinePatrolComponent* Patrol = InPawn->FindComponentByClass<UAISplinePatrolComponent>())
		{
			Patrol->bIsMovingToPoint = false;
			Patrol->SnapToClosestPoint();
			Patrol->StartPatrol();
		}
	}
}

void AAIController_Base::OnMoveCompleted(
	FAIRequestID RequestID,
	const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	UAISplinePatrolComponent* Patrol = ControlledPawn->FindComponentByClass<UAISplinePatrolComponent>();
	if (!Patrol) return;

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
			GetWorld()->GetTimerManager().ClearTimer(Patrol->PatrolTimerHandle);
	}
	else
	{
		TargetActor = nullptr;
		AI->StateComp->SetMovementState(EtheriaTags::State_Movement_Patrol);

		if (UAISplinePatrolComponent* Patrol = AI->FindComponentByClass<UAISplinePatrolComponent>())
		{
			Patrol->SnapToClosestPoint();
			Patrol->MoveToNextPoint();
		}
	}
}
