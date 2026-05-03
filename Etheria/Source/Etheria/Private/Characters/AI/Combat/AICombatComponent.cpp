/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AICombatComponent - Source"
 * Notes: ExecuteAttack plays the montage and starts a hit window timer.
 *        OnAttackHitWindow fires at HitWindowTime — bind this in BP to do damage/VFX/projectiles.
 *        If bUseAutoHitWindow is false, call ManualTriggerHitWindow from an AnimNotify.
 */

#include "Characters/AI/Combat/AICombatComponent.h"

#include "Characters/AI/BaseAICharacter.h"
#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Animation/AnimMontage.h"

UAICombatComponent::UAICombatComponent() { PrimaryComponentTick.bCanEverTick = true; }

void UAICombatComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ABaseAICharacter>(GetOwner());
}

void UAICombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickCooldowns(DeltaTime);
	TickCharge(DeltaTime);
	TickStagger(DeltaTime);
	TickAttack(DeltaTime);

	if (CurrentComboIndex >= 0 && !bIsAttacking)
	{
		ComboWindowTimer -= DeltaTime;
		if (ComboWindowTimer <= 0.f) ResetCombo();
	}
}

/* ═══════════ Getters ═══════════ */

float UAICombatComponent::GetChargePercent() const
{
	if (!bIsCharging || !Attacks.IsValidIndex(CurrentAttackIndex)) return 0.f;
	const float Max = Attacks[CurrentAttackIndex].ChargeTime;
	return Max > 0.f ? FMath::Clamp(ChargeTimer / Max, 0.f, 1.f) : 0.f;
}

float UAICombatComponent::GetEffectiveAttackRange() const
{
	switch (CombatStyle)
	{
	case EAICombatStyle::Melee:  return MeleeRange;
	case EAICombatStyle::Ranged: return RangedRange;
	case EAICombatStyle::Hybrid:
	{
		float Max = MeleeRange;
		for (const FAIAttackData& A : Attacks)
			if (A.CurrentCooldown <= 0.f) Max = FMath::Max(Max, A.Range);
		return Max;
	}
	}
	return MeleeRange;
}

bool UAICombatComponent::GetCurrentPhaseData(FAICombatPhaseData& OutData) const
{
	for (const FAICombatPhaseData& P : Phases)
		if (P.Phase == CurrentPhase) { OutData = P; return true; }
	return false;
}

bool UAICombatComponent::GetAttackByIndex(int32 Index, FAIAttackData& OutAttack) const
{
	if (!Attacks.IsValidIndex(Index)) return false;
	OutAttack = Attacks[Index];
	return true;
}

bool UAICombatComponent::GetAttackByName(FName Name, FAIAttackData& OutAttack, int32& OutIndex) const
{
	for (int32 i = 0; i < Attacks.Num(); ++i)
	{
		if (Attacks[i].AttackName == Name)
		{
			OutAttack = Attacks[i];
			OutIndex = i;
			return true;
		}
	}
	return false;
}

/* ═══════════ Can Attack ═══════════ */

bool UAICombatComponent::CanAttack() const
{
	if (bIsAttacking || bIsStaggered || !bIsInCombat) return false;
	if (GlobalCooldownTimer > 0.f) return false;
	if (!OwnerCharacter || OwnerCharacter->GetCurrentAIState() == EAIState::Dead) return false;
	return true;
}

bool UAICombatComponent::CanUseAttack(int32 AttackIndex, float DistanceToTarget) const
{
	if (!CanAttack() || !Attacks.IsValidIndex(AttackIndex)) return false;
	const FAIAttackData& A = Attacks[AttackIndex];
	if (A.CurrentCooldown > 0.f || !A.AttackMontage) return false;
	if (DistanceToTarget > A.Range || DistanceToTarget < A.MinRange) return false;
	if (A.AvailableInPhases.Num() > 0 && !A.AvailableInPhases.Contains(CurrentPhase)) return false;
	return true;
}

