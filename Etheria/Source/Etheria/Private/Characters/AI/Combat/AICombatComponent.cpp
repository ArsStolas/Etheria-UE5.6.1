/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: ArsStolas
 * Class: "AICombatComponent - Source"
 * Notes: ExecuteAttack plays the montage and starts a hit window timer.
 *        OnAttackHitWindow fires at HitWindowTime — bind this in BP to do damage/VFX/projectiles.
 *        If bUseAutoHitWindow is false, call ManualTriggerHitWindow from an AnimNotify.
 */

#include "Characters/AI/Combat/AICombatComponent.h"

#include "Characters/AI/BaseAICharacter.h"
#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"

UAICombatComponent::UAICombatComponent() { PrimaryComponentTick.bCanEverTick = true; }

void UAICombatComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ABaseAICharacter>(GetOwner());

	// Anchor attack-end to the ACTUAL montage end (the anim component reports it in both modes)
	// instead of a free timer that can desync on blends/interrupts. The timer stays as a fallback.
	if (OwnerCharacter)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->OnAIAnimEnded.AddDynamic(this, &UAICombatComponent::HandleActionMontageEnded);

	// Desync packs: offset the first global cooldown per instance so identical enemies don't
	// tick their cooldowns in lockstep and swing on the same frame.
	GlobalCooldownTimer = FMath::FRandRange(0.f, AttackDelayRandomDeviation);
}

void UAICombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Hit-stop: freeze the attacker for a beat on impact, then thaw. Skips all other ticks while frozen.
	if (HitStopTimer > 0.f)
	{
		HitStopTimer -= DeltaTime;
		if (HitStopTimer <= 0.f) EndHitStop();
		else return;
	}

	// Anti-stall: force Enrage after a configured time in combat (runs independently of pending work).
	if (bIsInCombat && EnrageAfterSeconds > 0.f && CurrentPhase != EAICombatPhase::Enrage)
	{
		CombatElapsedTime += DeltaTime;
		if (CombatElapsedTime >= EnrageAfterSeconds) SetPhase(EAICombatPhase::Enrage);
	}

	// Break/poise gauge: downed countdown + passive regen (runs independently of other pending work).
	TickBreak(DeltaTime);

	if (!HasPendingCombatWork()) return;

	TickCooldowns(DeltaTime);
	TickCharge(DeltaTime);
	TickStagger(DeltaTime);
	TickAttack(DeltaTime);
	TickRecovery(DeltaTime);

	if (CurrentComboIndex >= 0 && !bIsAttacking)
	{
		ComboWindowTimer -= DeltaTime;
		if (ComboWindowTimer <= 0.f) ResetCombo();
	}
}

