/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AI_Types - Header"
 * Notes: Shared enums and structs for the AI system. All tooltips visible in editor.
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

UENUM(BlueprintType) enum class EAIRank : uint8 { Basic, Elite, Boss };
UENUM(BlueprintType) enum class EAIState : uint8 { Idle, Patrolling, Chasing, Attacking, Returning, Fleeing, Interacting, Staggered, Dead, Investigating };
UENUM(BlueprintType) enum class EPatrolMode : uint8 { Stationary, Zone, Path };
UENUM(BlueprintType) enum class EPatrolLoopMode : uint8 { Loop, PingPong, Once };
UENUM(BlueprintType) enum class EAIAwarenessLevel : uint8 { Unaware, Suspicious, Alert, InCombat };
UENUM(BlueprintType) enum class EAICombatStyle : uint8 { Melee, Ranged, Hybrid };
UENUM(BlueprintType) enum class EAIAttackType : uint8 { LightMelee, HeavyMelee, Ranged, Special, Charged };
UENUM(BlueprintType) enum class EAICombatPhase : uint8 { Phase1, Phase2, Phase3, Enrage };
UENUM(BlueprintType) enum class EAINPCRole : uint8 { None, Dialogue, Merchant, QuestGiver };

UENUM(BlueprintType)
enum class EAIRespawnCondition : uint8
{
	Never       UMETA(ToolTip = "This AI never respawns once killed."),
	OnSave      UMETA(ToolTip = "Respawns when the player saves the game."),
	OnDayCycle  UMETA(ToolTip = "Respawns when a full in-game day passes."),
	OnTimer     UMETA(ToolTip = "Respawns after a configurable timer."),
	OnSaveOrDay UMETA(ToolTip = "Respawns on either a save or a day cycle.")
};

UENUM(BlueprintType)
enum class EAIAnimationMode : uint8
{
	DirectPlayback UMETA(DisplayName = "Direct Playback", ToolTip = "Plays montages directly on the mesh. No ABP needed. Best for creatures/animals."),
	AnimBlueprint  UMETA(DisplayName = "Animation Blueprint", ToolTip = "Uses an ABP with GroundSpeed/Direction variables. Montage_Play for actions. Best for humanoids.")
};

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

	/** Time (seconds) into the attack montage at which the hit window fires. This is when damage/VFX should happen.
	 *  Set to 0 to fire immediately. Set to -1 to use ManualTriggerHitWindow from an AnimNotify instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Seconds into the montage when OnAttackHitWindow fires. Typically 0.3-0.5 for melee swings."))
	float HitWindowTime = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float MinRange = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float ChargeTime = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0")) float ChargeMultiplier = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "360")) float AttackArc = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<EAICombatPhase> AvailableInPhases;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float SelectionWeight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanInterrupt = false;

	/** Recovery (seconds) after the montage ends during which the AI stays rooted and can't act — a punish window for the player. 0 = none. Use it on heavy/committed attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ToolTip = "Rooted, vulnerable window after the attack ends. Bigger = more punishable. Try 0.4-0.9 on heavies.")) float RecoveryTime = 0.f;

	/** Freeze-frame (seconds) applied to the attacker on a connecting hit, for impact weight. 0 = off. Try 0.04-0.10. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "0.5", ToolTip = "Brief hit-stop on connect for punch. 0 = off.")) float HitStopDuration = 0.f;

	/** Knockback impulse pushed onto a hit character along the attack direction. 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ToolTip = "Launch force applied to the victim along the hit direction. 0 = no knockback.")) float KnockbackForce = 0.f;

	/** If true, the auto hit-window damages EVERY valid target in range+arc (cleave/AoE), not just the current target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Cleave/AoE: hit all valid targets in the range+arc, not just the current one.")) bool bMultiTarget = false;

	/** Max victims for a multi-target hit. 0 = unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", EditCondition = "bMultiTarget", ToolTip = "Cap on cleave/AoE victims. 0 = no cap.")) int32 MaxTargets = 0;

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

	/** Adds to summon when this phase begins. Fires OnAIRequestSummon(Count) — spawn them in BP. 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ToolTip = "Minions to summon on entering this phase. Broadcasts OnAIRequestSummon for BP to spawn. 0 = none.")) int32 SummonCount = 0;
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
