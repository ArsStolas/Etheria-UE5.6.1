/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: ArsStolas
 * Class: "AICombatComponent - Header"
 * Notes: Blueprint-driven combat. Fill the Attacks array, bind dispatchers in your BP.
 *        OnAttackHitWindow fires at the right moment for you to spawn VFX, projectiles,
 *        do traces, apply damage — all from the Event Graph. No C++ override needed.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/AI/AI_Types.h"
#include "AICombatComponent.generated.h"

class ABaseAICharacter;
class UAnimMontage;

/* ── Dispatchers ── */

/** Fires when an attack starts playing. Use to trigger anticipation VFX, sounds, etc. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAIAttackStarted, const FAIAttackData&, Attack, int32, AttackIndex, UAnimMontage*, Montage);

/** Fires at the hit frame — THIS IS WHERE YOU DEAL DAMAGE, SPAWN VFX, PROJECTILES, ETC. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAIAttackHitWindow, const FAIAttackData&, Attack, int32, AttackIndex, AActor*, CurrentTarget);

/** Fires when an attack ends (completed or interrupted). Use to clean up VFX. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIAttackEnded, const FAIAttackData&, Attack, bool, bWasInterrupted);

/** Fires when a combo advances to the next step. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIComboAdvanced, const FAIComboChain&, Combo, int32, StepIndex);

/** Fires when a combo resets (completed or broken). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIComboReset, const FAIComboChain&, Combo);

/** Fires when the boss phase changes. Use to spawn phase transition VFX, change arena, etc. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAICombatPhaseChanged, EAICombatPhase, OldPhase, EAICombatPhase, NewPhase);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIChargeStarted, const FAIAttackData&, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIChargeUpdated, float, ChargePercent, const FAIAttackData&, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIChargeReleased, float, FinalChargePercent, const FAIAttackData&, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAICombatEntered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAICombatExited);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIStaggered, float, StaggerDuration);

/** Fires right after the hit window resolves, telling you whether it actually connected. Use for impact-vs-whoosh SFX/VFX, and to react to player block/parry in BP. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAIAttackResolved, const FAIAttackData&, Attack, AActor*, Target, bool, bHitConnected);

/** Fires when a phase begins that requests adds (FAICombatPhaseData::SummonCount). Spawn the minions in BP. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIRequestSummon, int32, Count);

/** Fires when the poise/break gauge fills and the boss BREAKS — drop the head, open the mountable crit window, etc. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIBreakStarted, float, Duration);

/** Fires when a break ends and the boss recovers (stands back up). Close the crit window in BP. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIBreakEnded);

/** Fires whenever the poise gauge changes (for a stun bar UI). NewPoise/MaxPoise. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIPoiseChanged, float, NewPoise, float, MaxPoise);

UCLASS(ClassGroup=(AI), Blueprintable, meta=(BlueprintSpawnableComponent))
class ETHERIA_API UAICombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAICombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* ═══════════ Getters ═══════════ */

	UFUNCTION(BlueprintPure, Category="AI|Combat") EAICombatStyle GetCombatStyle() const { return CombatStyle; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") EAICombatPhase GetCurrentPhase() const { return CurrentPhase; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsAttacking() const { return bIsAttacking; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsCharging() const { return bIsCharging; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsInCombat() const { return bIsInCombat; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsStaggered() const { return bIsStaggered; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsInRecovery() const { return bIsInRecovery; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsStaggerImmune() const { return StaggerImmunityTimer > 0.f; }
	UFUNCTION(BlueprintPure, Category="AI|Combat|Break") bool IsBroken() const { return bIsBroken; }
	/** Current poise as a 0..1 fraction of BreakThreshold — drive a stun bar UI with this. 0 if the break system is off. */
	UFUNCTION(BlueprintPure, Category="AI|Combat|Break") float GetPoisePercent() const { return BreakThreshold > 0.f ? FMath::Clamp(CurrentPoise / BreakThreshold, 0.f, 1.f) : 0.f; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetChargePercent() const;
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetGlobalCooldownRemaining() const { return GlobalCooldownTimer; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") int32 GetCurrentComboStep() const { return CurrentComboStep; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") int32 GetCurrentAttackIndex() const { return CurrentAttackIndex; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") const TArray<FAIAttackData>& GetAttacks() const { return Attacks; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetMeleeRange() const { return MeleeRange; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetRangedRange() const { return RangedRange; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetEffectiveAttackRange() const;
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool GetCurrentPhaseData(FAICombatPhaseData& OutData) const;

	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetPhaseDamageMultiplier() const;
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetPhaseSpeedMultiplier() const;

	/** Get the attack data for a specific index. Returns false if invalid. */
	UFUNCTION(BlueprintPure, Category="AI|Combat")
	bool GetAttackByIndex(int32 Index, FAIAttackData& OutAttack) const;

	/** Get the attack data by name. Returns false if not found. */
	UFUNCTION(BlueprintPure, Category="AI|Combat")
	bool GetAttackByName(FName Name, FAIAttackData& OutAttack, int32& OutIndex) const;

	/* ═══════════ Setters ═══════════ */

	UFUNCTION(BlueprintCallable, Category="AI|Combat") void SetCombatStyle(EAICombatStyle NewStyle) { CombatStyle = NewStyle; }
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void SetMeleeRange(float Range) { MeleeRange = Range; }
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void SetRangedRange(float Range) { RangedRange = Range; }

	/* ═══════════ Actions ═══════════ */

	UFUNCTION(BlueprintPure, Category="AI|Combat") bool CanAttack() const;
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool CanUseAttack(int32 AttackIndex, float DistanceToTarget) const;

	/** Pick the best attack for the given distance using weighted random selection. Returns -1 if none. */
	UFUNCTION(BlueprintCallable, Category="AI|Combat") int32 SelectBestAttack(float DistanceToTarget);

	/** Execute a specific attack by index. Plays the montage and sets up the hit window timer. */
	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool ExecuteAttack(int32 AttackIndex);

	/** Execute a specific attack by name. */
	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool ExecuteAttackByName(FName AttackName);

	/** Automatically pick and execute the best attack for the current distance. */
	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool ExecuteRandomAttack(float DistanceToTarget);

	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool StartChargeAttack(int32 AttackIndex);
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void ReleaseChargeAttack();
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void CancelCharge();
	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool StartCombo(int32 ComboIndex);
	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool AdvanceCombo();
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void ResetCombo();
	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool ExecuteRandomCombo();
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void InterruptAttack();
	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool TryInterruptCurrentAttack();
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void ApplyStagger(float Duration);
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void EnterCombat();
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void ExitCombat();
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void SetPhase(EAICombatPhase NewPhase);
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void EvaluatePhaseFromHP(float HPPercent);

	/** Manually trigger the hit window for the current attack (use if you prefer anim notifies over timers). */
	UFUNCTION(BlueprintCallable, Category="AI|Combat")
	void ManualTriggerHitWindow();

	/* ═══════════ Break / Poise (stun bar) ═══════════ */

	/** Add to the poise/break gauge (call from BP when the player hits a weak point / lands a heavy / a parry, etc.).
	 *  When the gauge reaches BreakThreshold the boss BREAKS. No-op if the break system is off or already broken. */
	UFUNCTION(BlueprintCallable, Category="AI|Combat|Break") void ApplyPoiseDamage(float Amount);

	/** Force the boss into the broken/downed state right now (skips the gauge). Use for scripted stuns. */
	UFUNCTION(BlueprintCallable, Category="AI|Combat|Break") void Break();

	/** Force the boss to recover from a break immediately (e.g. the player's crit window expired). */
	UFUNCTION(BlueprintCallable, Category="AI|Combat|Break") void EndBreak();

	/* ═══════════ Dispatchers — BIND THESE IN YOUR BP ═══════════ */

	/** Fires when an attack starts. Use for anticipation VFX, camera shake, etc. */
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events")
	FOnAIAttackStarted OnAIAttackStarted;

	/** THE MAIN EVENT — fires at the hit frame. Spawn VFX, do damage traces, launch projectiles HERE. */
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events")
	FOnAIAttackHitWindow OnAIAttackHitWindow;

	/** Fires when the attack animation ends. Clean up VFX, reset state. */
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events")
	FOnAIAttackEnded OnAIAttackEnded;

	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIComboAdvanced OnAIComboAdvanced;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIComboReset OnAIComboReset;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAICombatPhaseChanged OnAICombatPhaseChanged;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIChargeStarted OnAIChargeStarted;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIChargeUpdated OnAIChargeUpdated;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIChargeReleased OnAIChargeReleased;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAICombatEntered OnAICombatEntered;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAICombatExited OnAICombatExited;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIStaggered OnAIStaggered;

	/** Fires after each hit window with whether it connected. Bind for impact/whoosh feedback and block/parry reactions. */
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIAttackResolved OnAIAttackResolved;

	/** Fires when a phase with SummonCount > 0 begins. Bind in BP to spawn that many adds. */
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIRequestSummon OnAIRequestSummon;

	/** Fires when the boss BREAKS (poise gauge full or Break() called). Bind to drop the head, open a mountable
	 *  crit weak-point, play a roar, slow time, etc. Passes the downed duration. */
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIBreakStarted OnAIBreakStarted;

	/** Fires when the break ends. Bind to retract the weak-point / stand the boss up. */
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIBreakEnded OnAIBreakEnded;

	/** Fires whenever the poise gauge changes. Bind to drive a stun-bar widget. */
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIPoiseChanged OnAIPoiseChanged;

	/* ═══════════ Config ═══════════ */

	/** Combat style determines effective range calculation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Style",
		meta=(ToolTip="Melee/Ranged/Hybrid. Affects which range value is used for engagement distance."))
	EAICombatStyle CombatStyle = EAICombatStyle::Melee;

	/** All attacks this AI can perform. Fill this array — each entry is one attack with its montage, damage, range, and timing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Attacks",
		meta=(ToolTip="Define all available attacks. OnAttackHitWindow fires at HitWindowTime for each attack."))
	TArray<FAIAttackData> Attacks;

	/** Combo chains — sequences of attack indices played in order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Combos") TArray<FAIComboChain> Combos;

	/** Boss phase config. Leave empty for non-boss AI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Phases") TArray<FAICombatPhaseData> Phases;

	/** Force the Enrage phase after this many seconds in combat (anti-stall). 0 = never (HP-only phases). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Phases", meta=(ClampMin="0",
		ToolTip="Seconds in combat before forcing Enrage, so a turtling player can't stall a boss forever. 0 = off."))
	float EnrageAfterSeconds = 0.f;

	/** Brief cooldown injected on a phase change so the new phase's first attack doesn't fire on the same frame as the swap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Phases", meta=(ClampMin="0",
		ToolTip="Pause after a phase transition before the next attack — gives the swap a readable beat."))
	float PhaseTransitionPause = 0.4f;

	/** Maximum melee engagement distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Range", meta=(ClampMin="0")) float MeleeRange = 200.f;

	/** Maximum ranged engagement distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Range", meta=(ClampMin="0")) float RangedRange = 1500.f;

	/** Cooldown applied after ANY attack before the next one can fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Timing", meta=(ClampMin="0",
		ToolTip="Global cooldown between any two attacks. Prevents spam."))
	float GlobalCooldown = 1.0f;

	/** Random extra delay for natural feel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Timing", meta=(ClampMin="0"))
	float AttackDelayRandomDeviation = 0.5f;

	/** Chance (0-1) to attempt a combo instead of a single attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Combos", meta=(ClampMin="0", ClampMax="1"))
	float ComboChance = 0.3f;

	/** Chance (0-1) to attempt a charged attack when available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Charged", meta=(ClampMin="0", ClampMax="1"))
	float ChargeAttackChance = 0.15f;

	/** Autonomous charged attacks only auto-pick when the target is at least this fraction of effective range away
	 *  (so the wind-up has time to matter instead of being a free point-blank hit). 0 = allow point-blank charges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Charged", meta=(ClampMin="0", ClampMax="1",
		ToolTip="Min distance (as a fraction of attack range) for the AI to auto-start a charged attack. 0 = charge at any range."))
	float ChargeMinRangeRatio = 0.5f;

	/** Weight multiplier applied to the LAST-used attack during weighted selection, to discourage (not forbid) repeats.
	 *  Lower = stronger variety. 1 = no anti-repeat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Attacks", meta=(ClampMin="0", ClampMax="1",
		ToolTip="How much the just-used attack is discouraged from repeating (0.35 = 65% less likely). 1 = no anti-repeat."))
	float AttackRepeatPenalty = 0.35f;

	/** Stagger animation played when the AI is staggered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Stagger") TObjectPtr<UAnimMontage> StaggerMontage;

	/** Number of hits before stagger triggers. Resets after each stagger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Stagger", meta=(ClampMin="1",
		ToolTip="How many hits the AI can take before being staggered."))
	int32 StaggerThreshold = 3;

	/** Grace period after a stagger ends during which hits don't accumulate toward another stagger — prevents perma-stunlock. 0 = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Stagger", meta=(ClampMin="0",
		ToolTip="Seconds of stagger immunity after recovering. Stops chain/perma-stagger. 0 = off."))
	float StaggerImmunityDuration = 0.6f;

	/** Current hit count toward stagger. Publicly writable for external systems. */
	UPROPERTY(BlueprintReadWrite, Category="AI|Combat|Stagger") int32 CurrentHitCount = 0;

	/* ── Break / Poise (stun bar) ── */

	/** Poise needed to BREAK the boss. Feed it with ApplyPoiseDamage (e.g. from heavy hits / weak-point shots / parries).
	 *  On break, the boss goes down for BreakDownDuration and fires OnAIBreakStarted. 0 = break system OFF. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break", meta=(ClampMin="0",
		ToolTip="Poise required to stun/break the boss. Feed via ApplyPoiseDamage. 0 = no break system."))
	float BreakThreshold = 0.f;

	/** How long the boss stays broken/downed (the player's window to climb on / land a critical). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break", meta=(ClampMin="0.1",
		ToolTip="Seconds the boss stays down after breaking — the player's critical window."))
	float BreakDownDuration = 6.f;

	/** Poise recovered per second while NOT broken. 0 = the gauge never drains (every break needs a full fill). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break", meta=(ClampMin="0",
		ToolTip="Poise regenerated per second while not broken. 0 = gauge never drains."))
	float PoiseRegenPerSecond = 0.f;

	/** Optional montage played the moment the boss breaks (drop to knees / head slam down). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break") TObjectPtr<UAnimMontage> BreakMontage;

	/** Optional montage played when the boss recovers from a break (gets back up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break") TObjectPtr<UAnimMontage> BreakRecoverMontage;

	/** If true, the hit window is triggered by a timer (HitWindowTime in each attack).
	 *  If false, call ManualTriggerHitWindow() from an AnimNotify in your montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Timing",
		meta=(ToolTip="Auto mode: hit window fires after HitWindowTime seconds. Manual mode: call ManualTriggerHitWindow from an AnimNotify."))
	bool bUseAutoHitWindow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Damage",
		meta=(ToolTip="When ON, the hit window automatically applies the attack's BaseDamage to the current target if it's within Range and AttackArc. Turn OFF if you apply damage yourself from the OnAIAttackHitWindow event in Blueprint."))
	bool bAutoApplyHitWindowDamage = true;

	/** When ON, auto-applied hit-window damage requires a clear line of sight to the target,
	 *  so the AI can't hit through walls. Turn OFF for attacks that should ignore cover (e.g. magic). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Damage",
		meta=(ToolTip="Require an unobstructed trace to the target before auto-applying damage. Prevents hitting through walls."))
	bool bRequireLineOfSightForHit = true;

	/** Minimum wind-up (seconds) before ANY attack's hit window can fire, even if its HitWindowTime is lower.
	 *  Guarantees the player always gets a moment to read the attack. 0 = use each attack's HitWindowTime as-is.
	 *  Never overrides manual mode (HitWindowTime = -1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Timing", meta=(ClampMin="0",
		ToolTip="Floor on the wind-up before a hit can land. Makes attacks readable. 0 = off."))
	float MinTelegraphTime = 0.f;

	/** Cancel an active hit-stop freeze immediately and restore the mesh anim rate. Called on stagger/dormancy/death. */
	void EndHitStop();

protected:
	virtual void BeginPlay() override;

private:
	void TickCooldowns(float DeltaTime);
	void TickCharge(float DeltaTime);
	void TickStagger(float DeltaTime);
	void TickAttack(float DeltaTime);
	void TickRecovery(float DeltaTime);
	void TickBreak(float DeltaTime);
	float GetPhaseCooldownMultiplier() const;
	void FireHitWindow();
	void FinishAttack();
	void ApplyHitDamageTo(AActor* Victim, const FAIAttackData& Atk);
	bool ApplyMultiTargetDamage(const FAIAttackData& Atk);
	void ApplyHitStop(float Duration);
	bool HasPendingCombatWork() const;
	bool IsTargetInHitZone(const AActor* Target, const FAIAttackData& Atk) const;

	/** Bound to the animation component's OnAIAnimEnded — finalizes the attack when its montage actually ends. */
	UFUNCTION() void HandleActionMontageEnded(UAnimMontage* Montage);

	UPROPERTY() TObjectPtr<ABaseAICharacter> OwnerCharacter;

	EAICombatPhase CurrentPhase = EAICombatPhase::Phase1;
	float CombatElapsedTime = 0.f;
	int32 LastSelectedAttack = -1;
	int32 CurrentAttackIndex = -1;
	int32 CurrentComboIndex = -1;
	int32 CurrentComboStep = -1;
	float GlobalCooldownTimer = 0.f;
	float ComboWindowTimer = 0.f;
	float ChargeTimer = 0.f;
	float StaggerTimer = 0.f;
	float StaggerImmunityTimer = 0.f;
	float CurrentPoise = 0.f; // poise accumulated toward a break
	float BreakTimer = 0.f;   // remaining downed time while broken
	float AttackAnimTimer = 0.f;
	float HitWindowTimer = 0.f;
	float RecoveryTimer = 0.f;
	float HitStopTimer = 0.f;
	float SavedAnimRateBeforeHitStop = 1.f; // mesh anim rate captured before a hit-stop freeze (restored on end)
	/** Damage scale applied to the next hit window from a charged release (1 = uncharged). */
	float PendingChargeScale = 1.f;
	bool bIsInRecovery = false;
	bool bComboAdvancePending = false; // a combo step is waiting for the current recovery window to elapse
	bool bIsAttacking = false;
	bool bIsCharging = false;
	bool bIsInCombat = false;
	bool bIsStaggered = false;
	bool bIsBroken = false;
	bool bHitWindowFired = false;
};
