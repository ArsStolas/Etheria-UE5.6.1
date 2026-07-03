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
#include "Components/Combat/CombatComponent.h"
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

	if (OwnerCharacter)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->OnAIAnimEnded.AddDynamic(this, &UAICombatComponent::HandleActionMontageEnded);

	GlobalCooldownTimer = FMath::FRandRange(0.f, AttackDelayRandomDeviation);
}

void UAICombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (HitStopTimer > 0.f)
	{
		HitStopTimer -= DeltaTime;
		if (HitStopTimer <= 0.f) EndHitStop();
		else return;
	}

	if (bIsInCombat && EnrageAfterSeconds > 0.f && CurrentPhase != EAICombatPhase::Enrage)
	{
		CombatElapsedTime += DeltaTime;
		if (CombatElapsedTime >= EnrageAfterSeconds) SetPhase(EAICombatPhase::Enrage);
	}

	TickBreak(DeltaTime);

	if (!HasPendingCombatWork()) return;

	TickCooldowns(DeltaTime);
	TickCharge(DeltaTime);
	TickStagger(DeltaTime);
	TickAttack(DeltaTime);
	TickHitWindow(DeltaTime);
	TickRecovery(DeltaTime);

	if (bComboAdvancePending && !bIsInRecovery && !bIsAttacking && !bIsStaggered && !bIsBroken && ComboGapTimer > 0.f)
	{
		ComboGapTimer -= DeltaTime;
		if (ComboGapTimer <= 0.f) { bComboAdvancePending = false; AdvanceCombo(); }
	}

	if (CurrentComboIndex >= 0 && !bIsAttacking && !bIsInRecovery && !bComboAdvancePending)
	{
		ComboWindowTimer -= DeltaTime;
		if (ComboWindowTimer <= 0.f) ResetCombo();
	}
}

bool UAICombatComponent::HasPendingCombatWork() const
{
	if (bIsAttacking || bIsCharging || bIsStaggered || bIsInRecovery || bHitWindowActive) return true;
	if (StaggerImmunityTimer > 0.f) return true;
	if (GlobalCooldownTimer > 0.f) return true;
	if (CurrentComboIndex >= 0) return true;
	for (const FAIAttackData& A : Attacks)
		if (A.CurrentCooldown > 0.f) return true;
	return false;
}

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

