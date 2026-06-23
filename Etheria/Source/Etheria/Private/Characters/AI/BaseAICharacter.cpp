/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "BaseAICharacter - Source"
 * Notes: Connects to HealthComponent for hit reactions AND auto-Die() on HP=0.
 *        Dormancy is timer-driven (FTimerManager) so it works even when Tick is off.
 *        Death plays montage → spawns VFX near end → dissolves smoothly via material parameter.
 */

#include "Characters/AI/BaseAICharacter.h"

#include "Characters/AI/Movements/AIMovementComponent.h"
#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Characters/AI/Combat/AICombatComponent.h"
#include "Characters/AI/Controller/BaseAIController.h"
#include "Characters/AI/AIPackRegistrySubsystem.h"
#include "Components/Characters/HealthComponent.h"
#include "Components/SplineComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Animation/AnimMontage.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

ABaseAICharacter::ABaseAICharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AIMovementComponent  = CreateDefaultSubobject<UAIMovementComponent>(TEXT("AIMovementComponent"));
	AIAnimationComponent = CreateDefaultSubobject<UAIAnimationComponent>(TEXT("AIAnimationComponent"));
	AICombatComponent    = CreateDefaultSubobject<UAICombatComponent>(TEXT("AICombatComponent"));

	PatrolSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PatrolSpline"));
	PatrolSpline->SetupAttachment(RootComponent);
	PatrolSpline->SetClosedLoop(false);
	PatrolSpline->bDrawDebug = false;

	ProximityDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("ProximityDecal"));
	ProximityDecal->SetupAttachment(RootComponent);
	ProximityDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	ProximityDecal->SetVisibility(false);

	SightDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("SightDecal"));
	SightDecal->SetupAttachment(RootComponent);
	SightDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	SightDecal->SetVisibility(false);

	AIControllerClass = ABaseAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	if (UCharacterMovementComponent* MC = GetCharacterMovement())
	{
		MC->bOrientRotationToMovement = true;
		MC->RotationRate = FRotator(0.f, 400.f, 0.f);
	}
}

void ABaseAICharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnLocation = GetActorLocation();
	SpawnRotation = GetActorRotation();
	SpawnScale    = GetActorScale3D();

	if (UCharacterMovementComponent* MC = GetCharacterMovement())
		MC->RotationRate = FRotator(0.f, AIMovementComponent->MovementRotationRate, 0.f);

	// Animation URO — cheap CPU win for crowds (animate less often when far/small on screen).
	if (bEnableAnimURO && GetMesh())
		GetMesh()->bEnableUpdateRateOptimizations = true;

	// Non-combat NPCs (passive, or neutral that won't fight back) never attack — skip the combat tick entirely.
	if (AICombatComponent && !ShouldEngageTargets())
		AICombatComponent->SetComponentTickEnabled(false);

	if (AIMovementComponent && PatrolSpline)
		AIMovementComponent->SetPatrolSpline(PatrolSpline);

	if (bShowDetectionDecal)
		SetDetectionDecalVisible(true);

	OnTakeAnyDamage.AddDynamic(this, &ABaseAICharacter::HandleTakeAnyDamage);

	/* ── Auto-trigger Die() when HP reaches 0 ──
	 *  HealthComponent only sets a state tag — the AI itself was never told to play its death sequence.
	 *  We bind here so HP=0 → Die() automatically. */
	CachedHealthComponent = FindComponentByClass<UHealthComponent>();
	if (CachedHealthComponent)
	{
		CachedHealthComponent->OnHealthChanged.AddDynamic(this, &ABaseAICharacter::HandleHealthChanged);
		CachedHealthComponent->SetInvulnerable(!bCanReceiveDamage);
	}

	/* ── Dormancy on a TIMER, not on Tick ──
	 *  The previous design checked dormancy in Tick. But SetDormant() disables Tick when
	 *  putting the AI to sleep, so the check would never run again — AIs stayed dormant
	 *  forever even when the player walked right up to them.
	 *  FTimerManager runs independently of actor Tick, so this works in both states. */
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			DormancyCheckTimerHandle, this,
			&ABaseAICharacter::UpdateDormancy,
			DormancyCheckInterval, /*bLoop=*/true,
			/*FirstDelay=*/0.5f); // small delay so the player has spawned
	}

	if (bStartDormant)
		SetDormant(true);

	// Register in the pack registry for O(1) pack lookups (instead of TActorIterator on hot paths).
	if (PackID != NAME_None)
		if (UWorld* W = GetWorld())
			if (UAIPackRegistrySubsystem* Reg = W->GetSubsystem<UAIPackRegistrySubsystem>())
				Reg->Register(this, PackID);
}

void ABaseAICharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PackID != NAME_None)
		if (UWorld* W = GetWorld())
			if (UAIPackRegistrySubsystem* Reg = W->GetSubsystem<UAIPackRegistrySubsystem>())
				Reg->Unregister(this, PackID);

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(DormancyCheckTimerHandle);
		W->GetTimerManager().ClearTimer(RespawnTimerHandle);
		W->GetTimerManager().ClearTimer(DeathVFXTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ABaseAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Death fade runs even when "dead" (the actor isn't dormant during fade).
	if (bIsDeathFading)
	{
		TickDeathFade(DeltaTime);
		return;
	}

	if (bIsDormant) return;

	TickThreatDecay(DeltaTime);

	if (PackID != NAME_None && CurrentState == EAIState::Patrolling)
	{
		if (!IsPackLeader())
			UpdatePackFollow(DeltaTime);
		else if (AIMovementComponent && !AIMovementComponent->IsPatrolling()
			&& AIMovementComponent->PatrolMode != EPatrolMode::Stationary)
			AIMovementComponent->StartPatrol(); // just promoted (old leader died) → resume own patrol
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebugDormancy) DrawDebugDormancy();
#endif
}

/* ═══════════ Damage Routing ═══════════ */

float ABaseAICharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	// Invulnerable — reject the damage entirely. Returning 0 BEFORE Super means the
	// engine's OnTakeAnyDamage broadcast never fires, so HealthComponent doesn't
	// touch HP and BaseAICharacter::HandleTakeAnyDamage doesn't fire OnReceiveDamage
	// (no hit reaction, no fight-back trigger, no stagger). One check, all paths covered.
	if (!bCanReceiveDamage)
	{
		OnAIDamageBlocked.Broadcast(DamageCauser, DamageAmount);
		return 0.f;
	}

	// Already dead — also reject. Prevents double-Die() if the AI takes another hit
	// during its death fade-out window before the actor is hidden.
	if (CurrentState == EAIState::Dead) return 0.f;

	// Capture the hit direction here — the OnTakeAnyDamage broadcast (fired by Super) carries no direction,
	// so this is the only place to read it for a directional hit reaction.
	if (DamageEvent.GetTypeID() == FPointDamageEvent::ClassID)
		LastHitDirection = static_cast<const FPointDamageEvent&>(DamageEvent).ShotDirection;
	else if (DamageCauser)
		LastHitDirection = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
	else
		LastHitDirection = FVector::ZeroVector; // no direction → directional reaction falls back to random

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ABaseAICharacter::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	OnReceiveDamage(DamageCauser, Damage);
}

void ABaseAICharacter::SetCanReceiveDamage(bool bNewCanReceiveDamage)
{
	bCanReceiveDamage = bNewCanReceiveDamage;
	if (CachedHealthComponent)
		CachedHealthComponent->SetInvulnerable(!bNewCanReceiveDamage);
}

void ABaseAICharacter::HandleHealthChanged(float NewHealth, float MaxHealth)
{
	if (NewHealth <= 0.f && CurrentState != EAIState::Dead)
	{
		Die();
		return;
	}

	LastHealthFraction = (MaxHealth > 0.f) ? (NewHealth / MaxHealth) : 1.f;

	if (AICombatComponent && MaxHealth > 0.f)
		AICombatComponent->EvaluatePhaseFromHP(NewHealth / MaxHealth);
}

/* ═══════════ State ═══════════ */

void ABaseAICharacter::SetAIState(EAIState NewState)
{
	if (CurrentState == NewState) return;

	const EAIState OldState = CurrentState;
	CurrentState = NewState;
	OnAIStateChanged.Broadcast(OldState, NewState);

	// Leaving an idle/patrol dwell for an active state: cancel any idle/activity montage so DirectPlayback
	// locomotion isn't gated (the creature would otherwise slide while frozen in a graze/idle pose).
	if ((OldState == EAIState::Idle || OldState == EAIState::Patrolling)
		&& NewState != EAIState::Idle && NewState != EAIState::Patrolling
		&& AIAnimationComponent && AIAnimationComponent->IsPlayingIdleVariation())
		AIAnimationComponent->StopCurrentAction();

	if (AICombatComponent)
	{
		auto IsCombatState = [](EAIState S)
		{ return S == EAIState::Attacking || S == EAIState::Chasing || S == EAIState::Staggered; };

		const bool bNowInCombat = IsCombatState(NewState);
		const bool bWasInCombat = IsCombatState(OldState);

		if (bNowInCombat && !AICombatComponent->IsInCombat()) AICombatComponent->EnterCombat();
		else if (bWasInCombat && !bNowInCombat) AICombatComponent->ExitCombat();
	}
}

void ABaseAICharacter::SetTarget(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
	if (NewTarget) OnTargetAcquired.Broadcast(NewTarget);
}

void ABaseAICharacter::ClearTarget()
{
	CurrentTarget = nullptr;
	OnTargetLost.Broadcast();
}

void ABaseAICharacter::SetHostilityType(EAIHostilityType NewType)
{
	HostilityType = NewType;
	// Combat tick follows whether this AI can now engage (so a runtime turn-hostile gets its combat back).
	if (AICombatComponent)
		AICombatComponent->SetComponentTickEnabled(ShouldEngageTargets());
	// Re-arm/disable senses for the new hostility and disengage if it can no longer fight.
	if (ABaseAIController* AIC = Cast<ABaseAIController>(GetController()))
		AIC->ReconcileHostility();
}

void ABaseAICharacter::SetAwarenessLevel(EAIAwarenessLevel NewLevel)
{
	if (AwarenessLevel == NewLevel) return;
	const EAIAwarenessLevel Old = AwarenessLevel;
	AwarenessLevel = NewLevel;
	OnAwarenessChanged.Broadcast(Old, NewLevel);
}

/* ═══════════ Perception / Threat Response ═══════════ */

bool ABaseAICharacter::ShouldEngageTargets() const
{
	return HostilityType == EAIHostilityType::Aggressive
		|| (HostilityType == EAIHostilityType::Neutral && bFightBackWhenAttacked);
}

bool ABaseAICharacter::CanReactToPerception() const
{
	// Aggressive engage on sight; others keep perception if they can flee OR react with a look-at (notice).
	if (HostilityType == EAIHostilityType::Aggressive) return true;
	return bCanFlee || bNoticeReactions; // set both false on a truly inert NPC to disable its senses for perf
}