bool UAICombatComponent::HasPendingCombatWork() const
{
	if (bIsAttacking || bIsCharging || bIsStaggered || bIsInRecovery) return true;
	if (StaggerImmunityTimer > 0.f) return true;
	if (GlobalCooldownTimer > 0.f) return true;
	if (CurrentComboIndex >= 0) return true;
	for (const FAIAttackData& A : Attacks)
		if (A.CurrentCooldown > 0.f) return true;
	return false;
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
	float MaxAttack = 0.f;
	for (const FAIAttackData& A : Attacks)
		if (A.AttackMontage) MaxAttack = FMath::Max(MaxAttack, A.Range);
	if (MaxAttack > 0.f) return MaxAttack;

	switch (CombatStyle)
	{
	case EAICombatStyle::Melee:  return MeleeRange;
	case EAICombatStyle::Ranged: return RangedRange;
	case EAICombatStyle::Hybrid: return FMath::Max(MeleeRange, RangedRange);
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
	if (bIsAttacking || bIsStaggered || bIsBroken || bIsInRecovery || !bIsInCombat) return false;
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
			float W = Attacks[i].SelectionWeight;
			if (i == LastSelectedAttack) W *= AttackRepeatPenalty; // anti-repeat: discourage (not forbid) spamming the same move
			Candidates.Add(TPair<int32, float>(i, W));
			TotalWeight += W;
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

	if (bIsAttacking) return false;

	FAIAttackData& Atk = Attacks[AttackIndex];
	if (!Atk.AttackMontage) return false;

	// Play montage via animation component
	UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation();
	if (!AnimComp) return false;

	UAnimMontage* Played = AnimComp->PlayActionMontage(Atk.AttackMontage);
	if (!Played) return false;

	bIsAttacking = true;
	bHitWindowFired = false;
	bIsInRecovery = false; // a new swing cancels any pending recovery (e.g. combo chaining)
	RecoveryTimer = 0.f;
	bComboAdvancePending = false;
	PendingChargeScale = 1.f; // reset; a charged release re-sets this right after ExecuteAttack returns
	CurrentAttackIndex = AttackIndex;
	LastSelectedAttack = AttackIndex; // anti-repeat bookkeeping
	// Timer is now only a SAFETY FALLBACK — the montage-end callback (HandleActionMontageEnded) normally ends the attack.
	AttackAnimTimer = Atk.AttackMontage->GetPlayLength() + 0.5f;
	// Floor the wind-up to MinTelegraphTime so the hit can't land instantly, but never touch manual mode (-1).
	HitWindowTimer = (Atk.HitWindowTime < 0.f) ? Atk.HitWindowTime : FMath::Max(Atk.HitWindowTime, MinTelegraphTime);

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
	// Rank gates the fancy autonomous behaviours: Basic enemies only throw single attacks,
	// Elite/Boss may combo and charge. (Designers can still drive combos explicitly via StartCombo.)
	const bool bElitePlus = OwnerCharacter && static_cast<uint8>(OwnerCharacter->GetRank()) >= static_cast<uint8>(EAIRank::Elite);

	// Try combo
	if (bElitePlus && Combos.Num() > 0 && FMath::FRand() < ComboChance && CurrentComboIndex < 0)
		if (ExecuteRandomCombo()) return true;

	// Try charged — only when the target is at mid/far range, so the wind-up has time to matter
	// (avoids charging point-blank where it's just a free hit on the player).
	if (bElitePlus && FMath::FRand() < ChargeAttackChance && DistanceToTarget >= GetEffectiveAttackRange() * ChargeMinRangeRatio)
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
	const FAIAttackData& Atk = Attacks[CurrentAttackIndex];
	AActor* Target = OwnerCharacter->GetCurrentTarget();

	bool bConnected = false;
	if (bAutoApplyHitWindowDamage && Atk.BaseDamage > 0.f)
	{
		if (Atk.bMultiTarget)
		{
			bConnected = ApplyMultiTargetDamage(Atk); // cleave/AoE: every valid target in range+arc
		}
		else if (Target && IsTargetInHitZone(Target, Atk))
		{
			ApplyHitDamageTo(Target, Atk);
			bConnected = true;
		}
	}
	else
	{
		bConnected = Target && IsTargetInHitZone(Target, Atk); // report connection even when BP applies damage itself
	}

	// Freeze-frame for weight (attacker side; the victim-side juice is left to BP via the dispatchers).
	if (bConnected && Atk.HitStopDuration > 0.f)
		ApplyHitStop(Atk.HitStopDuration);

	OnAIAttackHitWindow.Broadcast(Atk, CurrentAttackIndex, Target);
	OnAIAttackResolved.Broadcast(Atk, Target, bConnected);
}

void UAICombatComponent::ApplyHitDamageTo(AActor* Victim, const FAIAttackData& Atk)
{
	if (!Victim || !OwnerCharacter) return;
	const float Damage = Atk.BaseDamage * GetPhaseDamageMultiplier() * PendingChargeScale;
	const FVector HitDir = (Victim->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();

	// Point damage carries the direction, so the victim can play a directional hit reaction.
	UGameplayStatics::ApplyPointDamage(Victim, Damage, HitDir, FHitResult(),
		OwnerCharacter->GetController(), OwnerCharacter, UDamageType::StaticClass());

	// Knockback along the hit direction (small upward component so it reads as a "pop").
	if (Atk.KnockbackForce > 0.f)
		if (ACharacter* HitChar = Cast<ACharacter>(Victim))
			HitChar->LaunchCharacter(HitDir * Atk.KnockbackForce + FVector(0.f, 0.f, Atk.KnockbackForce * 0.15f), false, false);
}

bool UAICombatComponent::ApplyMultiTargetDamage(const FAIAttackData& Atk)
{
	UWorld* W = GetWorld();
	if (!W) return false;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);
	W->OverlapMultiByObjectType(Overlaps, OwnerCharacter->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
		FCollisionShape::MakeSphere(Atk.Range + 100.f), Params);

	const int32 Cap = (Atk.MaxTargets > 0) ? Atk.MaxTargets : MAX_int32;
	int32 Hits = 0;
	TSet<AActor*> AlreadyHit; // an actor with multiple Pawn primitives must only take one hit
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* V = O.GetActor();
		if (!V || AlreadyHit.Contains(V) || !OwnerCharacter->IsValidTargetCandidate(V)) continue; // valid targets only (e.g. the player)
		if (!IsTargetInHitZone(V, Atk)) continue;
		AlreadyHit.Add(V);
		ApplyHitDamageTo(V, Atk);
		if (++Hits >= Cap) break;
	}
	return Hits > 0;
}