int32 UAICombatComponent::SelectBestAttack(float DistanceToTarget)
{
	TArray<TPair<int32, float>> Candidates;
	float TotalWeight = 0.f;
	for (int32 i = 0; i < Attacks.Num(); ++i)
	{
		if (CanUseAttack(i, DistanceToTarget))
		{
			float W = Attacks[i].SelectionWeight;
			if (i == LastSelectedAttack) W *= AttackRepeatPenalty;
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

bool UAICombatComponent::ExecuteAttack(int32 AttackIndex)
{
	if (!Attacks.IsValidIndex(AttackIndex) || !OwnerCharacter) return false;

	if (bIsAttacking) return false;

	FAIAttackData& Atk = Attacks[AttackIndex];
	if (!Atk.AttackMontage) return false;

	UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation();
	if (!AnimComp) return false;

	UAnimMontage* Played = AnimComp->PlayActionMontage(Atk.AttackMontage);
	if (!Played) return false;

	bIsAttacking = true;
	bHitWindowFired = false;
	bIsInRecovery = false;
	RecoveryTimer = 0.f;
	bComboAdvancePending = false;
	ComboGapTimer = 0.f;
	PendingChargeScale = 1.f;
	bHitWindowActive = false;
	HitWindowActiveTimer = 0.f;
	bHitConnectedThisSwing = false;
	HitThisSwing.Reset();
	MultiTargetHitsThisSwing = 0;
	bFeintArmed = false;
	CurrentAttackIndex = AttackIndex;
	LastSelectedAttack = AttackIndex;

	AttackAnimTimer = Atk.AttackMontage->GetPlayLength() + 0.5f;

	HitWindowTimer = (Atk.HitWindowTime < 0.f) ? Atk.HitWindowTime : FMath::Max(Atk.HitWindowTime, MinTelegraphTime);

	ApplyLunge(Atk);

	OnAIAttackStarted.Broadcast(Atk, AttackIndex, Atk.AttackMontage);
	return true;
}

void UAICombatComponent::ApplyLunge(const FAIAttackData& Atk)
{
	if (!Atk.bLungeToTarget || Atk.LungeSpeed <= 0.f || !OwnerCharacter) return;
	if (Atk.AttackType != EAIAttackType::LightMelee && Atk.AttackType != EAIAttackType::HeavyMelee) return;
	AActor* T = OwnerCharacter->GetCurrentTarget();
	if (!T) return;

	FVector To = T->GetActorLocation() - OwnerCharacter->GetActorLocation();
	To.Z = 0.f;
	float Reach = 0.f;
	if (const UCapsuleComponent* MyCap = OwnerCharacter->GetCapsuleComponent()) Reach += MyCap->GetScaledCapsuleRadius();
	if (const ACharacter* C = Cast<ACharacter>(T))
		if (const UCapsuleComponent* Cap = C->GetCapsuleComponent()) Reach += Cap->GetScaledCapsuleRadius();

	const float Gap = To.Size() - Reach;
	if (Gap < 15.f) return;

	const float Speed = FMath::Min(Atk.LungeSpeed, Gap * 4.f);
	OwnerCharacter->LaunchCharacter(To.GetSafeNormal2D() * Speed, true, false);
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

	const bool bElitePlus = OwnerCharacter && static_cast<uint8>(OwnerCharacter->GetRank()) >= static_cast<uint8>(EAIRank::Elite);

	const float EffComboChance = bElitePlus ? ComboChance : ComboChance * 0.5f;
	if (Combos.Num() > 0 && FMath::FRand() < EffComboChance && CurrentComboIndex < 0)
		if (ExecuteRandomCombo()) return true;

	if (bElitePlus && FMath::FRand() < ChargeAttackChance && DistanceToTarget >= GetEffectiveAttackRange() * ChargeMinRangeRatio)
		for (int32 i = 0; i < Attacks.Num(); ++i)
			if (Attacks[i].ChargeTime > 0.f && CanUseAttack(i, DistanceToTarget))
				return StartChargeAttack(i);

	const int32 Idx = SelectBestAttack(DistanceToTarget);
	if (Idx < 0 || !ExecuteAttack(Idx)) return false;

	if (bElitePlus && FeintChance > 0.f && CurrentComboIndex < 0 && HitWindowTimer > 0.25f
		&& FMath::FRand() < FeintChance)
	{
		bFeintArmed = true;
		FeintCancelTimer = HitWindowTimer * FMath::FRandRange(0.4f, 0.7f);
	}
	return true;
}

float UAICombatComponent::PickApproachRange(float CurrentDistance, float& OutMinRange)
{
	OutMinRange = 0.f;
	TArray<TPair<int32, float>> Candidates;
	float TotalWeight = 0.f;
	for (int32 i = 0; i < Attacks.Num(); ++i)
	{
		const FAIAttackData& A = Attacks[i];
		if (!A.AttackMontage || A.CurrentCooldown > 0.f) continue;
		if (A.AvailableInPhases.Num() > 0 && !A.AvailableInPhases.Contains(CurrentPhase)) continue;
		float W = A.SelectionWeight;
		if (i == LastSelectedAttack) W *= AttackRepeatPenalty;
		Candidates.Add(TPair<int32, float>(i, W));
		TotalWeight += W;
	}
	if (Candidates.Num() == 0)
	{
		int32 Best = INDEX_NONE;
		for (int32 i = 0; i < Attacks.Num(); ++i)
			if (Attacks[i].AttackMontage && (Best == INDEX_NONE || Attacks[i].MinRange < Attacks[Best].MinRange)) Best = i;
		if (Best != INDEX_NONE)
		{
			OutMinRange = Attacks[Best].MinRange;
			return Attacks[Best].Range;
		}
		return GetEffectiveAttackRange();
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	int32 Chosen = Candidates.Last().Key;
	for (const auto& [Idx, W] : Candidates)
	{
		Roll -= W;
		if (Roll <= 0.f) { Chosen = Idx; break; }
	}
	OutMinRange = Attacks[Chosen].MinRange;
	return Attacks[Chosen].Range;
}

float UAICombatComponent::GetSurfaceDistanceToTarget() const
{
	if (!OwnerCharacter) return TNumericLimits<float>::Max();
	const AActor* T = OwnerCharacter->GetCurrentTarget();
	if (!T) return TNumericLimits<float>::Max();

	float Reach = 0.f;
	if (const UCapsuleComponent* MyCap = OwnerCharacter->GetCapsuleComponent()) Reach += MyCap->GetScaledCapsuleRadius();
	if (const ACharacter* C = Cast<ACharacter>(T))
		if (const UCapsuleComponent* Cap = C->GetCapsuleComponent()) Reach += Cap->GetScaledCapsuleRadius();
	return FMath::Max(0.f, FVector::Dist(OwnerCharacter->GetActorLocation(), T->GetActorLocation()) - Reach);
}

void UAICombatComponent::ManualTriggerHitWindow()
{
	if (bIsAttacking && !bHitWindowFired) FireHitWindow();
}

void UAICombatComponent::FireHitWindow()
{
	if (!Attacks.IsValidIndex(CurrentAttackIndex) || !OwnerCharacter) return;

	bHitWindowFired = true;
	const FAIAttackData& Atk = Attacks[CurrentAttackIndex];

	HitThisSwing.Reset();
	bHitConnectedThisSwing = false;
	MultiTargetHitsThisSwing = 0;

	SweepHitWindow();

	OnAIAttackHitWindow.Broadcast(Atk, CurrentAttackIndex, OwnerCharacter->GetCurrentTarget());

	if (Atk.HitWindowDuration > 0.f)
	{
		bHitWindowActive = true;
		HitWindowActiveTimer = Atk.HitWindowDuration;
	}
	else
	{
		CloseHitWindow();
	}
}

void UAICombatComponent::TickHitWindow(float DeltaTime)
{
	if (!bHitWindowActive) return;
	if (!bIsAttacking || !Attacks.IsValidIndex(CurrentAttackIndex)) { bHitWindowActive = false; HitWindowActiveTimer = 0.f; return; }

	SweepHitWindow();
	HitWindowActiveTimer -= DeltaTime;
	if (HitWindowActiveTimer <= 0.f) CloseHitWindow();
}

void UAICombatComponent::SweepHitWindow()
{
	if (!Attacks.IsValidIndex(CurrentAttackIndex) || !OwnerCharacter) return;
	const FAIAttackData& Atk = Attacks[CurrentAttackIndex];
	AActor* Target = OwnerCharacter->GetCurrentTarget();

	const bool bWasConnected = bHitConnectedThisSwing;

	if (bAutoApplyHitWindowDamage && Atk.BaseDamage > 0.f)
	{
		if (Atk.bMultiTarget)
		{
			if (ApplyMultiTargetDamage(Atk)) bHitConnectedThisSwing = true;
		}
		else if (Target && !HitThisSwing.Contains(Target) && IsTargetInHitZone(Target, Atk))
		{
			HitThisSwing.Add(Target);
			if (ApplyHitDamageTo(Target, Atk)) bHitConnectedThisSwing = true;
		}
	}
	else if (Target && !HitThisSwing.Contains(Target) && IsTargetInHitZone(Target, Atk))
	{
		HitThisSwing.Add(Target);
		bHitConnectedThisSwing = true;
	}

	if (!bWasConnected && bHitConnectedThisSwing && Atk.HitStopDuration > 0.f)
		ApplyHitStop(Atk.HitStopDuration);
}

void UAICombatComponent::CloseHitWindow()
{
	bHitWindowActive = false;
	HitWindowActiveTimer = 0.f;
	if (Attacks.IsValidIndex(CurrentAttackIndex) && OwnerCharacter)
		OnAIAttackResolved.Broadcast(Attacks[CurrentAttackIndex], OwnerCharacter->GetCurrentTarget(), bHitConnectedThisSwing);
}

bool UAICombatComponent::ApplyHitDamageTo(AActor* Victim, const FAIAttackData& Atk)
{
	if (!Victim || !OwnerCharacter) return false;
	float Damage = Atk.BaseDamage * GetPhaseDamageMultiplier() * PendingChargeScale;
	const FVector HitDir = (Victim->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();

	bool bNegated = false;
	if (UCombatComponent* VictimCombat = Victim->FindComponentByClass<UCombatComponent>())
		Damage = VictimCombat->MitigateIncomingDamage(Damage, OwnerCharacter, bNegated);
	if (bNegated) return false;

	UGameplayStatics::ApplyPointDamage(Victim, Damage, HitDir, FHitResult(),
		OwnerCharacter->GetController(), OwnerCharacter, UDamageType::StaticClass());

	if (Atk.KnockbackForce > 0.f)
		if (ACharacter* HitChar = Cast<ACharacter>(Victim))
			HitChar->LaunchCharacter(HitDir * Atk.KnockbackForce + FVector(0.f, 0.f, Atk.KnockbackForce * 0.15f), false, false);
	return true;
}

bool UAICombatComponent::ApplyMultiTargetDamage(const FAIAttackData& Atk)
{
	UWorld* W = GetWorld();
	if (!W) return false;

	float GatherRadius = Atk.Range + 100.f;
	if (const UCapsuleComponent* MyCap = OwnerCharacter->GetCapsuleComponent())
		GatherRadius += MyCap->GetScaledCapsuleRadius();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);
	W->OverlapMultiByObjectType(Overlaps, OwnerCharacter->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
		FCollisionShape::MakeSphere(GatherRadius), Params);

	const int32 Cap = (Atk.MaxTargets > 0) ? Atk.MaxTargets : MAX_int32;
	bool bAnyHit = false;
	for (const FOverlapResult& O : Overlaps)
	{
		if (MultiTargetHitsThisSwing >= Cap) break;
		AActor* V = O.GetActor();

		if (!V || HitThisSwing.Contains(V) || !OwnerCharacter->IsValidTargetCandidate(V)) continue;
		if (!IsTargetInHitZone(V, Atk)) continue;
		HitThisSwing.Add(V);
		if (ApplyHitDamageTo(V, Atk)) { bAnyHit = true; ++MultiTargetHitsThisSwing; }
	}
	return bAnyHit;
}

bool UAICombatComponent::IsTargetInHitZone(const AActor* Target, const FAIAttackData& Atk) const
{
	if (!Target || !OwnerCharacter) return false;

	const FVector OwnerLoc = OwnerCharacter->GetActorLocation();
	const FVector TgtLoc = Target->GetActorLocation();

	float Reach = Atk.Range;
	float MyHalfHeight = 0.f, TgtHalfHeight = 0.f;
	if (const UCapsuleComponent* MyCap = OwnerCharacter->GetCapsuleComponent())
	{
		Reach += MyCap->GetScaledCapsuleRadius();
		MyHalfHeight = MyCap->GetScaledCapsuleHalfHeight();
	}
	if (const ACharacter* C = Cast<ACharacter>(Target))
		if (const UCapsuleComponent* Cap = C->GetCapsuleComponent())
		{
			Reach += Cap->GetScaledCapsuleRadius();
			TgtHalfHeight = Cap->GetScaledCapsuleHalfHeight();
		}

	if (FVector::DistSquared2D(OwnerLoc, TgtLoc) > Reach * Reach)
		return false;

	if (FMath::Abs(TgtLoc.Z - OwnerLoc.Z) > MyHalfHeight + TgtHalfHeight + Atk.VerticalHitSlack)
		return false;

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

	if (bRequireLineOfSightForHit)
		if (const UWorld* W = GetWorld())
		{
			FHitResult Block;
			FCollisionQueryParams Params(TEXT("AIHitWindowLoS"), false, OwnerCharacter);
			Params.AddIgnoredActor(Target);
			const FVector Start = OwnerLoc + FVector(0.f, 0.f, 50.f);
			const FVector End   = TgtLoc  + FVector(0.f, 0.f, 50.f);
			if (W->LineTraceSingleByChannel(Block, Start, End, ECollisionChannel::ECC_Visibility, Params))
				return false;
		}

	return true;
}

void UAICombatComponent::ApplyHitStop(float Duration)
{
	if (OwnerCharacter)
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		{

			if (HitStopTimer <= 0.f) SavedAnimRateBeforeHitStop = Mesh->GlobalAnimRateScale;
			Mesh->GlobalAnimRateScale = 0.01f;
		}
	HitStopTimer = Duration;
}

void UAICombatComponent::EndHitStop()
{
	HitStopTimer = 0.f;
	if (OwnerCharacter)
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
			Mesh->GlobalAnimRateScale = SavedAnimRateBeforeHitStop;
}

bool UAICombatComponent::StartChargeAttack(int32 AttackIndex)
{
	if (!CanAttack() || !Attacks.IsValidIndex(AttackIndex) || Attacks[AttackIndex].ChargeTime <= 0.f) return false;
	CurrentAttackIndex = AttackIndex;
	bIsCharging = true;
	ChargeTimer = 0.f;

	const FAIAttackData& Atk = Attacks[AttackIndex];
	if (Atk.ChargeLoopMontage && OwnerCharacter)
		if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
			AnimComp->PlayLoopingAction(Atk.ChargeLoopMontage, Atk.ChargeTime + 0.5f);

	OnAIChargeStarted.Broadcast(Atk);
	return true;
}

void UAICombatComponent::StopChargeLoopMontage()
{
	if (!OwnerCharacter || !Attacks.IsValidIndex(CurrentAttackIndex)) return;
	const FAIAttackData& Atk = Attacks[CurrentAttackIndex];
	if (!Atk.ChargeLoopMontage) return;
	if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
		if (AnimComp->GetCurrentActionMontage() == Atk.ChargeLoopMontage)
			AnimComp->StopCurrentAction();
}

void UAICombatComponent::ReleaseChargeAttack()
{
	if (!bIsCharging || !Attacks.IsValidIndex(CurrentAttackIndex)) return;
	const float Pct = GetChargePercent();
	bIsCharging = false;
	const int32 Idx = CurrentAttackIndex;
	StopChargeLoopMontage();
	OnAIChargeReleased.Broadcast(Pct, Attacks[Idx]);

	if (ExecuteAttack(Idx))
		PendingChargeScale = FMath::Lerp(1.f, Attacks[Idx].ChargeMultiplier, Pct);
}

void UAICombatComponent::CancelCharge()
{
	if (!bIsCharging) { ChargeTimer = 0.f; return; }
	bIsCharging = false;
	ChargeTimer = 0.f;
	StopChargeLoopMontage();

	if (Attacks.IsValidIndex(CurrentAttackIndex))
		OnAIChargeCancelled.Broadcast(Attacks[CurrentAttackIndex]);
	CurrentAttackIndex = -1;
}

bool UAICombatComponent::StartCombo(int32 ComboIndex)
{
	if (!CanAttack() || !Combos.IsValidIndex(ComboIndex) || Combos[ComboIndex].AttackIndices.Num() == 0) return false;

	if (!CanUseAttack(Combos[ComboIndex].AttackIndices[0], GetSurfaceDistanceToTarget())) return false;
	CurrentComboIndex = ComboIndex;
	CurrentComboStep = 0;
	ComboWindowTimer = Combos[ComboIndex].ComboWindowDuration;
	return ExecuteAttack(Combos[ComboIndex].AttackIndices[0]);
}

bool UAICombatComponent::AdvanceCombo()
{
	if (CurrentComboIndex < 0 || !Combos.IsValidIndex(CurrentComboIndex)) return false;
	const FAIComboChain& Combo = Combos[CurrentComboIndex];
	CurrentComboStep++;
	if (CurrentComboStep >= Combo.AttackIndices.Num()) { ResetCombo(); return false; }

	const int32 NextIdx = Combo.AttackIndices[CurrentComboStep];
	if (Attacks.IsValidIndex(NextIdx)
		&& GetSurfaceDistanceToTarget() > Attacks[NextIdx].Range * 1.3f)
	{
		ResetCombo();
		return false;
	}

	ComboWindowTimer = Combo.ComboWindowDuration;
	OnAIComboAdvanced.Broadcast(Combo, CurrentComboStep);
	return ExecuteAttack(NextIdx);
}

void UAICombatComponent::ResetCombo()
{
	if (CurrentComboIndex >= 0 && Combos.IsValidIndex(CurrentComboIndex))
		OnAIComboReset.Broadcast(Combos[CurrentComboIndex]);
	CurrentComboIndex = -1; CurrentComboStep = -1; ComboWindowTimer = 0.f; bComboAdvancePending = false; ComboGapTimer = 0.f;
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

void UAICombatComponent::InterruptAttack()
{
	if (!bIsAttacking) return;

	if (bHitWindowActive) CloseHitWindow();
	FAIAttackData Atk = Attacks.IsValidIndex(CurrentAttackIndex) ? Attacks[CurrentAttackIndex] : FAIAttackData();
	bIsAttacking = false;
	bHitWindowFired = false;
	HitThisSwing.Reset();
	bFeintArmed = false;
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
	EndHitStop();
	InterruptAttack();
	CancelCharge();
	ResetCombo();
	bIsStaggered = true;
	CurrentHitCount = 0;

	if (OwnerCharacter && !OwnerCharacter->IsDead())
		OwnerCharacter->SetAIState(EAIState::Staggered);

	float MontageLen = 0.f;
	if (OwnerCharacter && StaggerMontage)
		if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
			if (AnimComp->PlayActionMontage(StaggerMontage))
				MontageLen = StaggerMontage->GetPlayLength();
	StaggerTimer = FMath::Max(Duration, MontageLen);

	OnAIStaggered.Broadcast(StaggerTimer);
}

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
	ResetCombo();
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

	if (OwnerCharacter && !OwnerCharacter->IsDead())
		if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
		{

			if (BreakLoopMontage && AnimComp->GetCurrentActionMontage() == BreakLoopMontage)
				AnimComp->StopCurrentAction();
			if (BreakRecoverMontage)
				AnimComp->PlayActionMontage(BreakRecoverMontage);
		}

	OnAIBreakEnded.Broadcast();
}

void UAICombatComponent::TickBreak(float DeltaTime)
{
	if (bIsBroken)
	{
		BreakTimer -= DeltaTime;
		if (BreakTimer <= 0.f) { EndBreak(); return; }

		if (BreakLoopMontage && OwnerCharacter && !OwnerCharacter->IsDead())
			if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
				if (!AnimComp->IsPlayingAction())
					AnimComp->PlayLoopingAction(BreakLoopMontage, BreakTimer);
		return;
	}

	if (PoiseRegenPerSecond > 0.f && CurrentPoise > 0.f && BreakThreshold > 0.f)
	{
		CurrentPoise = FMath::Max(0.f, CurrentPoise - PoiseRegenPerSecond * DeltaTime);
		OnAIPoiseChanged.Broadcast(CurrentPoise, BreakThreshold);
	}
}

void UAICombatComponent::EnterCombat() { if (!bIsInCombat) { bIsInCombat = true; CombatElapsedTime = 0.f; CurrentHitCount = 0; OnAICombatEntered.Broadcast(); } }
void UAICombatComponent::ExitCombat() { if (bIsInCombat) { bIsInCombat = false; InterruptAttack(); CancelCharge(); ResetCombo(); bIsInRecovery = false; RecoveryTimer = 0.f; CurrentHitCount = 0; EndHitStop(); OnAICombatExited.Broadcast(); } }

void UAICombatComponent::SetPhase(EAICombatPhase NewPhase)
{
	if (CurrentPhase == NewPhase) return;
	const EAICombatPhase Old = CurrentPhase;
	CurrentPhase = NewPhase;

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
		OnAIRequestSummon.Broadcast(Data.SummonCount);
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

void UAICombatComponent::TickCooldowns(float DeltaTime)
{

	if (GlobalCooldownTimer > 0.f) GlobalCooldownTimer -= DeltaTime;
	for (FAIAttackData& A : Attacks)
		if (A.CurrentCooldown > 0.f) A.CurrentCooldown -= DeltaTime;
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
			StaggerImmunityTimer = StaggerImmunityDuration;

			if (OwnerCharacter && StaggerMontage)
				if (UAIAnimationComponent* AnimComp = OwnerCharacter->GetAIAnimation())
					if (AnimComp->GetCurrentActionMontage() == StaggerMontage)
						AnimComp->StopCurrentAction();
		}
		return;
	}
	if (StaggerImmunityTimer > 0.f) StaggerImmunityTimer -= DeltaTime;
}

void UAICombatComponent::TickAttack(float DeltaTime)
{
	if (!bIsAttacking) return;

	if (bFeintArmed && !bHitWindowFired)
	{
		FeintCancelTimer -= DeltaTime;
		if (FeintCancelTimer <= 0.f)
		{
			bFeintArmed = false;
			InterruptAttack();
			GlobalCooldownTimer = FMath::Min(GlobalCooldownTimer, 0.25f);
			return;
		}
	}

	if (bUseAutoHitWindow && !bHitWindowFired && Attacks.IsValidIndex(CurrentAttackIndex))
	{
		HitWindowTimer -= DeltaTime;
		if (HitWindowTimer <= 0.f)
			FireHitWindow();
	}

	AttackAnimTimer -= DeltaTime;
	if (AttackAnimTimer <= 0.f)
		FinishAttack();
}

void UAICombatComponent::FinishAttack()
{
	if (!bIsAttacking) return;
	if (!Attacks.IsValidIndex(CurrentAttackIndex)) { bIsAttacking = false; AttackAnimTimer = 0.f; return; }

	if (!bHitWindowFired)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Attack '%s' ended without a hit window — firing fallback. Check bUseAutoHitWindow / the montage AnimNotify."),
			OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("AI"),
			*Attacks[CurrentAttackIndex].AttackName.ToString());
		FireHitWindow();
	}

	if (bHitWindowActive) CloseHitWindow();

	FAIAttackData& Atk = Attacks[CurrentAttackIndex];
	bIsAttacking = false;
	bHitWindowFired = false;
	bFeintArmed = false;
	AttackAnimTimer = 0.f;

	const float CDMult = GetPhaseCooldownMultiplier();
	Atk.CurrentCooldown = Atk.Cooldown * CDMult;
	GlobalCooldownTimer = (GlobalCooldown + FMath::FRandRange(0.f, AttackDelayRandomDeviation)) * CDMult;

	float Recovery = Atk.RecoveryTime;
	if (!bHitConnectedThisSwing && WhiffRecoveryBonus > 0.f) Recovery += WhiffRecoveryBonus;
	if (Recovery > 0.f) { bIsInRecovery = true; RecoveryTimer = Recovery; }

	OnAIAttackEnded.Broadcast(Atk, false);

	const bool bInCombo = (CurrentComboIndex >= 0);
	CurrentAttackIndex = -1;
	if (bInCombo)
	{

		const float Gap = Combos.IsValidIndex(CurrentComboIndex) ? Combos[CurrentComboIndex].InterStepDelay : 0.f;
		if (bIsInRecovery) bComboAdvancePending = true;
		else if (Gap > 0.f) { bComboAdvancePending = true; ComboGapTimer = Gap; }
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
		if (bComboAdvancePending && !bIsStaggered && !bIsBroken) { bComboAdvancePending = false; AdvanceCombo(); }
	}
}

void UAICombatComponent::HandleActionMontageEnded(UAnimMontage* Montage)
{

	if (!bIsAttacking || !Attacks.IsValidIndex(CurrentAttackIndex)) return;
	if (Montage && Montage != Attacks[CurrentAttackIndex].AttackMontage) return;
	FinishAttack();
}

float UAICombatComponent::GetPhaseDamageMultiplier() const { FAICombatPhaseData D; return GetCurrentPhaseData(D) ? D.DamageMultiplier : 1.f; }
float UAICombatComponent::GetPhaseCooldownMultiplier() const { FAICombatPhaseData D; return GetCurrentPhaseData(D) ? D.CooldownMultiplier : 1.f; }
float UAICombatComponent::GetPhaseSpeedMultiplier() const { FAICombatPhaseData D; return GetCurrentPhaseData(D) ? D.SpeedMultiplier : 1.f; }
