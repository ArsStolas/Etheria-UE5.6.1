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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAIAttackStarted, const FAIAttackData&, Attack, int32, AttackIndex, UAnimMontage*, Montage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAIAttackHitWindow, const FAIAttackData&, Attack, int32, AttackIndex, AActor*, CurrentTarget);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIAttackEnded, const FAIAttackData&, Attack, bool, bWasInterrupted);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIComboAdvanced, const FAIComboChain&, Combo, int32, StepIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIComboReset, const FAIComboChain&, Combo);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAICombatPhaseChanged, EAICombatPhase, OldPhase, EAICombatPhase, NewPhase);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIChargeStarted, const FAIAttackData&, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIChargeUpdated, float, ChargePercent, const FAIAttackData&, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIChargeReleased, float, FinalChargePercent, const FAIAttackData&, Attack);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIChargeCancelled, const FAIAttackData&, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAICombatEntered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAICombatExited);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIStaggered, float, StaggerDuration);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAIAttackResolved, const FAIAttackData&, Attack, AActor*, Target, bool, bHitConnected);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIRequestSummon, int32, Count);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIBreakStarted, float, Duration);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIBreakEnded);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIPoiseChanged, float, NewPoise, float, MaxPoise);

UCLASS(ClassGroup=(AI), Blueprintable, meta=(BlueprintSpawnableComponent))
class ETHERIA_API UAICombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAICombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="AI|Combat") EAICombatStyle GetCombatStyle() const { return CombatStyle; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") EAICombatPhase GetCurrentPhase() const { return CurrentPhase; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsAttacking() const { return bIsAttacking; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsCharging() const { return bIsCharging; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsInCombat() const { return bIsInCombat; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsStaggered() const { return bIsStaggered; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsInRecovery() const { return bIsInRecovery; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool IsStaggerImmune() const { return StaggerImmunityTimer > 0.f; }
	UFUNCTION(BlueprintPure, Category="AI|Combat|Break") bool IsBroken() const { return bIsBroken; }