bool UAICombatComponent::IsTargetInHitZone(const AActor* Target, const FAIAttackData& Atk) const
{
	if (!Target || !OwnerCharacter) return false;

	const FVector OwnerLoc = OwnerCharacter->GetActorLocation();
	const FVector TgtLoc = Target->GetActorLocation();

	float Reach = Atk.Range;
	if (const ACharacter* C = Cast<ACharacter>(Target))
		if (const UCapsuleComponent* Cap = C->GetCapsuleComponent())
			Reach += Cap->GetScaledCapsuleRadius();

	if (FVector::DistSquared2D(OwnerLoc, TgtLoc) > Reach * Reach)
		return false;

	// Arc check (skipped for 360° attacks)
	if (Atk.AttackArc < 360.f)
	{
		const FVector ToTarget = (TgtLoc - OwnerLoc).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero())
		{
			const float Dot = FVector::DotProduct(OwnerCharacter->GetActorForwardVector().GetSafeNormal2D(), ToTarget);
			const float HalfArcCos = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Atk.AttackArc, 0.f, 360.f) * 0.5f));
			if (Dot < HalfArcCos)
				return false;
		}
	}

	// Line-of-sight: don't let auto-applied damage pass through walls.
	if (bRequireLineOfSightForHit)
		if (const UWorld* W = GetWorld())
		{
			FHitResult Block;
			FCollisionQueryParams Params(TEXT("AIHitWindowLoS"), false, OwnerCharacter);
			Params.AddIgnoredActor(Target);
			const FVector Start = OwnerLoc + FVector(0.f, 0.f, 50.f);
			const FVector End   = TgtLoc  + FVector(0.f, 0.f, 50.f);
			if (W->LineTraceSingleByChannel(Block, Start, End, ECollisionChannel::ECC_Visibility, Params))
				return false; // something solid sits between us and the target
		}

	return true;
}

void UAICombatComponent::ApplyHitStop(float Duration)
{
	if (OwnerCharacter)
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		{
			// Capture whatever another system (e.g. UCombatComponent parry slow-mo) left, but only when not
			// already frozen, so a second hit during the freeze doesn't save 0.01 as the restore value.
			if (HitStopTimer <= 0.f) SavedAnimRateBeforeHitStop = Mesh->GlobalAnimRateScale;
			Mesh->GlobalAnimRateScale = 0.01f; // near-freeze the attacker's animation for the beat
		}
	HitStopTimer = Duration;
}