/* ═══════════ Selection ═══════════ */

int32 UAICombatComponent::SelectBestAttack(float DistanceToTarget)
{
	TArray<TPair<int32, float>> Candidates;
	float TotalWeight = 0.f;
	for (int32 i = 0; i < Attacks.Num(); ++i)
	{
		if (CanUseAttack(i, DistanceToTarget))
		{
			Candidates.Add(TPair<int32, float>(i, Attacks[i].SelectionWeight));
			TotalWeight += Attacks[i].SelectionWeight;
		}
	}
	if (Candidates.Num() == 0) return -1;

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (const auto& [Idx, W] : Candidates)
	{
		Roll -= W;
		if (Roll <= 0.f) return Idx;
	}
	return Candidates.Last().Key;
}

/* ═══════════ Execute ═══════════ */

bool UAICombatComponent::ExecuteAttack(int32 AttackIndex)
{
	if (!Attacks.IsValidIndex(AttackIndex) || !OwnerCharacter) return false;

	FAIAttackData& Atk = Attacks[AttackIndex];
	if (!Atk.AttackMontage) return false;

	// Play montage via animation component
	UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation();
	if (!AnimComp) return false;

	UAnimMontage* Played = AnimComp->PlayActionMontage(Atk.AttackMontage);
	if (!Played) return false;

	bIsAttacking = true;
	bHitWindowFired = false;
	CurrentAttackIndex = AttackIndex;
	AttackAnimTimer = Atk.AttackMontage->GetPlayLength();
	HitWindowTimer = Atk.HitWindowTime;

	// Broadcast start — BP can react (anticipation VFX, sound cues, etc.)
	OnAIAttackStarted.Broadcast(Atk, AttackIndex, Atk.AttackMontage);
	return true;
}

bool UAICombatComponent::ExecuteAttackByName(FName AttackName)
{
	for (int32 i = 0; i < Attacks.Num(); ++i)
		if (Attacks[i].AttackName == AttackName)
			return ExecuteAttack(i);
	return false;
}

bool UAICombatComponent::ExecuteRandomAttack(float DistanceToTarget)
{
	// Try combo
	if (Combos.Num() > 0 && FMath::FRand() < ComboChance && CurrentComboIndex < 0)
		if (ExecuteRandomCombo()) return true;

	// Try charged
	if (FMath::FRand() < ChargeAttackChance)
		for (int32 i = 0; i < Attacks.Num(); ++i)
			if (Attacks[i].ChargeTime > 0.f && CanUseAttack(i, DistanceToTarget))
				return StartChargeAttack(i);

	// Normal
	const int32 Idx = SelectBestAttack(DistanceToTarget);
	return Idx >= 0 ? ExecuteAttack(Idx) : false;
}

/* ═══════════ Hit Window ═══════════ */

void UAICombatComponent::ManualTriggerHitWindow()
{
	if (bIsAttacking && !bHitWindowFired) FireHitWindow();
}

void UAICombatComponent::FireHitWindow()
{
	if (!Attacks.IsValidIndex(CurrentAttackIndex) || !OwnerCharacter) return;

	bHitWindowFired = true;
	AActor* Target = OwnerCharacter->GetCurrentTarget();

	// THIS IS THE KEY EVENT — Blueprint binds here to do damage, spawn VFX, projectiles, AoE, etc.
	OnAIAttackHitWindow.Broadcast(Attacks[CurrentAttackIndex], CurrentAttackIndex, Target);
}

/* ═══════════ Charge ═══════════ */

bool UAICombatComponent::StartChargeAttack(int32 AttackIndex)
{
	if (!CanAttack() || !Attacks.IsValidIndex(AttackIndex) || Attacks[AttackIndex].ChargeTime <= 0.f) return false;
	CurrentAttackIndex = AttackIndex;
	bIsCharging = true;
	ChargeTimer = 0.f;
	OnAIChargeStarted.Broadcast(Attacks[AttackIndex]);
	return true;
}