bool ABaseAICharacter::MatchesFleeTag(const AActor* Other) const
{
	if (!Other || FleeFromTags.Num() == 0) return false;
	for (const FName& Tag : FleeFromTags)
		if (Other->ActorHasTag(Tag)) return true;
	return false;
}

bool ABaseAICharacter::IsValidTargetCandidate(AActor* Candidate) const
{
	if (!Candidate || Candidate == this) return false;
	if (Cast<ABaseAICharacter>(Candidate))
		return MatchesFleeTag(Candidate); // other AI allowed only if it's a tagged predator we flee
	if (bOnlyDetectPlayers)
	{
		const APawn* P = Cast<APawn>(Candidate);
		if (!P || !P->IsPlayerControlled())
			return MatchesFleeTag(Candidate); // non-players allowed only if tagged predator
	}
	return true;
}

bool ABaseAICharacter::IsTargetDeadOrInvalid(const AActor* Target) const
{
	if (!IsValid(Target)) return true;
	if (const ABaseAICharacter* AI = Cast<ABaseAICharacter>(Target)) return AI->IsDead();
	if (const UHealthComponent* HC = Target->FindComponentByClass<UHealthComponent>()) return HC->IsDead();
	return false;
}

/* ═══════════ Threat / Aggro ═══════════ */

void ABaseAICharacter::AddThreat(AActor* Source, float Amount)
{
	// Only accumulate threat for things we could actually target (player/predator), never dead/invalid actors.
	if (!bUseThreatSystem || Amount <= 0.f || !IsValidTargetCandidate(Source) || IsTargetDeadOrInvalid(Source)) return;
	ThreatTable.FindOrAdd(Source) += Amount;
}

void ABaseAICharacter::EvaluateThreatSwitch()
{
	// Only REDIRECT an existing target; initial acquisition stays with perception/ReactToThreat.
	if (!bUseThreatSystem || !CurrentTarget) return;

	const ABaseAIController* AIC = Cast<ABaseAIController>(GetController());
	const float CurThreat = ThreatTable.FindRef(CurrentTarget.Get());
	AActor* Best = CurrentTarget;
	float BestThreat = CurThreat;
	for (const TPair<TWeakObjectPtr<AActor>, float>& Pair : ThreatTable)
	{
		AActor* A = Pair.Key.Get();
		if (!A || A == CurrentTarget) continue;
		// Never switch to a dead/invalid/out-of-territory actor — that would thrash (switch then bail next tick).
		if (IsTargetDeadOrInvalid(A) || !IsValidTargetCandidate(A)) continue;
		if (AIC && !AIC->IsThreatInTerritory(A)) continue;
		if (Pair.Value > BestThreat) { BestThreat = Pair.Value; Best = A; }
	}

	if (Best && Best != CurrentTarget && BestThreat > CurThreat * ThreatSwitchMargin)
		SetTarget(Best);
}

void ABaseAICharacter::TickThreatDecay(float DeltaTime)
{
	if (!bUseThreatSystem || ThreatTable.Num() == 0) return;
	const float Decay = ThreatDecayPerSecond * DeltaTime;
	for (auto It = ThreatTable.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); continue; }
		if (ThreatDecayPerSecond > 0.f)
		{
			It.Value() -= Decay;
			if (It.Value() <= 0.f) It.RemoveCurrent();
		}
	}
}

bool ABaseAICharacter::ReactToThreat(AActor* Threat, bool bFromDamage)
{
	if (!Threat || CurrentState == EAIState::Dead || bIsDormant) return false;
	if (CurrentState == EAIState::Interacting)
	{
		if (!bFromDamage) return false; // sight won't pull an NPC out of dialogue
		EndInteraction();               // damage does: clear partner + close the UI before re-targeting
	}

	// Morale-break window: a just-routed AI keeps fleeing instead of instantly turning back to fight when hit.
	const bool bMoraleBroken = (GetWorld() && GetWorld()->GetTimeSeconds() < MoraleBreakUntil);

	const bool bAlreadyEngaging = (CurrentState == EAIState::Chasing || CurrentState == EAIState::Attacking);
	const bool bAlreadyFleeing  = (CurrentState == EAIState::Fleeing);

	const bool bWantsFight = !bMoraleBroken && (
		(HostilityType == EAIHostilityType::Aggressive) ||
		(bFromDamage && HostilityType == EAIHostilityType::Neutral && bFightBackWhenAttacked));

	bool bWantsFlee = false;
	if (!bWantsFight && bCanFlee &&
		(HostilityType == EAIHostilityType::Passive || HostilityType == EAIHostilityType::Neutral))
	{
		if (bFromDamage)
		{
			bWantsFlee = true;
		}
		else
		{
			bWantsFlee = (FleeFromTags.Num() == 0);
			for (const FName& Tag : FleeFromTags)
				if (Threat->ActorHasTag(Tag)) { bWantsFlee = true; break; }
		}
	}

	// A wolf dragged past its own tether must finish going home before it may re-acquire — otherwise the next
	// CheckLeashAndReturn (spawn-relative bSelfTooFar) flips it straight back to Returning (the stutter loop). This is
	// the one chokepoint ALL cold-acquire paths funnel through (sight/proximity/ramp/rally). Damage and an
	// already-engaged/fleeing AI are exempt; once back inside leash the return handler re-aggros cleanly.
	if (!bFromDamage && !bAlreadyEngaging && !bAlreadyFleeing)
		if (ABaseAIController* AIC = Cast<ABaseAIController>(GetController()))
			if (AIC->IsSelfOutsideLeash())
				return false;

	if (bWantsFight)
	{
		if (bAlreadyEngaging) return false;
		SetAwarenessLevel(EAIAwarenessLevel::InCombat);
		SetTarget(Threat);
		SetAIState(EAIState::Chasing);
		if (AIMovementComponent)
		{
			AIMovementComponent->StopPatrol();
			AIMovementComponent->SetDesiredSpeed(AIMovementComponent->ChaseSpeed);
			AIMovementComponent->MoveToLocation(Threat->GetActorLocation());
		}
		if (PackID != NAME_None) AlertPack(Threat);
		RallyNearbyAllies(Threat); // wolf-pack reflex: drag nearby allies into the fight (no PackID needed)
		return true;
	}

	if (bWantsFlee)
	{
		if (bAlreadyFleeing) { SetTarget(Threat); return false; }
		SetAwarenessLevel(EAIAwarenessLevel::Alert);
		SetTarget(Threat);
		SetAIState(EAIState::Fleeing);
		if (AIAnimationComponent) AIAnimationComponent->PlayStartle();
		if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->FleeFrom(Threat); }
		if (PackID != NAME_None) AlertPack(Threat);
		AlarmNearbyAllies(Threat); // scatter the herd
		return true;
	}

	// Notice: an NPC that won't fight or flee still acknowledges the threat (turn-to-look) when calm.
	if (bNoticeReactions && !bFromDamage && (CurrentState == EAIState::Idle || CurrentState == EAIState::Patrolling))
	{
		SetAwarenessLevel(EAIAwarenessLevel::Suspicious);
		if (ABaseAIController* AIC = Cast<ABaseAIController>(GetController()))
			AIC->NoticeActor(Threat);
	}

	return false;
}

