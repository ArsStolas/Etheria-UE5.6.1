/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemBossController - Source"
 */

#include "Characters/AI/Boss/GolemBossController.h"

AGolemBossController::AGolemBossController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	bUseCustomAttackLogic = true;
	bCustomLogicFacesTarget = false;
	bUseAttackTokens = false;
	bUseReactiveEvade = false;

	DetectionRadius = 12000.f;
	LoseSightRadius = 14000.f;
	DetectionReactionTime = 0.f;
	HearingRange = 12000.f;
	TargetMemoryDuration = 0.f;
}
