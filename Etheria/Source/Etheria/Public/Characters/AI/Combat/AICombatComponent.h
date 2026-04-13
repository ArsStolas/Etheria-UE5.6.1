/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AICombatComponent - Header"
 * Notes: Handles melee/ranged combat, combos, charged attacks, boss phases.
 *        Uses UAnimMontage played via AIAnimationComponent — no ABP.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/AI/AI_Types.h"
#include "AICombatComponent.generated.h"

class ABaseAICharacter;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIAttackStarted, const FAIAttackData&, Attack, int32, AttackIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIAttackEnded, const FAIAttackData&, Attack, bool, bWasInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIComboAdvanced, const FAIComboChain&, Combo, int32, StepIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIComboReset, const FAIComboChain&, Combo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAICombatPhaseChanged, EAICombatPhase, OldPhase, EAICombatPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIChargeStarted, const FAIAttackData&, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIChargeUpdated, float, ChargePercent, const FAIAttackData&, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIChargeReleased, float, FinalChargePercent, const FAIAttackData&, Attack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAICombatEntered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAICombatExited);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIStaggered, float, StaggerDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIDamageDealt, AActor*, Target, float, Damage);

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UAICombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAICombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* ═══════════ Getters ═══════════ */

	UFUNCTION(BlueprintPure, Category = "AI|Combat") EAICombatStyle GetCombatStyle() const { return CombatStyle; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") EAICombatPhase GetCurrentPhase() const { return CurrentPhase; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") bool IsAttacking() const { return bIsAttacking; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") bool IsCharging() const { return bIsCharging; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") bool IsInCombat() const { return bIsInCombat; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") bool IsStaggered() const { return bIsStaggered; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") float GetChargePercent() const;
	UFUNCTION(BlueprintPure, Category = "AI|Combat") float GetGlobalCooldownRemaining() const { return GlobalCooldownTimer; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") int32 GetCurrentComboStep() const { return CurrentComboStep; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") const TArray<FAIAttackData>& GetAttacks() const { return Attacks; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") float GetMeleeRange() const { return MeleeRange; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") float GetRangedRange() const { return RangedRange; }
	UFUNCTION(BlueprintPure, Category = "AI|Combat") float GetEffectiveAttackRange() const;
	UFUNCTION(BlueprintPure, Category = "AI|Combat") bool GetCurrentPhaseData(FAICombatPhaseData& OutData) const;

	/* ═══════════ Setters ═══════════ */

	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void SetCombatStyle(EAICombatStyle NewStyle) { CombatStyle = NewStyle; }
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void SetMeleeRange(float Range) { MeleeRange = Range; }
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void SetRangedRange(float Range) { RangedRange = Range; }

	/* ═══════════ Actions ═══════════ */

	UFUNCTION(BlueprintPure, Category = "AI|Combat") bool CanAttack() const;
	UFUNCTION(BlueprintPure, Category = "AI|Combat") bool CanUseAttack(int32 AttackIndex, float DistanceToTarget) const;
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") int32 SelectBestAttack(float DistanceToTarget);
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") bool ExecuteAttack(int32 AttackIndex);
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") bool ExecuteRandomAttack(float DistanceToTarget);
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") bool StartChargeAttack(int32 AttackIndex);
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void ReleaseChargeAttack();
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void CancelCharge();
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") bool StartCombo(int32 ComboIndex);
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") bool AdvanceCombo();
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void ResetCombo();
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") bool ExecuteRandomCombo();
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void InterruptAttack();
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void ApplyStagger(float Duration);
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void EnterCombat();
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void ExitCombat();
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void SetPhase(EAICombatPhase NewPhase);
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void EvaluatePhaseFromHP(float HPPercent);
	UFUNCTION(BlueprintCallable, Category = "AI|Combat") void DealDamageToTarget(AActor* Target, float DamageAmount);

	/* ═══════════ Dispatchers ═══════════ */

	UPROPERTY(BlueprintAssignable) FOnAIAttackStarted OnAIAttackStarted;
	UPROPERTY(BlueprintAssignable) FOnAIAttackEnded OnAIAttackEnded;
	UPROPERTY(BlueprintAssignable) FOnAIComboAdvanced OnAIComboAdvanced;
	UPROPERTY(BlueprintAssignable) FOnAIComboReset OnAIComboReset;
	UPROPERTY(BlueprintAssignable) FOnAICombatPhaseChanged OnAICombatPhaseChanged;
	UPROPERTY(BlueprintAssignable) FOnAIChargeStarted OnAIChargeStarted;
	UPROPERTY(BlueprintAssignable) FOnAIChargeUpdated OnAIChargeUpdated;
	UPROPERTY(BlueprintAssignable) FOnAIChargeReleased OnAIChargeReleased;
	UPROPERTY(BlueprintAssignable) FOnAICombatEntered OnAICombatEntered;
	UPROPERTY(BlueprintAssignable) FOnAICombatExited OnAICombatExited;
	UPROPERTY(BlueprintAssignable) FOnAIStaggered OnAIStaggered;
	UPROPERTY(BlueprintAssignable) FOnAIDamageDealt OnAIDamageDealt;

	/* ═══════════ Config ═══════════ */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Style")
	EAICombatStyle CombatStyle = EAICombatStyle::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Attacks")
	TArray<FAIAttackData> Attacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Combos")
	TArray<FAIComboChain> Combos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Phases")
	TArray<FAICombatPhaseData> Phases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Range", meta = (ClampMin = "0"))
	float MeleeRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Range", meta = (ClampMin = "0"))
	float RangedRange = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Timing", meta = (ClampMin = "0"))
	float GlobalCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Timing", meta = (ClampMin = "0"))
	float AttackDelayRandomDeviation = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Combos", meta = (ClampMin = "0", ClampMax = "1"))
	float ComboChance = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Charged", meta = (ClampMin = "0", ClampMax = "1"))
	float ChargeAttackChance = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Stagger")
	TObjectPtr<UAnimMontage> StaggerMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Stagger", meta = (ClampMin = "1"))
	int32 StaggerThreshold = 3;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Combat|Stagger")
	int32 CurrentHitCount = 0;

protected:
	virtual void BeginPlay() override;

private:
	void TickCooldowns(float DeltaTime);
	void TickCharge(float DeltaTime);
	void TickStagger(float DeltaTime);
	void TickAttack(float DeltaTime);
	float GetPhaseDamageMultiplier() const;
	float GetPhaseCooldownMultiplier() const;

	UPROPERTY()
	TObjectPtr<ABaseAICharacter> OwnerCharacter;

	EAICombatPhase CurrentPhase = EAICombatPhase::Phase1;
	int32 CurrentAttackIndex = -1;
	int32 CurrentComboIndex = -1;
	int32 CurrentComboStep = -1;
	float GlobalCooldownTimer = 0.f;
	float ComboWindowTimer = 0.f;
	float ChargeTimer = 0.f;
	float StaggerTimer = 0.f;
	float AttackAnimTimer = 0.f;
	bool bIsAttacking = false;
	bool bIsCharging = false;
	bool bIsInCombat = false;
	bool bIsStaggered = false;
};