void ABaseAICharacter::OnPerceiveTarget(AActor* PerceivedActor)
{
	if (!PerceivedActor || CurrentState == EAIState::Dead || bIsDormant) return;
	if (!IsValidTargetCandidate(PerceivedActor)) return; // players + tagged predators only (per config)
	ReactToThreat(PerceivedActor, false);
}

void ABaseAICharacter::OnReceiveDamage(AActor* DamageInstigator, float DamageAmount)
{
	if (CurrentState == EAIState::Dead || bIsDormant) return;

	OnAIDamaged.Broadcast(DamageInstigator);
	AddThreat(DamageInstigator, DamageAmount * ThreatPerDamage);

	bool bStaggering = false;
	if (ShouldEngageTargets() && AICombatComponent && !AICombatComponent->IsStaggerImmune())
	{
		AICombatComponent->CurrentHitCount++;
		if (AICombatComponent->CurrentHitCount >= AICombatComponent->StaggerThreshold)
			bStaggering = true;
	}

	if (bStaggering && AICombatComponent)
	{
		AICombatComponent->ApplyStagger(StaggerDuration);
	}
	else if (AIAnimationComponent)
	{
		const bool bInterrupted = AICombatComponent ? AICombatComponent->TryInterruptCurrentAttack() : false;
		const bool bUnderHyperArmor = AICombatComponent && AICombatComponent->IsAttacking() && !bInterrupted;
		if (!bUnderHyperArmor)
			AIAnimationComponent->PlayDirectionalHitReaction(LastHitDirection); // octant-based; falls back to random
	}

	if (!DamageInstigator) return;

	// Morale break: low HP + can flee → run, overriding fight-back. (Skipped while staggering — stagger-exit handles it.)
	if (!bStaggering && bCanFlee && FleeHealthThreshold > 0.f && LastHealthFraction <= FleeHealthThreshold
		&& CurrentState != EAIState::Fleeing)
	{
		SetAwarenessLevel(EAIAwarenessLevel::Alert);
		SetTarget(DamageInstigator);
		SetAIState(EAIState::Fleeing); // leaving a combat state auto-calls ExitCombat
		if (GetWorld()) MoraleBreakUntil = GetWorld()->GetTimeSeconds() + MoraleBreakCooldown; // don't re-aggro on the next hit
		if (AIAnimationComponent) AIAnimationComponent->PlayStartle();
		if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->FleeFrom(DamageInstigator); }
		return;
	}

	if (bStaggering)
		SetTarget(DamageInstigator);
	else
		ReactToThreat(DamageInstigator, true);

	EvaluateThreatSwitch(); // a higher-threat attacker can now steal aggro
}

/* ═══════════ Interaction ═══════════ */

void ABaseAICharacter::Interact_Implementation(AActor* Target)
{
	BeginInteraction(Target);
}

void ABaseAICharacter::CanReceiveTrace_Implementation()
{
	// Highlight eligibility hook — extend in BP if needed. Nothing required in C++.
}

void ABaseAICharacter::BeginInteraction(AActor* Interactor)
{
	if (!bIsInteractable || CurrentState == EAIState::Dead || bIsDormant) return;
	if (CurrentState == EAIState::Interacting) return;

	PreInteractionState = CurrentState;
	InteractionPartner = Interactor;

	if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->StopMovement(); }
	SetAIState(EAIState::Interacting);               // controller faces the partner & holds
	if (AIAnimationComponent) AIAnimationComponent->PlayInteraction();

	OnInteractionStarted.Broadcast(Interactor, NPCRole, DialogueID); // BP: open dialogue/shop/quest UI
}

void ABaseAICharacter::EndInteraction()
{
	if (CurrentState != EAIState::Interacting) return;
	InteractionPartner = nullptr;
	OnInteractionEnded.Broadcast();

	if (AIMovementComponent && AIMovementComponent->PatrolMode != EPatrolMode::Stationary)
	{
		SetAIState(EAIState::Patrolling);
		AIMovementComponent->StartPatrol();
	}
	else
	{
		SetAIState(EAIState::Idle);
	}
}

