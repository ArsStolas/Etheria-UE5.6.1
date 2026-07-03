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

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	GolemBossComponent = CreateDefaultSubobject<UGolemBossComponent>(TEXT("GolemBossComponent"));

	HostilityType = EAIHostilityType::Aggressive;
	Rank = EAIRank::Boss;

	bCanReceiveDamage = false;

	AIControllerClass = AGolemBossController::StaticClass();

	if (UCharacterMovementComponent* MC = GetCharacterMovement())
		MC->bOrientRotationToMovement = false;

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		Capsule->InitCapsuleSize(120.f, 300.f);

	LeashRange = 100000.f;

	BuildDefaultAttacks();
}

void AGolemBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (bImmovable)
		if (UCharacterMovementComponent* MC = GetCharacterMovement())
		{
			MC->StopMovementImmediately();
			MC->DisableMovement();
			MC->GravityScale = 0.f;
			MC->SetComponentTickEnabled(false);
		}

	ImmovableAnchor = GetActorLocation();
	bImmovableAnchorSet = bImmovable;
	if (bImmovable) SetActorTickEnabled(true);
}

void AGolemBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	EnforceImmovable();
}

void AGolemBossCharacter::EnforceImmovable()
{
	if (!bImmovable || !bImmovableAnchorSet) return;

	if (UCharacterMovementComponent* MC = GetCharacterMovement())
	{
		MC->StopMovementImmediately();
		if (MC->MovementMode != MOVE_None) MC->SetMovementMode(MOVE_None);
	}

	if (!GetActorLocation().Equals(ImmovableAnchor, 1.f))
		SetActorLocation(ImmovableAnchor, false, nullptr, ETeleportType::TeleportPhysics);
}

void AGolemBossCharacter::LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride)
{

	if (bImmovable) return;
	Super::LaunchCharacter(LaunchVelocity, bXYOverride, bZOverride);
}

float AGolemBossCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{

	if (GolemBossComponent && DamageAmount > 0.f && !IsDead())
	{
		AActor* Causer = DamageCauser ? DamageCauser : (EventInstigator ? EventInstigator->GetPawn() : nullptr);
		if (GolemBossComponent->RouteBodyHit(DamageAmount, Causer))
			return DamageAmount;
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AGolemBossCharacter::BuildDefaultAttacks()
{
	if (!GolemBossComponent) return;

	GolemBossComponent->ArenaRadius = 2000.f;
	GolemBossComponent->EyeSocketNames = { TEXT("eye_l"), TEXT("eye_r") };
	GolemBossComponent->ThrowSocketName = TEXT("hand_r");

	TArray<FGolemAttackConfig>& Atk = GolemBossComponent->Attacks;
	Atk.Reset();

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

	{
		FGolemAttackConfig A;
		A.AttackId = TEXT("ArenaSweep");
		A.Shape = EGolemHazardShape::SweepLine;
		A.TargetMode = EGolemTargetMode::AtTarget;
		A.WindupDuration = 1.2f; A.ActiveDuration = 1.4f; A.RecoveryDuration = 0.9f; A.Cooldown = 8.f;
		A.Damage = 28.f; A.SweepThickness = 300.f; A.SweepLength = 3000.f; A.DamageInterval = 0.1f;
		A.KnockbackForce = 900.f;
		A.bRequiresAirZoneEscape = true;
		A.MinSafeAltitude = 450.f;
		A.SelectionWeight = 1.0f;
		Atk.Add(A);
	}

	{
		FGolemAttackConfig A;
		A.AttackId = TEXT("TwoHandSlam");
		A.Shape = EGolemHazardShape::GroundFissures;
		A.TargetMode = EGolemTargetMode::ArenaExtremities;
		A.WindupDuration = 1.6f; A.ActiveDuration = 2.0f; A.RecoveryDuration = 1.2f; A.Cooldown = 14.f;
		A.Damage = 22.f; A.DamageInterval = 0.4f; A.ImpactRadius = 300.f;
		A.MinSafeAltitude = 450.f;
		A.ImpactSocketNames = { TEXT("hand_l"), TEXT("hand_r") };
		A.bRequiresAirZoneEscape = true;
		A.bExposesArmWeakPoints = true;
		A.SelectionWeight = 0.7f;
		Atk.Add(A);
	}

	{
		FGolemAttackConfig A;
		A.AttackId = TEXT("EyeLaser");
		A.Shape = EGolemHazardShape::BeamSweep;
		A.TargetMode = EGolemTargetMode::EyeSockets;
		A.WindupDuration = 1.3f; A.ActiveDuration = 1.6f; A.RecoveryDuration = 1.0f; A.Cooldown = 9.f;
		A.Damage = 20.f; A.DamageInterval = 0.15f;
		A.BeamCount = 2; A.BeamLength = 3500.f; A.BeamWidth = 200.f; A.BeamSpreadAngle = 0.f; A.BeamSweepAngle = 0.f;
		A.bBeamRiseFromGround = true; A.BeamSweepStartFraction = 0.15f; A.BeamRiseExtraHeight = 0.f; A.BeamRiseHoldFraction = 0.4f;
		A.bUseMeshBeam = false;
		A.SelectionWeight = 0.9f;
		Atk.Add(A);
	}

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

	TArray<FGolemPhaseConfig>& Ph = GolemBossComponent->Phases;
	Ph.Reset();
	{
		FGolemPhaseConfig P; P.PhaseName = TEXT("Opening"); P.HealthThreshold = 1.0f; P.CooldownScale = 1.0f; P.DamageScale = 1.0f; Ph.Add(P);
	}
	{
		FGolemPhaseConfig P; P.PhaseName = TEXT("Enrage"); P.HealthThreshold = 0.4f; P.CooldownScale = 0.7f; P.DamageScale = 1.2f; Ph.Add(P);
	}

	GolemBossComponent->GlobalCooldown = 2.5f;
	GolemBossComponent->GlobalCooldownRandom = 0.7f;

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
