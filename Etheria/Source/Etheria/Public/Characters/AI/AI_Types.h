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

UENUM(BlueprintType)
enum class EAIHostilityType : uint8
{
	Passive    UMETA(DisplayName = "Passive",    ToolTip = "Will never attack. Can flee if configured."),
	Neutral    UMETA(DisplayName = "Neutral",    ToolTip = "Ignores the player unless attacked or configured to flee."),
	Aggressive UMETA(DisplayName = "Aggressive", ToolTip = "Attacks the player on sight.")
};

UENUM(BlueprintType)
enum class EAIRank : uint8
{
	Basic UMETA(DisplayName = "Basic", ToolTip = "Standard enemy."),
	Elite UMETA(DisplayName = "Elite", ToolTip = "Stronger enemy with more health and damage."),
	Boss  UMETA(DisplayName = "Boss",  ToolTip = "Boss enemy with phases and special attacks.")
};

UENUM(BlueprintType)
enum class EAIState : uint8
{
	Idle, Patrolling, Chasing, Attacking, Returning, Fleeing, Interacting, Staggered, Dead
};

UENUM(BlueprintType)
enum class EPatrolMode : uint8
{
	Stationary UMETA(ToolTip = "AI stays in place."),
	Zone       UMETA(ToolTip = "AI picks random points within a radius."),
	Path       UMETA(ToolTip = "AI follows the spline component attached to it.")
};

UENUM(BlueprintType)
enum class EPatrolLoopMode : uint8 { Loop, PingPong, Once };

UENUM(BlueprintType)
enum class EAIAwarenessLevel : uint8 { Unaware, Suspicious, Alert, InCombat };

UENUM(BlueprintType)
enum class EAICombatStyle : uint8 { Melee, Ranged, Hybrid };

UENUM(BlueprintType)
enum class EAIAttackType : uint8 { LightMelee, HeavyMelee, Ranged, Special, Charged };

UENUM(BlueprintType)
enum class EAICombatPhase : uint8 { Phase1, Phase2, Phase3, Enrage };

UENUM(BlueprintType)
enum class EAIRespawnCondition : uint8
{
	Never       UMETA(ToolTip = "This AI never respawns once killed."),
	OnSave      UMETA(ToolTip = "Respawns when the player saves the game."),
	OnDayCycle  UMETA(ToolTip = "Respawns when a full in-game day passes."),
	OnTimer     UMETA(ToolTip = "Respawns after a configurable timer."),
	OnSaveOrDay UMETA(ToolTip = "Respawns on either a save or a day cycle, whichever comes first.")
};

UENUM(BlueprintType)
enum class EAIAnimationMode : uint8
{
	DirectPlayback UMETA(DisplayName = "Direct Playback", ToolTip = "Plays montages directly on the mesh via PlayAnimation(). No ABP needed. Best for creatures/animals."),
	AnimBlueprint  UMETA(DisplayName = "Animation Blueprint", ToolTip = "Uses an Animation Blueprint with Montage_Play(). Requires an ABP with a DefaultSlot. Best for humanoids.")
};

/* ═══════════ Structs ═══════════ */

USTRUCT(BlueprintType)
struct FAIAttackData
{
	GENERATED_BODY()

	/** Display name for this attack (for debugging and UI). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName AttackName = NAME_None;

	/** Type classification of this attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAIAttackType AttackType = EAIAttackType::LightMelee;

	/** Animation montage to play when this attack executes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	/** Base damage dealt by this attack before phase multipliers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float BaseDamage = 10.f;

	/** Maximum distance to the target for this attack to be usable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float Range = 200.f;

	/** Cooldown in seconds before this attack can be used again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float Cooldown = 2.f;

	/** Minimum distance to the target required (prevents melee at point-blank for ranged). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float MinRange = 0.f;

	/** If > 0, the AI must charge for this duration before releasing the attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float ChargeTime = 0.f;

	/** Damage multiplier applied when fully charged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0")) float ChargeMultiplier = 2.f;

	/** Arc in degrees for melee cone hit detection. 360 = all around. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "360")) float AttackArc = 90.f;

	/** If not empty, this attack is only available during these combat phases. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<EAICombatPhase> AvailableInPhases;

	/** Higher weight = more likely to be randomly selected over other attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float SelectionWeight = 1.f;

	/** If true, this attack can interrupt the AI's current action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanInterrupt = false;

	/** Runtime: remaining cooldown. Not editable. */
	float CurrentCooldown = 0.f;
};

USTRUCT(BlueprintType)
struct FAICombatPhaseData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) EAICombatPhase Phase = EAICombatPhase::Phase1;

	/** HP percentage at which this phase activates (0.5 = 50% HP). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0")) float HPThreshold = 1.f;

	/** Movement speed multiplier during this phase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float SpeedMultiplier = 1.f;

	/** Damage multiplier during this phase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float DamageMultiplier = 1.f;

	/** Cooldown multiplier (< 1 means faster attacks). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float CooldownMultiplier = 1.f;

	/** Montage to play when entering this phase (roar, power-up, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UAnimMontage> PhaseTransitionMontage = nullptr;
};

USTRUCT(BlueprintType)
struct FAIComboChain
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ComboName = NAME_None;

	/** Indices into the Attacks array, played in order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> AttackIndices;

	/** Max time between combo steps before the combo resets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float ComboWindowDuration = 1.5f;

	/** Higher weight = more likely to be randomly selected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float SelectionWeight = 1.f;
};