void UAICombatComponent::ReleaseChargeAttack()
{
	if (!bIsCharging || !Attacks.IsValidIndex(CurrentAttackIndex)) return;
	const float Pct = GetChargePercent();
	bIsCharging = false;
	OnAIChargeReleased.Broadcast(Pct, Attacks[CurrentAttackIndex]);
	ExecuteAttack(CurrentAttackIndex);
}

void UAICombatComponent::CancelCharge() { bIsCharging = false; ChargeTimer = 0.f; CurrentAttackIndex = -1; }

/* ═══════════ Combos ═══════════ */

bool UAICombatComponent::StartCombo(int32 ComboIndex)
{
	if (!CanAttack() || !Combos.IsValidIndex(ComboIndex) || Combos[ComboIndex].AttackIndices.Num() == 0) return false;
	CurrentComboIndex = ComboIndex;
	CurrentComboStep = 0;
	return ExecuteAttack(Combos[ComboIndex].AttackIndices[0]);
}

bool UAICombatComponent::AdvanceCombo()
{
	if (CurrentComboIndex < 0 || !Combos.IsValidIndex(CurrentComboIndex)) return false;
	const FAIComboChain& Combo = Combos[CurrentComboIndex];
	CurrentComboStep++;
	if (CurrentComboStep >= Combo.AttackIndices.Num()) { ResetCombo(); return false; }
	ComboWindowTimer = Combo.ComboWindowDuration;
	OnAIComboAdvanced.Broadcast(Combo, CurrentComboStep);
	return ExecuteAttack(Combo.AttackIndices[CurrentComboStep]);
}

void UAICombatComponent::ResetCombo()
{
	if (CurrentComboIndex >= 0 && Combos.IsValidIndex(CurrentComboIndex))
		OnAIComboReset.Broadcast(Combos[CurrentComboIndex]);
	CurrentComboIndex = -1; CurrentComboStep = -1; ComboWindowTimer = 0.f;
}

bool UAICombatComponent::ExecuteRandomCombo()
{
	if (Combos.Num() == 0) return false;
	float TW = 0.f;
	for (const auto& C : Combos) TW += C.SelectionWeight;
	float Roll = FMath::FRandRange(0.f, TW);
	for (int32 i = 0; i < Combos.Num(); ++i) { Roll -= Combos[i].SelectionWeight; if (Roll <= 0.f) return StartCombo(i); }
	return StartCombo(Combos.Num() - 1);
}

/* ═══════════ Interrupt / Stagger ═══════════ */

void UAICombatComponent::InterruptAttack()
{
	if (!bIsAttacking) return;
	FAIAttackData Atk = Attacks.IsValidIndex(CurrentAttackIndex) ? Attacks[CurrentAttackIndex] : FAIAttackData();
	bIsAttacking = false;
	bHitWindowFired = false;
	AttackAnimTimer = 0.f;

	if (OwnerCharacter)
		if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
			AnimComp->StopCurrentAction();

	OnAIAttackEnded.Broadcast(Atk, true);
	CurrentAttackIndex = -1;
	if (CurrentComboIndex >= 0) ResetCombo();
}

void UAICombatComponent::ApplyStagger(float Duration)
{
	InterruptAttack();
	CancelCharge();
	bIsStaggered = true;
	StaggerTimer = Duration;
	CurrentHitCount = 0;

	if (OwnerCharacter && StaggerMontage)
		if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
			AnimComp->PlayActionMontage(StaggerMontage);

	OnAIStaggered.Broadcast(Duration);
}

/* ═══════════ Combat State ═══════════ */

