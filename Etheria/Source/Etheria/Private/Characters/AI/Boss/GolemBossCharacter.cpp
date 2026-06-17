/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemBossCharacter - Source"
 */

#include "Characters/AI/Boss/GolemBossCharacter.h"

#include "Characters/AI/Boss/GolemBossComponent.h"
#include "Characters/AI/Boss/GolemBossController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AGolemBossCharacter::AGolemBossCharacter()
{
	GolemBossComponent = CreateDefaultSubobject<UGolemBossComponent>(TEXT("GolemBossComponent"));

	HostilityType = EAIHostilityType::Aggressive;
	Rank = EAIRank::Boss;

	AIControllerClass = AGolemBossController::StaticClass();

	// Hand-faced giant: the component drives yaw; movement must not fight it or wander off.
	if (UCharacterMovementComponent* MC = GetCharacterMovement())
		MC->bOrientRotationToMovement = false;

	// Big-boss footprint by default (the root capsule must stay the root on a Character — tune to your mesh in BP).
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		Capsule->InitCapsuleSize(120.f, 300.f);

	// Stationary arena boss: don't drop aggro at the default 2000cm leash (sight is arena-wide).
	LeashRange = 100000.f;

	BuildDefaultAttacks();
}

void AGolemBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Anchored giant: freeze the movement component so contact / depenetration can never drift the body off its mark.
	// Collision stays untouched (the capsule still blocks the player and receives hits); facing yaw uses SetActorRotation.
	if (bImmovable)
		if (UCharacterMovementComponent* MC = GetCharacterMovement())
			MC->DisableMovement();
}

void AGolemBossCharacter::LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride)
{
	// The arena Golem absorbs hits without being knocked back — you can damage it, but you can't shove it.
	if (bImmovable) return;
	Super::LaunchCharacter(LaunchVelocity, bXYOverride, bZOverride);
}

