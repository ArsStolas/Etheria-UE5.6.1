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

	/** Stagger animation played when the AI is staggered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Stagger") TObjectPtr<UAnimMontage> StaggerMontage;

	/** Number of hits before stagger triggers. Resets after each stagger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Stagger", meta=(ClampMin="1",
		ToolTip="How many hits the AI can take before being staggered."))
	int32 StaggerThreshold = 3;

	/** Current hit count toward stagger. Publicly writable for external systems. */
	UPROPERTY(BlueprintReadWrite, Category="AI|Combat|Stagger") int32 CurrentHitCount = 0;

	/** If true, the hit window is triggered by a timer (HitWindowTime in each attack).
	 *  If false, call ManualTriggerHitWindow() from an AnimNotify in your montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Timing",
		meta=(ToolTip="Auto mode: hit window fires after HitWindowTime seconds. Manual mode: call ManualTriggerHitWindow from an AnimNotify."))
	bool bUseAutoHitWindow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Damage",
		meta=(ToolTip="When ON, the hit window automatically applies the attack's BaseDamage to the current target if it's within Range and AttackArc. Turn OFF if you apply damage yourself from the OnAIAttackHitWindow event in Blueprint."))
	bool bAutoApplyHitWindowDamage = true;

protected:
	virtual void BeginPlay() override;

private:
	void TickCooldowns(float DeltaTime);
	void TickCharge(float DeltaTime);
	void TickStagger(float DeltaTime);
	void TickAttack(float DeltaTime);
	float GetPhaseCooldownMultiplier() const;
	void FireHitWindow();
	bool HasPendingCombatWork() const;
	bool IsTargetInHitZone(const AActor* Target, const FAIAttackData& Atk) const;

	UPROPERTY() TObjectPtr<ABaseAICharacter> OwnerCharacter;

	EAICombatPhase CurrentPhase = EAICombatPhase::Phase1;
	int32 CurrentAttackIndex = -1;
	int32 CurrentComboIndex = -1;
	int32 CurrentComboStep = -1;
	float GlobalCooldownTimer = 0.f;
	float ComboWindowTimer = 0.f;
	float ChargeTimer = 0.f;
	float StaggerTimer = 0.f;
	float AttackAnimTimer = 0.f;
	float HitWindowTimer = 0.f;
	bool bIsAttacking = false;
	bool bIsCharging = false;
	bool bIsInCombat = false;
	bool bIsStaggered = false;
	bool bHitWindowFired = false;
};
