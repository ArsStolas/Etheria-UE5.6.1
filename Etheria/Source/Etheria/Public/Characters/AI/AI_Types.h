/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AI_Types - Header"
 * Notes: Shared enums and structs for the AI system.
 */

#pragma once

#include "CoreMinimal.h"
#include "AI_Types.generated.h"

class UAnimMontage;

/* ═══════════ Enums ═══════════ */

UENUM(BlueprintType)
enum class EAIHostilityType : uint8
{
	Passive    UMETA(DisplayName = "Passive"),
	Neutral    UMETA(DisplayName = "Neutral"),
	Aggressive UMETA(DisplayName = "Aggressive")
};

UENUM(BlueprintType)
enum class EAIRank : uint8
{
	Basic UMETA(DisplayName = "Basic"),
	Elite UMETA(DisplayName = "Elite"),
	Boss  UMETA(DisplayName = "Boss")
};

UENUM(BlueprintType)
enum class EAIState : uint8
{
	Idle,
	Patrolling,
	Chasing,
	Attacking,
	Returning,
	Fleeing,
	Interacting,
	Staggered,
	Dead
};

UENUM(BlueprintType)
enum class EPatrolMode : uint8
{
	Stationary,
	Zone,
	Path
};

UENUM(BlueprintType)
enum class EPatrolLoopMode : uint8
{
	Loop,
	PingPong,
	Once
};

UENUM(BlueprintType)
enum class EAIAwarenessLevel : uint8
{
	Unaware,
	Suspicious,
	Alert,
	InCombat
};

UENUM(BlueprintType)
enum class EAICombatStyle : uint8
{
	Melee,
	Ranged,
	Hybrid
};

UENUM(BlueprintType)
enum class EAIAttackType : uint8
{
	LightMelee,
	HeavyMelee,
	Ranged,
	Special,
	Charged
};

UENUM(BlueprintType)
enum class EAICombatPhase : uint8
{
	Phase1,
	Phase2,
	Phase3,
	Enrage
};

UENUM(BlueprintType)
enum class EAIRespawnCondition : uint8
{
	/** Never respawn once dead. */
	Never       UMETA(DisplayName = "Never"),
	/** Respawn when the player saves. */
	OnSave      UMETA(DisplayName = "On Player Save"),
	/** Respawn after a full in-game day. */
	OnDayCycle  UMETA(DisplayName = "On Day Cycle"),
	/** Respawn after a timer. */
	OnTimer     UMETA(DisplayName = "After Timer"),
	/** Respawn on both save and day cycle. */
	OnSaveOrDay UMETA(DisplayName = "On Save or Day Cycle")
};

/* ═══════════ Structs ═══════════ */

USTRUCT(BlueprintType)
struct FAIAttackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName AttackName = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAIAttackType AttackType = EAIAttackType::LightMelee;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UAnimMontage> AttackMontage = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float BaseDamage = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float Range = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float Cooldown = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float MinRange = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float ChargeTime = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0")) float ChargeMultiplier = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "360")) float AttackArc = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<EAICombatPhase> AvailableInPhases;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float SelectionWeight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanInterrupt = false;
	float CurrentCooldown = 0.f;
};

USTRUCT(BlueprintType)
struct FAICombatPhaseData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAICombatPhase Phase = EAICombatPhase::Phase1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0")) float HPThreshold = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float SpeedMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float DamageMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float CooldownMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UAnimMontage> PhaseTransitionMontage = nullptr;
};

USTRUCT(BlueprintType)
struct FAIComboChain
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ComboName = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> AttackIndices;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float ComboWindowDuration = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float SelectionWeight = 1.f;
};
