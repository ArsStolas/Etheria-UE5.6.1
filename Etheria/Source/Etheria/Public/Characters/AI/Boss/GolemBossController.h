/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemBossController - Header"
 * Notes: Controller preset for the stationary arena Golem. Enables custom (BP/C++-scripted) offense
 *        so the default chase/melee brain stays out of the way, and configures relentless, arena-wide
 *        perception. The Golem's attacks are driven by UGolemBossComponent, not by this controller.
 */

#pragma once

#include "CoreMinimal.h"
#include "Characters/AI/Controller/BaseAIController.h"
#include "GolemBossController.generated.h"

UCLASS()
class ETHERIA_API AGolemBossController : public ABaseAIController
{
	GENERATED_BODY()

public:
	AGolemBossController(const FObjectInitializer& ObjectInitializer);
};