void UAICombatComponent::EndHitStop()
{
	HitStopTimer = 0.f;
	if (OwnerCharacter)
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
			Mesh->GlobalAnimRateScale = SavedAnimRateBeforeHitStop; // restore the exact prior rate, not a hardcoded 1.f
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
	const float Pct = GetChargePercent(); // must read before clearing bIsCharging
	bIsCharging = false;
	const int32 Idx = CurrentAttackIndex;
	OnAIChargeReleased.Broadcast(Pct, Attacks[Idx]);
	// ExecuteAttack resets PendingChargeScale to 1, so scale damage by the charge AFTER it returns.
	if (ExecuteAttack(Idx))
		PendingChargeScale = FMath::Lerp(1.f, Attacks[Idx].ChargeMultiplier, Pct);
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
	CurrentComboIndex = -1; CurrentComboStep = -1; ComboWindowTimer = 0.f; bComboAdvancePending = false;
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

bool UAICombatComponent::TryInterruptCurrentAttack()
{
	if (!bIsAttacking) return false;
	if (Attacks.IsValidIndex(CurrentAttackIndex) && !Attacks[CurrentAttackIndex].bCanInterrupt)
		return false;
	InterruptAttack();
	return true;
}

void UAICombatComponent::ApplyStagger(float Duration)
{
	EndHitStop(); // a stagger overrides our own hit-stop freeze, else the stagger montage plays at 0.01x and TickStagger is skipped
	InterruptAttack();
	CancelCharge();
	bIsStaggered = true;
	StaggerTimer = Duration;
	CurrentHitCount = 0;

	if (OwnerCharacter && !OwnerCharacter->IsDead())
		OwnerCharacter->SetAIState(EAIState::Staggered);

	if (OwnerCharacter && StaggerMontage)
		if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
			AnimComp->PlayActionMontage(StaggerMontage);

	OnAIStaggered.Broadcast(Duration);
}

/* ═══════════ Break / Poise ═══════════ */

void UAICombatComponent::ApplyPoiseDamage(float Amount)
{
	if (BreakThreshold <= 0.f || bIsBroken || Amount <= 0.f) return;
	CurrentPoise = FMath::Min(CurrentPoise + Amount, BreakThreshold);
	OnAIPoiseChanged.Broadcast(CurrentPoise, BreakThreshold);
	if (CurrentPoise >= BreakThreshold) Break();
}

void UAICombatComponent::Break()
{
	if (bIsBroken) return;
	EndHitStop();
	InterruptAttack();
	CancelCharge();
	bIsInRecovery = false;
	RecoveryTimer = 0.f;
	bIsBroken = true;
	BreakTimer = BreakDownDuration;
	CurrentPoise = 0.f;
	OnAIPoiseChanged.Broadcast(CurrentPoise, BreakThreshold);

	if (OwnerCharacter && BreakMontage && !OwnerCharacter->IsDead())
		if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
			AnimComp->PlayActionMontage(BreakMontage);

	OnAIBreakStarted.Broadcast(BreakDownDuration);
}

void UAICombatComponent::EndBreak()
{
	if (!bIsBroken) return;
	bIsBroken = false;
	BreakTimer = 0.f;

	if (OwnerCharacter && BreakRecoverMontage && !OwnerCharacter->IsDead())
		if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
			AnimComp->PlayActionMontage(BreakRecoverMontage);

	OnAIBreakEnded.Broadcast();
}

void UAICombatComponent::TickBreak(float DeltaTime)
{
	if (bIsBroken)
	{
		BreakTimer -= DeltaTime;
		if (BreakTimer <= 0.f) EndBreak();
		return;
	}
	// Passive poise regen while up.
	if (PoiseRegenPerSecond > 0.f && CurrentPoise > 0.f && BreakThreshold > 0.f)
	{
		CurrentPoise = FMath::Max(0.f, CurrentPoise - PoiseRegenPerSecond * DeltaTime);
		OnAIPoiseChanged.Broadcast(CurrentPoise, BreakThreshold);
	}
}

/* ═══════════ Combat State ═══════════ */

void UAICombatComponent::EnterCombat() { if (!bIsInCombat) { bIsInCombat = true; CombatElapsedTime = 0.f; OnAICombatEntered.Broadcast(); } }
void UAICombatComponent::ExitCombat() { if (bIsInCombat) { bIsInCombat = false; InterruptAttack(); CancelCharge(); ResetCombo(); bIsInRecovery = false; RecoveryTimer = 0.f; EndHitStop(); OnAICombatExited.Broadcast(); } }

/* ═══════════ Phases ═══════════ */

void UAICombatComponent::SetPhase(EAICombatPhase NewPhase)
{
	if (CurrentPhase == NewPhase) return;
	const EAICombatPhase Old = CurrentPhase;
	CurrentPhase = NewPhase;

	// Readable beat: don't let the new phase's first attack fire on the same frame the stats swapped.
	GlobalCooldownTimer = FMath::Max(GlobalCooldownTimer, PhaseTransitionPause);

	FAICombatPhaseData Data;
	const bool bHasData = GetCurrentPhaseData(Data);
	if (bHasData && Data.PhaseTransitionMontage && OwnerCharacter)
	{
		InterruptAttack();
		if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
			AnimComp->PlayActionMontage(Data.PhaseTransitionMontage);
	}

	OnAICombatPhaseChanged.Broadcast(Old, NewPhase);

	if (bHasData && Data.SummonCount > 0)
		OnAIRequestSummon.Broadcast(Data.SummonCount); // BP spawns the adds
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
	const float CDMult = GetPhaseCooldownMultiplier();
	// Scale the GLOBAL cooldown by phase too — otherwise the dominant barrier ignores Enrage/phase speedups.
	if (GlobalCooldownTimer > 0.f) GlobalCooldownTimer -= DeltaTime * CDMult;
	for (FAIAttackData& A : Attacks)
		if (A.CurrentCooldown > 0.f) A.CurrentCooldown -= DeltaTime * CDMult;
}

void UAICombatComponent::TickCharge(float DeltaTime)
{
	if (!bIsCharging) return;
	ChargeTimer += DeltaTime;
	if (Attacks.IsValidIndex(CurrentAttackIndex))
	{
		const FAIAttackData& Atk = Attacks[CurrentAttackIndex];
		OnAIChargeUpdated.Broadcast(GetChargePercent(), Atk);
		if (ChargeTimer >= Atk.ChargeTime)
		{
			// Only commit the charged swing if it can actually land; otherwise drop the wind-up so the heavy
			// doesn't auto-fire into empty air on a fixed timer (and waste its cooldown).
			AActor* Tgt = OwnerCharacter ? OwnerCharacter->GetCurrentTarget() : nullptr;
			if (Tgt && IsTargetInHitZone(Tgt, Atk)) ReleaseChargeAttack();
			else CancelCharge();
		}
	}
}

void UAICombatComponent::TickStagger(float DeltaTime)
{
	if (bIsStaggered)
	{
		StaggerTimer -= DeltaTime;
		if (StaggerTimer <= 0.f)
		{
			bIsStaggered = false;
			StaggerImmunityTimer = StaggerImmunityDuration; // brief grace so hits can't perma-stagger
		}
		return;
	}
	if (StaggerImmunityTimer > 0.f) StaggerImmunityTimer -= DeltaTime;
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

	// Fallback end timer — the montage-end callback (HandleActionMontageEnded) normally finishes first.
	AttackAnimTimer -= DeltaTime;
	if (AttackAnimTimer <= 0.f)
		FinishAttack();
}

void UAICombatComponent::FinishAttack()
{
	if (!bIsAttacking) return;
	if (!Attacks.IsValidIndex(CurrentAttackIndex)) { bIsAttacking = false; AttackAnimTimer = 0.f; return; }

	// Safety net: hit window never fired (manual mode + missing notify, or HitWindowTime > montage length).
	if (!bHitWindowFired)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Attack '%s' ended without a hit window — firing fallback. Check bUseAutoHitWindow / the montage AnimNotify."),
			OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("AI"),
			*Attacks[CurrentAttackIndex].AttackName.ToString());
		FireHitWindow();
	}

	FAIAttackData& Atk = Attacks[CurrentAttackIndex];
	bIsAttacking = false;
	bHitWindowFired = false;
	AttackAnimTimer = 0.f;
	Atk.CurrentCooldown = Atk.Cooldown;
	GlobalCooldownTimer = GlobalCooldown + FMath::FRandRange(0.f, AttackDelayRandomDeviation);

	// Rooted recovery window so committed attacks are punishable.
	if (Atk.RecoveryTime > 0.f) { bIsInRecovery = true; RecoveryTimer = Atk.RecoveryTime; }

	OnAIAttackEnded.Broadcast(Atk, false);

	const bool bInCombo = (CurrentComboIndex >= 0);
	CurrentAttackIndex = -1;
	if (bInCombo)
	{
		// Honor the recovery window BETWEEN combo steps: if this step set a rooted window, wait it out
		// (TickRecovery fires the next step) so mid-combo heavies stay punishable; else chain immediately.
		if (bIsInRecovery) bComboAdvancePending = true;
		else AdvanceCombo();
	}
}