void UAICombatComponent::EnterCombat() { if (!bIsInCombat) { bIsInCombat = true; OnAICombatEntered.Broadcast(); } }
void UAICombatComponent::ExitCombat() { if (bIsInCombat) { bIsInCombat = false; InterruptAttack(); CancelCharge(); ResetCombo(); OnAICombatExited.Broadcast(); } }

/* ═══════════ Phases ═══════════ */

void UAICombatComponent::SetPhase(EAICombatPhase NewPhase)
{
	if (CurrentPhase == NewPhase) return;
	const EAICombatPhase Old = CurrentPhase;
	CurrentPhase = NewPhase;

	FAICombatPhaseData Data;
	if (GetCurrentPhaseData(Data) && Data.PhaseTransitionMontage && OwnerCharacter)
	{
		InterruptAttack();
		if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
			AnimComp->PlayActionMontage(Data.PhaseTransitionMontage);
	}
	OnAICombatPhaseChanged.Broadcast(Old, NewPhase);
}

void UAICombatComponent::EvaluatePhaseFromHP(float HPPercent)
{
	if (Phases.Num() == 0) return;
	EAICombatPhase Best = EAICombatPhase::Phase1;
	float Lowest = 2.f;
	for (const auto& P : Phases)
		if (HPPercent <= P.HPThreshold && P.HPThreshold < Lowest) { Lowest = P.HPThreshold; Best = P.Phase; }
	SetPhase(Best);
}

/* ═══════════ Private Ticks ═══════════ */

void UAICombatComponent::TickCooldowns(float DeltaTime)
{
	if (GlobalCooldownTimer > 0.f) GlobalCooldownTimer -= DeltaTime;
	const float CDMult = GetPhaseCooldownMultiplier();
	for (FAIAttackData& A : Attacks)
		if (A.CurrentCooldown > 0.f) A.CurrentCooldown -= DeltaTime * CDMult;
}

void UAICombatComponent::TickCharge(float DeltaTime)
{
	if (!bIsCharging) return;
	ChargeTimer += DeltaTime;
	if (Attacks.IsValidIndex(CurrentAttackIndex))
	{
		OnAIChargeUpdated.Broadcast(GetChargePercent(), Attacks[CurrentAttackIndex]);
		if (ChargeTimer >= Attacks[CurrentAttackIndex].ChargeTime) ReleaseChargeAttack();
	}
}

void UAICombatComponent::TickStagger(float DeltaTime)
{
	if (!bIsStaggered) return;
	StaggerTimer -= DeltaTime;
	if (StaggerTimer <= 0.f) bIsStaggered = false;
}

void UAICombatComponent::TickAttack(float DeltaTime)
{
	if (!bIsAttacking) return;

	// Hit window timer (auto mode)
	if (bUseAutoHitWindow && !bHitWindowFired && Attacks.IsValidIndex(CurrentAttackIndex))
	{
		HitWindowTimer -= DeltaTime;
		if (HitWindowTimer <= 0.f)
			FireHitWindow();
	}

	// Attack end timer
	AttackAnimTimer -= DeltaTime;
	if (AttackAnimTimer <= 0.f)
	{
		FAIAttackData& Atk = Attacks[CurrentAttackIndex];
		bIsAttacking = false;
		bHitWindowFired = false;
		Atk.CurrentCooldown = Atk.Cooldown;
		GlobalCooldownTimer = GlobalCooldown + FMath::FRandRange(0.f, AttackDelayRandomDeviation);
		OnAIAttackEnded.Broadcast(Atk, false);
		if (CurrentComboIndex >= 0) AdvanceCombo();
		CurrentAttackIndex = -1;
	}
}

float UAICombatComponent::GetPhaseDamageMultiplier() const { FAICombatPhaseData D; return GetCurrentPhaseData(D) ? D.DamageMultiplier : 1.f; }
float UAICombatComponent::GetPhaseCooldownMultiplier() const { FAICombatPhaseData D; return GetCurrentPhaseData(D) ? D.CooldownMultiplier : 1.f; }