/* ═══════════ Pack ═══════════ */

void ABaseAICharacter::AlertPack(AActor* Threat)
{
	if (PackID == NAME_None || !Threat) return;
	for (ABaseAICharacter* Other : GetPackMembers())
		Other->OnPackAlert(this, Threat);

#if ENABLE_DRAW_DEBUG
	if (bShowDebugPack)
		DrawDebugSphere(GetWorld(), GetActorLocation(), PackAlertRadius, 16, FColor::Blue, false, 2.f, 0, 3.f);
#endif
}

void ABaseAICharacter::AlarmNearbyAllies(AActor* Threat)
{
	// Don't re-broadcast while we ourselves are reacting to someone else's alarm (prevents an O(N^2) cascade).
	if (!bAlarmsNearbyAllies || AlarmRadius <= 0.f || !Threat || bSuppressAlarmBroadcast) return;
	UWorld* W = GetWorld();
	if (!W) return;

	// Bounded overlap instead of a full-level TActorIterator.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	W->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
		FCollisionShape::MakeSphere(AlarmRadius), Params);

	for (const FOverlapResult& Ov : Overlaps)
	{
		ABaseAICharacter* O = Cast<ABaseAICharacter>(Ov.GetActor());
		if (!O || O == this || O->IsDead() || O->HostilityType != HostilityType) continue;
		O->bSuppressAlarmBroadcast = true;   // alerted ally flees but doesn't re-broadcast → single-hop spread
		O->OnPerceiveTarget(Threat);         // nearby same-type NPCs react (flee) too — the herd scatters
		O->bSuppressAlarmBroadcast = false;
	}
}

void ABaseAICharacter::RallyNearbyAllies(AActor* Threat)
{
	// Single-hop: an ally we rally engages but doesn't itself re-rally, so the shout doesn't chain across the map.
	if (!bCallForHelpOnEngage || CombatAlertRadius <= 0.f || !Threat || bSuppressRallyBroadcast) return;
	if (HostilityType != EAIHostilityType::Aggressive) return; // only predators call the pack in to attack
	UWorld* W = GetWorld();
	if (!W) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	W->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
		FCollisionShape::MakeSphere(CombatAlertRadius), Params);

	for (const FOverlapResult& Ov : Overlaps)
	{
		ABaseAICharacter* O = Cast<ABaseAICharacter>(Ov.GetActor());
		if (!O || O == this || O->IsDead() || O->IsDormant()) continue;
		if (O->HostilityType != EAIHostilityType::Aggressive) continue;
		if (!O->IsValidTargetCandidate(Threat)) continue; // respect each ally's own target filter (e.g. players-only)

		const EAIState S = O->GetCurrentAIState();
		if (S == EAIState::Chasing || S == EAIState::Attacking || S == EAIState::Fleeing || S == EAIState::Dead) continue;

		O->bSuppressRallyBroadcast = true;
		if (bRallyEngagesDirectly)
			O->OnPerceiveTarget(Threat); // aggressive → ReactToThreat → chase the target now
		else if (ABaseAIController* AIC = Cast<ABaseAIController>(O->GetController()))
			AIC->InvestigateThreat(Threat); // softer: converge on the spot, commit only on sight
		O->bSuppressRallyBroadcast = false;
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebugPack)
		DrawDebugSphere(W, GetActorLocation(), CombatAlertRadius, 16, FColor::Red, false, 2.f, 0, 3.f);
#endif
}

void ABaseAICharacter::OnPackAlert(ABaseAICharacter* Alerter, AActor* Threat)
{
	if (!Threat || CurrentState == EAIState::Dead || bIsDormant) return;
	if (CurrentState == EAIState::Chasing || CurrentState == EAIState::Attacking || CurrentState == EAIState::Fleeing) return;
	OnPackAlerted.Broadcast(Alerter, Threat);

	// Aggressive packmates go INVESTIGATE the threat's location instead of telepathically hard-aggroing
	// through walls — they only commit once their own perception confirms it.
	if (HostilityType == EAIHostilityType::Aggressive)
	{
		if (ABaseAIController* AIC = Cast<ABaseAIController>(GetController()))
		{
			AIC->InvestigateThreat(Threat);
			return;
		}
	}

	// Passive/neutral react per their own rules (flee if configured, respecting the player filter).
	OnPerceiveTarget(Threat);
}

TArray<ABaseAICharacter*> ABaseAICharacter::GetPackMembers() const
{
	TArray<ABaseAICharacter*> Members;
	if (PackID == NAME_None) return Members;

	UWorld* W = GetWorld();
	UAIPackRegistrySubsystem* Reg = W ? W->GetSubsystem<UAIPackRegistrySubsystem>() : nullptr;
	if (!Reg) return Members;

	TArray<ABaseAICharacter*> All;
	Reg->GetPackMembers(PackID, All); // O(pack size), not O(all actors)
	const FVector MyLoc = GetActorLocation();
	const float RadiusSq = PackAlertRadius * PackAlertRadius;
	for (ABaseAICharacter* O : All)
	{
		if (!O || O == this || O->IsDead()) continue;
		if (FVector::DistSquared(MyLoc, O->GetActorLocation()) <= RadiusSq) Members.Add(O);
	}
	return Members;
}

void ABaseAICharacter::AlertPackSearch(const FVector& Location)
{
	if (PackID == NAME_None) return;
	for (ABaseAICharacter* Member : GetPackMembers())
		if (Member && Member != this)
			if (ABaseAIController* AIC = Cast<ABaseAIController>(Member->GetController()))
				AIC->Investigate(Location); // idle members converge on the last-known spot
}