	UFUNCTION(BlueprintPure, Category="AI|Combat|Break") float GetPoisePercent() const { return BreakThreshold > 0.f ? FMath::Clamp(CurrentPoise / BreakThreshold, 0.f, 1.f) : 0.f; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetChargePercent() const;
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetGlobalCooldownRemaining() const { return GlobalCooldownTimer; }

	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetTimeUntilHitWindow() const { return (bIsAttacking && bUseAutoHitWindow && !bHitWindowFired) ? HitWindowTimer : 0.f; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool HasHitWindowFired() const { return bHitWindowFired; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") int32 GetCurrentComboStep() const { return CurrentComboStep; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") int32 GetCurrentAttackIndex() const { return CurrentAttackIndex; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") const TArray<FAIAttackData>& GetAttacks() const { return Attacks; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetMeleeRange() const { return MeleeRange; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetRangedRange() const { return RangedRange; }
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetEffectiveAttackRange() const;
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool GetCurrentPhaseData(FAICombatPhaseData& OutData) const;

	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetPhaseDamageMultiplier() const;
	UFUNCTION(BlueprintPure, Category="AI|Combat") float GetPhaseSpeedMultiplier() const;

	UFUNCTION(BlueprintPure, Category="AI|Combat")
	bool GetAttackByIndex(int32 Index, FAIAttackData& OutAttack) const;

	UFUNCTION(BlueprintPure, Category="AI|Combat")
	bool GetAttackByName(FName Name, FAIAttackData& OutAttack, int32& OutIndex) const;

	UFUNCTION(BlueprintCallable, Category="AI|Combat") void SetCombatStyle(EAICombatStyle NewStyle) { CombatStyle = NewStyle; }
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void SetMeleeRange(float Range) { MeleeRange = Range; }
	UFUNCTION(BlueprintCallable, Category="AI|Combat") void SetRangedRange(float Range) { RangedRange = Range; }

	UFUNCTION(BlueprintPure, Category="AI|Combat") bool CanAttack() const;
	UFUNCTION(BlueprintPure, Category="AI|Combat") bool CanUseAttack(int32 AttackIndex, float DistanceToTarget) const;

	UFUNCTION(BlueprintCallable, Category="AI|Combat") int32 SelectBestAttack(float DistanceToTarget);

	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool ExecuteAttack(int32 AttackIndex);

	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool ExecuteAttackByName(FName AttackName);

	UFUNCTION(BlueprintCallable, Category="AI|Combat") bool ExecuteRandomAttack(float DistanceToTarget);

	UFUNCTION(BlueprintCallable, Category="AI|Combat") float PickApproachRange(float CurrentDistance, float& OutMinRange);

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

	UFUNCTION(BlueprintCallable, Category="AI|Combat")
	void ManualTriggerHitWindow();

	UFUNCTION(BlueprintCallable, Category="AI|Combat|Break") void ApplyPoiseDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category="AI|Combat|Break") void Break();

	UFUNCTION(BlueprintCallable, Category="AI|Combat|Break") void EndBreak();

	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events")
	FOnAIAttackStarted OnAIAttackStarted;

	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events")
	FOnAIAttackHitWindow OnAIAttackHitWindow;

	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events")
	FOnAIAttackEnded OnAIAttackEnded;

	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIComboAdvanced OnAIComboAdvanced;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIComboReset OnAIComboReset;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAICombatPhaseChanged OnAICombatPhaseChanged;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIChargeStarted OnAIChargeStarted;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIChargeUpdated OnAIChargeUpdated;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIChargeReleased OnAIChargeReleased;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIChargeCancelled OnAIChargeCancelled;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAICombatEntered OnAICombatEntered;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAICombatExited OnAICombatExited;
	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIStaggered OnAIStaggered;

	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIAttackResolved OnAIAttackResolved;

	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIRequestSummon OnAIRequestSummon;

	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIBreakStarted OnAIBreakStarted;

	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIBreakEnded OnAIBreakEnded;

	UPROPERTY(BlueprintAssignable, Category="AI|Combat|Events") FOnAIPoiseChanged OnAIPoiseChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Style",
		meta=(ToolTip="Melee/Ranged/Hybrid. Affects which range value is used for engagement distance."))
	EAICombatStyle CombatStyle = EAICombatStyle::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Attacks",
		meta=(ToolTip="Define all available attacks. OnAttackHitWindow fires at HitWindowTime for each attack."))
	TArray<FAIAttackData> Attacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Combos") TArray<FAIComboChain> Combos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Phases") TArray<FAICombatPhaseData> Phases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Phases", meta=(ClampMin="0",
		ToolTip="Seconds in combat before forcing Enrage, so a turtling player can't stall a boss forever. 0 = off."))
	float EnrageAfterSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Phases", meta=(ClampMin="0",
		ToolTip="Pause after a phase transition before the next attack — gives the swap a readable beat."))
	float PhaseTransitionPause = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Range", meta=(ClampMin="0")) float MeleeRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Range", meta=(ClampMin="0")) float RangedRange = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Timing", meta=(ClampMin="0",
		ToolTip="Global cooldown between any two attacks. Prevents spam."))
	float GlobalCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Timing", meta=(ClampMin="0"))
	float AttackDelayRandomDeviation = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Combos", meta=(ClampMin="0", ClampMax="1"))
	float ComboChance = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Charged", meta=(ClampMin="0", ClampMax="1"))
	float ChargeAttackChance = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Attacks", meta=(ClampMin="0", ClampMax="0.6",
		ToolTip="Elite+ chance to cancel a wind-up partway and re-attack (feint). Punishes reflex-dodging. 0 = off."))
	float FeintChance = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Timing", meta=(ClampMin="0",
		ToolTip="Extra recovery seconds when the attack hits nothing. Makes baiting a whiff a real opening."))
	float WhiffRecoveryBonus = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Charged", meta=(ClampMin="0", ClampMax="1",
		ToolTip="Min distance (as a fraction of attack range) for the AI to auto-start a charged attack. 0 = charge at any range."))
	float ChargeMinRangeRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Attacks", meta=(ClampMin="0", ClampMax="1",
		ToolTip="How much the just-used attack is discouraged from repeating (0.35 = 65% less likely). 1 = no anti-repeat."))
	float AttackRepeatPenalty = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Stagger") TObjectPtr<UAnimMontage> StaggerMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Stagger", meta=(ClampMin="1",
		ToolTip="How many hits the AI can take before being staggered."))
	int32 StaggerThreshold = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Stagger", meta=(ClampMin="0",
		ToolTip="Seconds of stagger immunity after recovering. Stops chain/perma-stagger. 0 = off."))
	float StaggerImmunityDuration = 0.6f;

	UPROPERTY(BlueprintReadWrite, Category="AI|Combat|Stagger") int32 CurrentHitCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break", meta=(ClampMin="0",
		ToolTip="Poise required to stun/break the boss. Feed via ApplyPoiseDamage. 0 = no break system."))
	float BreakThreshold = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break", meta=(ClampMin="0.1",
		ToolTip="Seconds the boss stays down after breaking — the player's critical window."))
	float BreakDownDuration = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break", meta=(ClampMin="0",
		ToolTip="Poise regenerated per second while not broken. 0 = gauge never drains."))
	float PoiseRegenPerSecond = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break") TObjectPtr<UAnimMontage> BreakMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break") TObjectPtr<UAnimMontage> BreakLoopMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Break") TObjectPtr<UAnimMontage> BreakRecoverMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Timing",
		meta=(ToolTip="Auto mode: hit window fires after HitWindowTime seconds. Manual mode: call ManualTriggerHitWindow from an AnimNotify."))
	bool bUseAutoHitWindow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Damage",
		meta=(ToolTip="When ON, the hit window automatically applies the attack's BaseDamage to the current target if it's within Range and AttackArc. Turn OFF if you apply damage yourself from the OnAIAttackHitWindow event in Blueprint."))
	bool bAutoApplyHitWindowDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Damage",
		meta=(ToolTip="Require an unobstructed trace to the target before auto-applying damage. Prevents hitting through walls."))
	bool bRequireLineOfSightForHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Timing", meta=(ClampMin="0",
		ToolTip="Floor on the wind-up before a hit can land. Makes attacks readable. 0 = off."))
	float MinTelegraphTime = 0.3f;

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
	void TickHitWindow(float DeltaTime);
	void SweepHitWindow();
	void CloseHitWindow();
	void StopChargeLoopMontage();
	void FinishAttack();

	bool ApplyHitDamageTo(AActor* Victim, const FAIAttackData& Atk);
	bool ApplyMultiTargetDamage(const FAIAttackData& Atk);
	void ApplyHitStop(float Duration);
	void ApplyLunge(const FAIAttackData& Atk);
	bool HasPendingCombatWork() const;
	bool IsTargetInHitZone(const AActor* Target, const FAIAttackData& Atk) const;

	float GetSurfaceDistanceToTarget() const;

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
	float CurrentPoise = 0.f;
	float BreakTimer = 0.f;
	float AttackAnimTimer = 0.f;
	float HitWindowTimer = 0.f;
	float RecoveryTimer = 0.f;
	float HitStopTimer = 0.f;
	float SavedAnimRateBeforeHitStop = 1.f;

	float PendingChargeScale = 1.f;
	bool bIsInRecovery = false;
	bool bComboAdvancePending = false;
	float ComboGapTimer = 0.f;
	bool bIsAttacking = false;
	bool bIsCharging = false;
	bool bIsInCombat = false;
	bool bIsStaggered = false;
	bool bIsBroken = false;
	bool bHitWindowFired = false;

	bool bHitWindowActive = false;
	float HitWindowActiveTimer = 0.f;
	bool bHitConnectedThisSwing = false;
	bool bImpactSoundPlayedThisSwing = false;
	TSet<TWeakObjectPtr<AActor>> HitThisSwing;
	int32 MultiTargetHitsThisSwing = 0;

	bool bFeintArmed = false;
	float FeintCancelTimer = 0.f;
};