void AGolemBossCharacter::BuildDefaultAttacks()
{
	if (!GolemBossComponent) return;

	GolemBossComponent->ArenaRadius = 2000.f;
	GolemBossComponent->EyeSocketNames = { TEXT("eye_l"), TEXT("eye_r") }; // rename to match your skeleton
	GolemBossComponent->ThrowSocketName = TEXT("hand_r");                  // rock leaves this socket at the throw release

	TArray<FGolemAttackConfig>& Atk = GolemBossComponent->Attacks;
	Atk.Reset();

	// 1) Hammer punch + expanding shockwave ring (jump the ring).
	{
		FGolemAttackConfig A;
		A.AttackId = TEXT("HammerPunch");
		A.Shape = EGolemHazardShape::RadialSlam;
		A.TargetMode = EGolemTargetMode::AtTarget;
		A.WindupDuration = 1.3f; A.ActiveDuration = 0.9f; A.RecoveryDuration = 1.0f; A.Cooldown = 6.f;
		A.Damage = 30.f; A.ImpactRadius = 350.f; A.DamageInterval = 0.1f;
		A.bEmitShockwave = true; A.ShockwaveMaxRadius = 1500.f; A.ShockwaveThickness = 250.f; A.ShockwaveDamage = 18.f;
		A.KnockbackForce = 700.f; A.SelectionWeight = 1.2f;
		Atk.Add(A);
	}

	// 2) Rock throw — opens the AIR ZONE (spawns your WindColumn). Forced before the unavoidable attack.
	{
		FGolemAttackConfig A;
		A.AttackId = TEXT("RockThrow");
		A.Shape = EGolemHazardShape::ProjectileImpact;
		A.TargetMode = EGolemTargetMode::AtTarget;
		A.WindupDuration = 1.5f; A.ActiveDuration = 0.f; A.RecoveryDuration = 0.8f; A.Cooldown = 10.f;
		A.Damage = 25.f; A.ImpactRadius = 400.f;
		A.bOpensAirZone = true; A.AirZoneRadius = 700.f; A.AirZoneLifetime = 12.f;
		A.SelectionWeight = 0.8f;
		Atk.Add(A);
	}

	// 3) Arena sweep — a wall to jump over.
	{
		FGolemAttackConfig A;
		A.AttackId = TEXT("ArenaSweep");
		A.Shape = EGolemHazardShape::SweepLine;
		A.TargetMode = EGolemTargetMode::AtTarget;
		A.WindupDuration = 1.2f; A.ActiveDuration = 1.4f; A.RecoveryDuration = 0.9f; A.Cooldown = 8.f;
		A.Damage = 28.f; A.SweepThickness = 300.f; A.SweepLength = 3000.f; A.DamageInterval = 0.1f;
		A.bRequiresAirZoneEscape = true; // auto-forces a rock-throw first
		A.MinSafeAltitude = 450.f;       // above a jump (~340) yet reachable by the wind column — must FLY to clear it
		A.SelectionWeight = 1.0f;
		Atk.Add(A);
	}

	// 4) Two-hand slam — ground fissures across the whole floor. Unavoidable on the ground (fly to escape).
	{
		FGolemAttackConfig A;
		A.AttackId = TEXT("TwoHandSlam");
		A.Shape = EGolemHazardShape::GroundFissures;
		A.TargetMode = EGolemTargetMode::ArenaExtremities;
		A.WindupDuration = 1.6f; A.ActiveDuration = 2.0f; A.RecoveryDuration = 1.2f; A.Cooldown = 14.f;
		A.Damage = 22.f; A.DamageInterval = 0.4f; A.ImpactRadius = 300.f;
		A.bRequiresAirZoneEscape = true;    // auto-forces a RockThrow beforehand
		A.bExposesArmWeakPoints = true;     // arms down -> the player can climb and break the arm crystals
		A.SelectionWeight = 0.7f;
		Atk.Add(A);
	}

	// 5) Eye laser — beams sweeping in an X / V.
	{
		FGolemAttackConfig A;
		A.AttackId = TEXT("EyeLaser");
		A.Shape = EGolemHazardShape::BeamSweep;
		A.TargetMode = EGolemTargetMode::EyeSockets;
		A.WindupDuration = 1.3f; A.ActiveDuration = 1.6f; A.RecoveryDuration = 1.0f; A.Cooldown = 9.f;
		A.Damage = 20.f; A.DamageInterval = 0.15f;
		A.BeamCount = 2; A.BeamLength = 3500.f; A.BeamWidth = 160.f; A.BeamSpreadAngle = 70.f; A.BeamSweepAngle = 50.f;
		A.SelectionWeight = 0.9f;
		Atk.Add(A);
	}

	// 6) Rock avalanche — many random impacts to dodge. Does NOT open air zones.
	{
		FGolemAttackConfig A;
		A.AttackId = TEXT("RockAvalanche");
		A.Shape = EGolemHazardShape::Bombardment;
		A.TargetMode = EGolemTargetMode::ArenaRandom;
		A.WindupDuration = 1.0f; A.ActiveDuration = 4.0f; A.RecoveryDuration = 0.8f; A.Cooldown = 12.f;
		A.Damage = 24.f; A.ImpactRadius = 250.f;
		A.ImpactCount = 14; A.BombardmentSpread = 1.0f; A.BombardmentWarnLead = 0.8f;
		A.SelectionWeight = 0.9f;
		Atk.Add(A);
	}

	// Light 2-phase ramp (HP-gated). Order high -> low threshold.
	TArray<FGolemPhaseConfig>& Ph = GolemBossComponent->Phases;
	Ph.Reset();
	{
		FGolemPhaseConfig P; P.PhaseName = TEXT("Opening"); P.HealthThreshold = 1.0f; P.CooldownScale = 1.0f; P.DamageScale = 1.0f; Ph.Add(P);
	}
	{
		FGolemPhaseConfig P; P.PhaseName = TEXT("Enrage"); P.HealthThreshold = 0.4f; P.CooldownScale = 0.7f; P.DamageScale = 1.2f; Ph.Add(P);
	}

	// Breathing room so the player can punish the boss between attacks (exposed in Golem|Pacing).
	GolemBossComponent->GlobalCooldown = 2.5f;
	GolemBossComponent->GlobalCooldownRandom = 0.7f;

	// Weak-point crystals: break BOTH arms during the slam to topple the boss, then crit the head crystal.
	TArray<FGolemWeakPoint>& WP = GolemBossComponent->WeakPoints;
	WP.Reset();
	{
		FGolemWeakPoint C; C.Id = TEXT("ArmL"); C.Kind = EGolemWeakPointKind::Arm;  C.SocketName = TEXT("crystal_arm_l"); C.Health = 120.f; WP.Add(C);
	}
	{
		FGolemWeakPoint C; C.Id = TEXT("ArmR"); C.Kind = EGolemWeakPointKind::Arm;  C.SocketName = TEXT("crystal_arm_r"); C.Health = 120.f; WP.Add(C);
	}
	{
		FGolemWeakPoint C; C.Id = TEXT("Head"); C.Kind = EGolemWeakPointKind::Head; C.SocketName = TEXT("crystal_head");  C.Health = 1.f;   WP.Add(C);
	}
	GolemBossComponent->ToppleDuration = 8.f;
	GolemBossComponent->HeadCritMultiplier = 5.f;
}