ABaseAICharacter* ABaseAICharacter::GetPackLeader() const
{
	if (PackID == NAME_None) return nullptr;

	UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	if (CachedPackLeader.IsValid() && !CachedPackLeader->IsDead()
		&& CachedPackLeader->GetPackID() == PackID
		&& (Now - PackLeaderCacheStamp) < PACK_LEADER_CACHE_TTL)
	{
		return CachedPackLeader.Get();
	}

	ABaseAICharacter* Leader = nullptr;
	if (UAIPackRegistrySubsystem* Reg = W ? W->GetSubsystem<UAIPackRegistrySubsystem>() : nullptr)
	{
		TArray<ABaseAICharacter*> All;
		Reg->GetPackMembers(PackID, All);
		for (ABaseAICharacter* O : All)
		{
			if (!O || O->IsDead()) continue;
			if (!Leader || O->GetUniqueID() < Leader->GetUniqueID()) Leader = O; // deterministic leader = lowest id
		}
	}

	CachedPackLeader = Leader;
	PackLeaderCacheStamp = Now;
	return Leader;
}

bool ABaseAICharacter::IsPackLeader() const { return GetPackLeader() == this; }

void ABaseAICharacter::UpdatePackFollow(float DeltaTime)
{
	ABaseAICharacter* Leader = GetPackLeader();
	if (!Leader || Leader == this || !AIMovementComponent) return;

	const float FollowThreshold = PackFollowDistance + PackSpreadRadius;
	const float DistSq = FVector::DistSquared(GetActorLocation(), Leader->GetActorLocation());
	if (FollowReevalTimer > 0.f) FollowReevalTimer -= DeltaTime;

	if (DistSq > FollowThreshold * FollowThreshold)
	{
		// Cache the spread offset so we don't re-roll a wandering goal every frame (which defeats the repath gate
		// and triggers a near-per-frame pathfind). Re-roll only on a timer or when freshly falling behind.
		if (!bHasFollowOffset || FollowReevalTimer <= 0.f)
		{
			const FVector2D R = FMath::RandPointInCircle(PackSpreadRadius);
			CachedFollowOffset = FVector(R.X, R.Y, 0.f);
			bHasFollowOffset = true;
			FollowReevalTimer = 1.5f;
		}
		// Clear any dwell idle/activity first so DirectPlayback locomotion isn't gated (would slide otherwise).
		if (AIAnimationComponent && AIAnimationComponent->IsPlayingIdleVariation())
			AIAnimationComponent->StopCurrentAction();

		const FVector DirToLeader = (Leader->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		const FVector Goal = Leader->GetActorLocation() - DirToLeader * PackFollowDistance + CachedFollowOffset;
		// Catch up faster the further behind we are, but only mildly (no leader-overtaking sprint). Detour handles separation.
		const float SpeedScale = FMath::GetMappedRangeValueClamped(
			FVector2D(FollowThreshold, FollowThreshold * 3.f), FVector2D(1.f, 1.4f), FMath::Sqrt(DistSq));
		AIMovementComponent->SetDesiredSpeed(AIMovementComponent->PatrolSpeed * SpeedScale);
		AIMovementComponent->MoveToLocation(Goal);
	}
	else
	{
		bHasFollowOffset = false; // back in formation → fresh offset next time we fall behind
	}
}

/* ═══════════ Leash ═══════════ */

void ABaseAICharacter::TeleportToSpawn(FVector OverrideLocation)
{
	ClearTarget();
	if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->StopMovement(); }
	if (AICombatComponent) { AICombatComponent->InterruptAttack(); AICombatComponent->ExitCombat(); }

	// Prefer the caller's reachable (nav-projected) point; fall back to the raw spawn.
	const FVector Dest = OverrideLocation.IsNearlyZero() ? SpawnLocation : OverrideLocation;
	SetActorHiddenInGame(true);
	SetActorLocation(Dest);
	SetActorRotation(SpawnRotation);
	SetActorHiddenInGame(false);

	SetAwarenessLevel(EAIAwarenessLevel::Unaware);
	SetAIState(EAIState::Idle);

	// Landed home: re-scan once right now so a player standing on the spawn re-aggros this frame, instead of being
	// ignored until the detection ramp refills (the teleport cleared our target + perception). If it bites we go
	// straight to Chasing and skip the patrol kick. Mirrors HandleReturnState's on-arrival reacquire.
	if (ABaseAIController* AIC = Cast<ABaseAIController>(GetController()))
		if (AIC->ReacquireOnReturn()) return;

	if (AIMovementComponent && AIMovementComponent->PatrolMode != EPatrolMode::Stationary)
	{
		SetAIState(EAIState::Patrolling);
		AIMovementComponent->StartPatrol();
	}
}

/* ═══════════ Death Sequence ═══════════ */

