/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AIWanderComponent - Source
*/

#include "Components/Characters/IA/AIWanderComponent.h"
#include "Characters/AI/BaseAI.h"
#include "Characters/AI/Controllers/AIController_Base.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Navigation/PathFollowingComponent.h"

void UAIWanderComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAIWanderComponent::StartWander()
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (ABaseAI* AI = Cast<ABaseAI>(OwnerPawn))
		{
			if (!AI->StateComp || AI->StateComp->IsInMovementState(EtheriaTags::State_Movement_Wander))
			{
				FVector Dest = GetRandomPointInRadius();
				if (AAIController_Base* AIController = Cast<AAIController_Base>(OwnerPawn->GetController()))
					AIController->MoveTo(Dest);
			}
		}
	}

	GetWorld()->GetTimerManager().SetTimer(WanderTimer, this, &UAIWanderComponent::StartWander, WaitTime, false);
}

FVector UAIWanderComponent::GetRandomPointInRadius()
{
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation Result;

	if (NavSys)
		NavSys->GetRandomReachablePointInRadius(GetOwner()->GetActorLocation(), WanderRadius, Result);

	return Result.Location;
}
