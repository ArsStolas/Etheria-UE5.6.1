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
	// Hand all offense to the GolemBossComponent (no built-in chase/melee/evade/token brain).
	bUseCustomAttackLogic = true;
	bCustomLogicFacesTarget = false; // the component yaws the body itself
	bUseAttackTokens = false;
	bUseReactiveEvade = false;

	// See the whole arena and never give up the player (relentless boss).
	SightRadius = 12000.f;
	LoseSightRadius = 14000.f;
	SightFOVDegrees = 180.f;
	HearingRange = 12000.f;
	TargetMemoryDuration = 0.f;
	bUseDetectionRamp = false;
}