void ABaseAICharacter::Die()
{
	if (CurrentState == EAIState::Dead) return;

	// Close any open dialogue cleanly so the BP UI isn't orphaned.
	if (CurrentState == EAIState::Interacting && InteractionPartner)
	{
		InteractionPartner = nullptr;
		OnInteractionEnded.Broadcast();
	}

	const EAIState OldState = CurrentState;
	CurrentState = EAIState::Dead;
	OnAIStateChanged.Broadcast(OldState, EAIState::Dead);

	ClearTarget();
	if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->StopMovement(); }
	if (AICombatComponent) AICombatComponent->ExitCombat();

	// Disable physics and brain so the corpse doesn't keep colliding with the player
	if (GetCharacterMovement()) GetCharacterMovement()->DisableMovement();
	if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (AController* AC = GetController()) AC->SetActorTickEnabled(false);

	// Play the death montage and grab its length so we can schedule the VFX/fade
	UAnimMontage* DeathMontage = nullptr;
	if (AIAnimationComponent) DeathMontage = AIAnimationComponent->PlayDeath();

	OnAIDied.Broadcast();

	const float MontageLength = (DeathMontage ? DeathMontage->GetPlayLength() : 1.f);
	const float TriggerAt = FMath::Max(0.f, MontageLength - DeathVFXTimeBeforeEnd);

	if (TriggerAt <= KINDA_SMALL_NUMBER)
	{
		// Montage shorter than configured offset (or no montage at all) — trigger immediately
		OnDeathVFXAndFadeStart();
	}
	else if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(DeathVFXTimerHandle, this,
			&ABaseAICharacter::OnDeathVFXAndFadeStart, TriggerAt, false);
	}

	if (RespawnCondition == EAIRespawnCondition::OnTimer)
	{
		GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this,
			&ABaseAICharacter::HandleRespawnTimer, RespawnTimerDuration, false);
	}
}

void ABaseAICharacter::OnDeathVFXAndFadeStart()
{
	// ── Spawn the configurable VFX ──
	if (DeathVFX)
	{
		USkeletalMeshComponent* MMesh = GetMesh();
		if (DeathVFXSocket != NAME_None && MMesh && MMesh->DoesSocketExist(DeathVFXSocket))
		{
			SpawnedDeathVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
				DeathVFX, MMesh, DeathVFXSocket,
				FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget, /*bAutoDestroy=*/true);
		}
		else
		{
			SpawnedDeathVFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), DeathVFX, GetActorLocation(), GetActorRotation());
		}
	}

	// ── Begin the dissolve fade ──
	if (bUseDeathFade)
	{
		CacheMeshMaterials();
		DeathStartLocation = GetActorLocation();
		DeathFadeProgress = 0.f;
		bIsDeathFading = true;
		OnAIDeathFadeStarted.Broadcast();
	}
	else
	{
		FinishDeathFade();
	}
}

void ABaseAICharacter::TickDeathFade(float DeltaTime)
{
	if (DeathFadeDuration <= 0.f) { FinishDeathFade(); return; }

	DeathFadeProgress += DeltaTime / DeathFadeDuration;
	const float Alpha = FMath::Clamp(DeathFadeProgress, 0.f, 1.f);

	// 1) Drive the dissolve scalar parameter on every cached MID
	if (bDissolveParamFound && DeathDissolveParameterName != NAME_None)
	{
		for (UMaterialInstanceDynamic* MID : CachedDynamicMaterials)
			if (MID) MID->SetScalarParameterValue(DeathDissolveParameterName, Alpha);
	}
	else if (bUseScaleFallback)
	{
		// Fallback: shrink the actor smoothly. Visually less elegant but always works.
		const float Scale = FMath::Lerp(1.f, 0.01f, Alpha);
		SetActorScale3D(SpawnScale * Scale);
	}

	// 2) Optional sink into ground
	if (DeathSinkDistance > 0.f)
	{
		FVector NewLoc = DeathStartLocation;
		NewLoc.Z -= Alpha * DeathSinkDistance;
		SetActorLocation(NewLoc, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (Alpha >= 1.f) FinishDeathFade();
}

void ABaseAICharacter::FinishDeathFade()
{
	bIsDeathFading = false;
	SetActorHiddenInGame(true);

	// Let the VFX finish naturally — deactivating stops new emission but lets existing particles fade.
	if (SpawnedDeathVFX)
		SpawnedDeathVFX->Deactivate();

	OnAIDeathFadeCompleted.Broadcast();

	// Cleanup: a never-respawning corpse shouldn't linger as a hidden actor forever. Defer the Destroy to next
	// tick — this can run synchronously inside a damage broadcast, and destroying mid-broadcast is a use-after-free.
	if (bDestroyCorpseIfNoRespawn && RespawnCondition == EAIRespawnCondition::Never)
		if (UWorld* W = GetWorld())
			W->GetTimerManager().SetTimerForNextTick(this, &ABaseAICharacter::DestroyCorpse);
}

void ABaseAICharacter::DestroyCorpse()
{
	Destroy();
}

void ABaseAICharacter::CacheMeshMaterials()
{
	CachedDynamicMaterials.Reset();
	bDissolveParamFound = false;

	USkeletalMeshComponent* MMesh = GetMesh();
	if (!MMesh) return;

	const int32 NumMats = MMesh->GetNumMaterials();
	for (int32 i = 0; i < NumMats; ++i)
	{
		if (UMaterialInstanceDynamic* MID = MMesh->CreateAndSetMaterialInstanceDynamic(i))
		{
			CachedDynamicMaterials.Add(MID);

			// Verify the dissolve param exists on at least one material so we know whether to use the fallback
			if (!bDissolveParamFound && DeathDissolveParameterName != NAME_None)
			{
				float Existing = 0.f;
				if (MID->GetScalarParameterValue(DeathDissolveParameterName, Existing))
					bDissolveParamFound = true;
			}
		}
	}

	if (!bDissolveParamFound && bUseScaleFallback)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[%s] Dissolve parameter '%s' not found on materials — using scale fallback for death fade."),
			*GetName(), *DeathDissolveParameterName.ToString());
	}
}