void UAICombatComponent::TickRecovery(float DeltaTime)
{
	if (!bIsInRecovery) return;
	RecoveryTimer -= DeltaTime;
	if (RecoveryTimer <= 0.f)
	{
		bIsInRecovery = false; RecoveryTimer = 0.f;
		if (bComboAdvancePending) { bComboAdvancePending = false; AdvanceCombo(); } // resume the combo after the punish window
	}
}

void UAICombatComponent::HandleActionMontageEnded(UAnimMontage* Montage)
{
	// Only react to OUR attack montage ending. Idle/hit/death montages and already-finalized
	// attacks (bIsAttacking == false, e.g. interrupted) are ignored.
	if (!bIsAttacking || !Attacks.IsValidIndex(CurrentAttackIndex)) return;
	if (Montage && Montage != Attacks[CurrentAttackIndex].AttackMontage) return;
	FinishAttack();
}

float UAICombatComponent::GetPhaseDamageMultiplier() const { FAICombatPhaseData D; return GetCurrentPhaseData(D) ? D.DamageMultiplier : 1.f; }
float UAICombatComponent::GetPhaseCooldownMultiplier() const { FAICombatPhaseData D; return GetCurrentPhaseData(D) ? D.CooldownMultiplier : 1.f; }
float UAICombatComponent::GetPhaseSpeedMultiplier() const { FAICombatPhaseData D; return GetCurrentPhaseData(D) ? D.SpeedMultiplier : 1.f; }
