/*
* Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: BaseAI - Source
*/

#include "Characters/AI/BaseAI.h"

ABaseAI::ABaseAI()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABaseAI::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	/*
	 * DELETE Perception and Handle Decision from Tick when really used.
	 * Instead use FTimer, Events calls etc.
	 */
	
	HandlePerception(); 
	HandleDecisionMaking();
}

void ABaseAI::HandlePerception()
{
	
}

void ABaseAI::HandleDecisionMaking()
{
	
}