void ABaseAICharacter::ResetDeathVisuals()
{
	bIsDeathFading = false;
	DeathFadeProgress = 0.f;

	if (UWorld* W = GetWorld())
		W->GetTimerManager().ClearTimer(DeathVFXTimerHandle);

	if (SpawnedDeathVFX)
	{
		SpawnedDeathVFX->DestroyComponent();
		SpawnedDeathVFX = nullptr;
	}

	// Reset dissolve param on all MIDs
	if (DeathDissolveParameterName != NAME_None)
	{
		for (UMaterialInstanceDynamic* MID : CachedDynamicMaterials)
			if (MID) MID->SetScalarParameterValue(DeathDissolveParameterName, 0.f);
	}

	// Reset scale fallback
	SetActorScale3D(SpawnScale);

	SetActorHiddenInGame(false);
}

/* ═══════════ Respawn ═══════════ */

void ABaseAICharacter::Respawn()
{
	if (UWorld* W = GetWorld())
		W->GetTimerManager().ClearTimer(RespawnTimerHandle);

	ResetDeathVisuals();

	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnRotation);

	if (GetCharacterMovement()) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (AController* AC = GetController()) AC->SetActorTickEnabled(true);

	// Restore HP — otherwise the AI respawns at 0 and immediately dies again
	if (CachedHealthComponent) CachedHealthComponent->ResetHealth();

	SetAwarenessLevel(EAIAwarenessLevel::Unaware);
	SetAIState(EAIState::Idle);

	if (AIMovementComponent && AIMovementComponent->PatrolMode != EPatrolMode::Stationary)
	{
		SetAIState(EAIState::Patrolling);
		AIMovementComponent->StartPatrol();
	}

	OnAIRespawned.Broadcast();
}

void ABaseAICharacter::NotifyPlayerSaved()
{
	if (CurrentState != EAIState::Dead) return;
	if (RespawnCondition == EAIRespawnCondition::OnSave || RespawnCondition == EAIRespawnCondition::OnSaveOrDay) Respawn();
}

void ABaseAICharacter::NotifyDayCycleComplete()
{
	if (CurrentState != EAIState::Dead) return;
	if (RespawnCondition == EAIRespawnCondition::OnDayCycle || RespawnCondition == EAIRespawnCondition::OnSaveOrDay) Respawn();
}

void ABaseAICharacter::HandleRespawnTimer() { if (CurrentState == EAIState::Dead) Respawn(); }

/* ═══════════ Optimization ═══════════ */

void ABaseAICharacter::SetDormant(bool bNewDormant)
{
	if (bIsDormant == bNewDormant) return;
	bIsDormant = bNewDormant;

	SetActorHiddenInGame(bNewDormant);
	SetActorTickEnabled(!bNewDormant);

	// Thaw any active hit-stop before freezing the combat tick, else GlobalAnimRateScale is stranded at 0.01x.
	if (bNewDormant && AICombatComponent) AICombatComponent->EndHitStop();

	if (AIMovementComponent)  AIMovementComponent->SetComponentTickEnabled(!bNewDormant);
	if (AIAnimationComponent) AIAnimationComponent->SetComponentTickEnabled(!bNewDormant);
	if (AICombatComponent)    AICombatComponent->SetComponentTickEnabled(!bNewDormant);
	if (GetCharacterMovement()) GetCharacterMovement()->SetComponentTickEnabled(!bNewDormant);

	// Also disable the controller — its perception updates and state machine were costing CPU even while dormant.
	if (AController* AC = GetController()) AC->SetActorTickEnabled(!bNewDormant);

	// Disable collision while dormant so the player can't bump into invisible AI capsules
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		Capsule->SetCollisionEnabled(bNewDormant ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);

	OnAIDormancyChanged.Broadcast();
}

void ABaseAICharacter::UpdateDormancy()
{
	// Dead AIs don't need dormancy management — they're either fading or already hidden.
	// Never sleep mid-conversation either (player could be standing right there).
	if (CurrentState == EAIState::Dead || CurrentState == EAIState::Interacting) return;

	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn()) return;

	const float DistSq = FVector::DistSquared(GetActorLocation(), PC->GetPawn()->GetActorLocation());

	// Hysteresis prevents rapid toggling at the boundary:
	//   sleep when further than DormantDistance,
	//   wake when closer than DormantDistance * 0.8 (so we need to come ~20% inside).
	const float SleepSq = DormantDistance * DormantDistance;
	const float WakeSq  = (DormantDistance * 0.8f) * (DormantDistance * 0.8f);
	if (!bIsDormant && DistSq > SleepSq)
		SetDormant(true);
	else if (bIsDormant && DistSq < WakeSq)
		SetDormant(false);
}

/* ═══════════ Detection Decal ═══════════ */

void ABaseAICharacter::SetDetectionDecalVisible(bool bVisible)
{
	if (ProximityDecal && ProximityDecalMaterial) { ProximityDecal->SetDecalMaterial(ProximityDecalMaterial); ProximityDecal->SetVisibility(bVisible); }
	if (SightDecal && SightDecalMaterial) { SightDecal->SetDecalMaterial(SightDecalMaterial); SightDecal->SetVisibility(bVisible); }
}

/* ═══════════ Debug ═══════════ */

#if ENABLE_DRAW_DEBUG
void ABaseAICharacter::DrawDebugDormancy() const
{
	const UWorld* W = GetWorld();
	if (!W) return;
	const FColor Col = bIsDormant ? FColor::Red : FColor::Green;
	DrawDebugSphere(W, GetActorLocation(), DormantDistance, 24, Col, false, -1.f, 0, 2.f);
	DrawDebugSphere(W, GetActorLocation(), DormantDistance * 0.8f, 24, FColor(180, 180, 180), false, -1.f, 0, 1.f);
}
#endif