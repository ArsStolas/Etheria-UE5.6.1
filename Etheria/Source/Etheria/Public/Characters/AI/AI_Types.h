/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: ArsStolas
 * Class: "AI_Types - Header"
 * Notes: Shared enums and structs for the AI system. All tooltips visible in editor.
 */

#pragma once

#include "CoreMinimal.h"
#include "AI_Types.generated.h"

class UAnimMontage;
class USoundBase;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EAIHostilityType : uint8
{
	Passive    UMETA(DisplayName = "Passive",    ToolTip = "Will never attack. Can flee if configured."),
	Neutral    UMETA(DisplayName = "Neutral",    ToolTip = "Ignores the player unless attacked or configured to flee."),
	Aggressive UMETA(DisplayName = "Aggressive", ToolTip = "Attacks the player on sight.")
};

UENUM(BlueprintType) enum class EAIRank : uint8 { Basic, Elite, Boss };

UENUM(BlueprintType)
enum class EAIKillability : uint8
{
	Auto       UMETA(DisplayName = "Auto",       ToolTip = "Killable only if this AI fights (Aggressive, or Neutral that fights back). Villagers, passive animals and interactable NPCs are UNTOUCHABLE by the player: swings/arrows/AoE pass through, no lock-on, no damage, no death."),
	Killable   UMETA(DisplayName = "Killable",   ToolTip = "Can always be hit and killed, regardless of hostility."),
	Unkillable UMETA(DisplayName = "Unkillable", ToolTip = "Untouchable by the player (attacks pass through, no lock-on); non-player damage floors at 1 HP — can never die.")
};
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Seconds into the montage when OnAttackHitWindow fires. Typically 0.3-0.5 for melee swings."))
	float HitWindowTime = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1",
		ToolTip = "Active hit duration after the window opens. The zone re-checks per frame; each victim is hit once per swing. 0 = one instant check."))
	float HitWindowDuration = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float MinRange = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) float ChargeTime = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1.0")) float ChargeMultiplier = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "360")) float AttackArc = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<EAICombatPhase> AvailableInPhases;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1")) float SelectionWeight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanInterrupt = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ToolTip = "Rooted, vulnerable window after the attack ends. Bigger = more punishable. Try 0.4-0.9 on heavies.")) float RecoveryTime = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "0.5", ToolTip = "Brief hit-stop on connect for punch. 0 = off.")) float HitStopDuration = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ToolTip = "Launch force applied to the victim along the hit direction. 0 = no knockback.")) float KnockbackForce = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0",
		ToolTip = "Extra vertical tolerance (cm) beyond the capsules for the hit to land. Low = grounded bites can't hit airborne/ledge targets."))
	float VerticalHitSlack = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Small committed forward drive at swing start (melee). Closes the last gap so the hit lands with weight."))
	bool bLungeToTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", EditCondition = "bLungeToTarget",
		ToolTip = "Max burst speed of the swing-start lunge. The burst scales with the remaining gap."))
	float LungeSpeed = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Looping wind-up montage played during a charged attack's ChargeTime — the readable telegraph."))
	TObjectPtr<UAnimMontage> ChargeLoopMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Auto-played at the attacker's location when the swing STARTS — the audio telegraph. Crucial for attacks coming from behind the player."))
	TObjectPtr<USoundBase> WindupSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Auto-played at the victim's location when the hit CONNECTS."))
	TObjectPtr<USoundBase> ImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Auto-spawned at the attacker when the swing starts (dust kick, glow...). Optional."))
	TObjectPtr<UNiagaraSystem> WindupVFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Cleave/AoE: hit all valid targets in the range+arc, not just the current one.")) bool bMultiTarget = false;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1",
		ToolTip = "Cooldown duration scale for this phase. 0.7 = 30% faster attacks; 1 = unchanged.")) float CooldownMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UAnimMontage> PhaseTransitionMontage = nullptr;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1",
		ToolTip = "Gap between combo steps — the AI re-aims during it and the player can read the chain. 0 = instant chain."))
	float InterStepDelay = 0.18f;
};
