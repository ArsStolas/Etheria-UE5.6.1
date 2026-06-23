/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "BaseAIController - Source"
 */

#include "Characters/AI/Controller/BaseAIController.h"

#include "Characters/AI/BaseAICharacter.h"
#include "Characters/AI/Movements/AIMovementComponent.h"
#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Characters/AI/Combat/AICombatComponent.h"
#include "Characters/AI/Combat/AICombatDirectorSubsystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

ABaseAIController::ABaseAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
	SetupPerception();
}

void ABaseAIController::SetupPerception()
{
	UAIPerceptionComponent* PC = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SetPerceptionComponent(*PC);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = SightFOVDegrees;
	SightConfig->SetMaxAge(5.f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	PC->ConfigureSense(*SightConfig);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(3.f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	PC->ConfigureSense(*HearingConfig);

	PC->SetDominantSense(UAISense_Sight::StaticClass());
	PC->OnTargetPerceptionUpdated.AddDynamic(this, &ABaseAIController::OnPerceptionUpdated);
}

void ABaseAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AICharacter = Cast<ABaseAICharacter>(InPawn);
	if (!AICharacter) return;

	SpawnOrigin = AICharacter->GetActorLocation();
	// Snap the return goal onto the navmesh so "go home" is always reachable (spawn may be slightly off-nav).
	if (const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Proj;
		if (Nav->ProjectPointToNavigation(SpawnOrigin, Proj, FVector(200.f, 200.f, 300.f)))
			SpawnOrigin = Proj.Location;
	}

	AICharacter->OnAIStateChanged.AddDynamic(this, &ABaseAIController::HandleAIStateChanged);
	AICharacter->OnAIDormancyChanged.AddDynamic(this, &ABaseAIController::HandleDormancyChanged);
	ConfigureCrowdAvoidance();

	if (UAIPerceptionComponent* PerComp = GetPerceptionComponent())
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = SightFOVDegrees;
		HearingConfig->HearingRange = HearingRange;
		PerComp->RequestStimuliListenerUpdate();
	}

	// Inert NPCs (can't react to perception) don't need sight/hearing at all — disable the senses to save CPU.
	if (!AICharacter->CanReactToPerception())
		if (UAIPerceptionComponent* PerComp = GetPerceptionComponent())
		{
			PerComp->SetSenseEnabled(UAISense_Sight::StaticClass(), false);
			PerComp->SetSenseEnabled(UAISense_Hearing::StaticClass(), false);
		}

	NextIdleAnimTime = IdleAnimInterval + FMath::FRandRange(0.f, IdleAnimRandomDeviation);

	/* ── Defer initial patrol kickoff ─────────────────────────────────────
	 * For placed pawns, OnPossess runs BEFORE the pawn's BeginPlay, which is
	 * where the patrol spline gets wired up. Calling StartPatrol() directly
	 * here would silently fail in Path mode (no spline → AI stuck on its own
	 * spot). A small delay guarantees:
	 *   - Character::BeginPlay has run (PatrolSpline pointer set).
	 *   - The navmesh is fully built.
	 *   - The movement component is ready to issue MoveTo requests.
	 * ──────────────────────────────────────────────────────────────────── */
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			InitialPatrolTimerHandle,
			this,
			&ABaseAIController::TryStartInitialPatrol,
			FMath::Max(InitialPatrolDelay, 0.01f),
			false);
	}
}

void ABaseAIController::OnUnPossess()
{
	ReleaseAttackTokenHeld();
	if (AICharacter)
	{
		AICharacter->OnAIStateChanged.RemoveDynamic(this, &ABaseAIController::HandleAIStateChanged);
		AICharacter->OnAIDormancyChanged.RemoveDynamic(this, &ABaseAIController::HandleDormancyChanged);
	}
	if (UWorld* W = GetWorld())
		W->GetTimerManager().ClearTimer(InitialPatrolTimerHandle);
	Super::OnUnPossess();
}

void ABaseAIController::ConfigureCrowdAvoidance()
{
	if (!bUseCrowdAvoidance) return;
	// Skip Detour for solo non-aggressive NPCs (a lone cow/villager) — it costs per-agent in the crowd manager.
	if (AICharacter && AICharacter->GetHostilityType() != EAIHostilityType::Aggressive && AICharacter->GetPackID() == NAME_None)
		return;
	if (UCrowdFollowingComponent* Crowd = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		Crowd->SetCrowdSimulationState(ECrowdSimulationState::Enabled);
		Crowd->SetCrowdSeparation(true);
		Crowd->SetCrowdSeparationWeight(CrowdSeparationWeight);
		Crowd->SetCrowdAvoidanceRangeMultiplier(CrowdAvoidanceRangeMultiplier);
		Crowd->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Medium);
	}
}

void ABaseAIController::HandleAIStateChanged(EAIState OldState, EAIState NewState)
{
	if ((OldState == EAIState::Attacking && NewState != EAIState::Attacking) || NewState == EAIState::Dead)
		ReleaseAttackTokenHeld();

	if (NewState == EAIState::Returning) ReturnTimer = 0.f; // (re)start the anti-soft-lock watchdog
	if (NewState == EAIState::Fleeing) { bFleeCornered = false; FleeReevalTimer = 0.f; } // fresh flee → recompute now, don't carry a stale corner-freeze

	// Leaving the active swing: drop any half-committed dodge so it can't replay stale on re-entry.
	if (OldState == EAIState::Attacking && NewState != EAIState::Attacking) { bEvading = false; EvadeTimer = 0.f; }

	// Disengaging entirely: forget the size-up so re-engaging the same target gets a fresh first-contact beat.
	if (NewState == EAIState::Returning || NewState == EAIState::Idle || NewState == EAIState::Patrolling
		|| NewState == EAIState::Investigating || NewState == EAIState::Dead)
	{ EngagedTarget = nullptr; EngageReactionTimer = 0.f; }

	// Going home / idle: clear the target-bound last-known memory (a fresh chase re-captures it).
	if (NewState == EAIState::Returning || NewState == EAIState::Idle || NewState == EAIState::Patrolling)
	{ LastKnownActor = nullptr; LastKnownLocation = FVector::ZeroVector; }

	if (AICharacter)
		if (UCharacterMovementComponent* MC = AICharacter->GetCharacterMovement())
			MC->bOrientRotationToMovement = (NewState != EAIState::Attacking);
}

void ABaseAIController::HandleDormancyChanged()
{
	if (!AICharacter) return;
	if (AICharacter->IsDormant()) { ReleaseAttackTokenHeld(); return; }

	// Waking up: recover from transient states that may have frozen mid-transition while dormant.
	const EAIState S = AICharacter->GetCurrentAIState();
	if (S == EAIState::Interacting) { AICharacter->EndInteraction(); return; } // close stale dialogue cleanly
	if (S == EAIState::Investigating || S == EAIState::Staggered)
	{
		UAICombatComponent* C = AICharacter->GetAICombat();
		if (S == EAIState::Staggered && C && C->IsStaggered()) return; // genuinely still staggered — let it drain
		AICharacter->SetAIState((AICharacter->GetCurrentTarget() && AICharacter->ShouldEngageTargets())
			? EAIState::Chasing : EAIState::Returning);
	}
}

UAICombatDirectorSubsystem* ABaseAIController::GetCombatDirector() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetSubsystem<UAICombatDirectorSubsystem>() : nullptr;
}

bool ABaseAIController::TryTakeAttackTurn(AActor* Target)
{
	if (!bUseAttackTokens) return true;

	UAICombatDirectorSubsystem* Dir = GetCombatDirector();
	if (!Dir || !AICharacter || !Target) return true;

	const bool bGranted = Dir->RequestAttackToken(Target, AICharacter, MaxSimultaneousAttackers, AttackTokenLeaseDuration);
	bHoldingAttackToken = bGranted;
	TokenTarget = bGranted ? Target : nullptr;
	return bGranted;
}

void ABaseAIController::ReleaseAttackTokenHeld()
{
	if (!bHoldingAttackToken) return;
	if (UAICombatDirectorSubsystem* Dir = GetCombatDirector())
		Dir->ReleaseAttackToken(TokenTarget.Get(), AICharacter);
	bHoldingAttackToken = false;
	TokenTarget = nullptr;
}

bool ABaseAIController::IsAttackWindowOpen(AActor* Target) const
{
	if (!bUseAttackTokens || AttackInterval <= 0.f) return true;
	if (UAICombatDirectorSubsystem* Dir = GetCombatDirector())
		return Dir->IsAttackWindowOpen(Target, AttackInterval);
	return true;
}

void ABaseAIController::NotifyAttackStarted(AActor* Target)
{
	if (!bUseAttackTokens || AttackInterval <= 0.f) return;
	if (UAICombatDirectorSubsystem* Dir = GetCombatDirector())
		Dir->NotifyAttackStarted(Target);
}

bool ABaseAIController::HasUsableAttack(const UAICombatComponent* Combat, float Distance) const
{
	if (!Combat) return false;
	const int32 Num = Combat->GetAttacks().Num();
	for (int32 i = 0; i < Num; ++i)
		if (Combat->CanUseAttack(i, Distance)) return true;
	return false;
}

void ABaseAIController::TryStartInitialPatrol()
{
	if (!AICharacter || AICharacter->IsDead() || AICharacter->IsDormant()) return;

	UAIMovementComponent* MC = AICharacter->GetAIMovement();
	if (!MC) return;

	// Don't override an active state (e.g. AI was already alerted during the delay window).
	const EAIState State = AICharacter->GetCurrentAIState();
	if (State != EAIState::Idle && State != EAIState::Patrolling) return;

	if (MC->PatrolMode == EPatrolMode::Stationary) return;

	AICharacter->SetAIState(EAIState::Patrolling);
	MC->StartPatrol();
}

void ABaseAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!AICharacter || AICharacter->IsDormant()) return;

	if (EvadeCooldownTimer > 0.f) EvadeCooldownTimer -= DeltaTime; // drains even while chasing, not just in the attack state

	// Keep the last-known location fresh while we can actually sense the target (bound to that exact target).
	if (AActor* T = AICharacter->GetCurrentTarget())
		if (IsTargetCurrentlySeen(T))
		{
			LastKnownLocation = T->GetActorLocation();
			LastKnownActor = T;
		}

	if (LostSightTimer >= 0.f)
	{
		LostSightTimer -= DeltaTime;
		if (LostSightTimer <= 0.f)
		{
			LostSightTimer = -1.f;
			const EAIState Cur = AICharacter->GetCurrentAIState();
			AICharacter->ClearTarget();
			if (Cur == EAIState::Chasing || Cur == EAIState::Attacking)
			{
				AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Alert);
				// Don't give up cold — go SEARCH where we last saw them (and rally idle packmates there), then return if nothing.
				if (!LastKnownLocation.IsZero())
				{
					AICharacter->AlertPackSearch(LastKnownLocation);
					BeginInvestigate(LastKnownLocation);
				}
				else AICharacter->SetAIState(EAIState::Returning);
			}
		}
	}

	// Proximity overlap only for AI that actually pursue or flee — notice-only NPCs rely on cheap sight perception.
	if (bUseProximityDetection
		&& (AICharacter->GetHostilityType() == EAIHostilityType::Aggressive || AICharacter->CanFlee()))
	{
		ProximityTimer -= DeltaTime;
		if (ProximityTimer <= 0.f) { ProximityTimer = ProximityCheckInterval; CheckProximityDetection(); }
	}

	TickDetection(DeltaTime);

	// Ambient look-at: briefly face a noticed actor while idle/patrolling (curiosity).
	if (NoticedActor.IsValid() && NoticeTimer > 0.f)
	{
		const EAIState St = AICharacter->GetCurrentAIState();
		if (St == EAIState::Idle || St == EAIState::Patrolling)
		{
			// Only turn to look when basically stationary, so a patrolling NPC keeps its heading (no rotation fight).
			if (AICharacter->GetVelocity().SizeSquared2D() < 10000.f) // < ~100 cm/s
				FaceTargetYawOnly(NoticedActor.Get(), DeltaTime);
			NoticeTimer -= DeltaTime;
			if (NoticeTimer <= 0.f)
			{
				NoticedActor = nullptr;
				if (AICharacter->GetAwarenessLevel() == EAIAwarenessLevel::Suspicious)
					AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
			}
		}
		else { NoticedActor = nullptr; NoticeTimer = 0.f; }
	}

	// Heard-noise suspicion fades on its own lifetime (identical in Idle and Patrol).
	if (SuspicionTimer > 0.f)
	{
		SuspicionTimer -= DeltaTime;
		// Don't force Unaware if a detection ramp is building — it owns the Suspicious state then (no on/off flicker).
		if (SuspicionTimer <= 0.f && !PendingDetectTarget.IsValid()
			&& AICharacter->GetAwarenessLevel() == EAIAwarenessLevel::Suspicious)
			AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
	}

	switch (AICharacter->GetCurrentAIState())
	{
	case EAIState::Idle:          HandleIdleState(DeltaTime);        break;
	case EAIState::Patrolling:    HandlePatrolState(DeltaTime);      break;
	case EAIState::Chasing:       HandleChaseState(DeltaTime);       break;
	case EAIState::Attacking:     HandleAttackState(DeltaTime);      break;
	case EAIState::Returning:     HandleReturnState(DeltaTime);      break;
	case EAIState::Fleeing:       HandleFleeState(DeltaTime);        break;
	case EAIState::Staggered:     HandleStaggerState(DeltaTime);     break;
	case EAIState::Investigating: HandleInvestigateState(DeltaTime); break;
	case EAIState::Interacting:   HandleInteractState(DeltaTime);    break;
	default: break;
	}

#if ENABLE_DRAW_DEBUG
	if (AICharacter->ShouldShowDebugPerception()) DrawDebugPerception();
#endif
}

void ABaseAIController::FaceTargetYawOnly(AActor* Target, float DeltaTime)
{
	if (!Target || !AICharacter) return;
	const FVector Dir = (Target->GetActorLocation() - AICharacter->GetActorLocation()).GetSafeNormal2D();
	if (Dir.IsNearlyZero()) return;
	const FRotator Cur = AICharacter->GetActorRotation();
	const FRotator Goal = FRotator(Cur.Pitch, Dir.Rotation().Yaw, Cur.Roll); // yaw only — preserve pitch/roll
	AICharacter->SetActorRotation(FMath::RInterpTo(Cur, Goal, DeltaTime, FaceTargetRotationSpeed));
}

/* ═══════════ Perception ═══════════ */

void ABaseAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!AICharacter || !Actor || AICharacter->IsDormant()) return;

	const bool bIsHearing = (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>());

	if (Stimulus.WasSuccessfullySensed())
	{
		if (!IsThreatInTerritory(Actor)) return;

		// Heard a noise: investigate the SOURCE location instead of locking onto the actor's live position
		// through walls. Already-engaged AI just refreshes its memory.
		if (bIsHearing)
		{
			if (AICharacter->GetCurrentTarget() == Actor) { LostSightTimer = -1.f; return; }

			const EAIState Cur = AICharacter->GetCurrentAIState();
			const bool bEngaged = (Cur == EAIState::Chasing || Cur == EAIState::Attacking);
			if (!bEngaged && AICharacter->CanReactToPerception())
			{
				if (bInvestigateNoises && AICharacter->GetHostilityType() == EAIHostilityType::Aggressive)
					BeginInvestigate(Stimulus.StimulusLocation); // hunters go look
				else if (AICharacter->GetAwarenessLevel() < EAIAwarenessLevel::Alert)
				{
					AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Suspicious); // prey gets edgy at a noise
					SuspicionTimer = SuspicionDuration; // give it a real lifetime so a bark/posture can play
				}
			}
			return;
		}

		// Sight (hard sense): refresh memory if it's our target, else feed the detection ramp.
		if (AICharacter->GetCurrentTarget() == Actor) { LostSightTimer = -1.f; return; }

		if (bUseDetectionRamp && AICharacter->CanReactToPerception() && !AICharacter->GetCurrentTarget())
		{
			PendingDetectTarget = Actor; // TickDetection polls whether it's still sensed and fills the meter
			return;
		}

		AICharacter->OnPerceiveTarget(Actor);
	}
	else
	{
		// Lost a sense on this actor. (The detection meter drains on its own — TickDetection polls HasActiveStimulus.)
		if (AICharacter->GetCurrentTarget() == Actor)
		{
			const EAIState Cur = AICharacter->GetCurrentAIState();
			if ((Cur == EAIState::Chasing || Cur == EAIState::Attacking) && AICharacter->ShouldEngageTargets())
			{
				if (TargetMemoryDuration > 0.f && LostSightTimer < 0.f) LostSightTimer = TargetMemoryDuration;
			}
			else
			{
				AICharacter->ClearTarget();
				if (Cur == EAIState::Fleeing) AICharacter->SetAIState(EAIState::Returning);
			}
		}
	}
}

bool ABaseAIController::ReacquireOnReturn()
{
	UWorld* W = GetWorld();
	if (!AICharacter || !W) return false;
	if (!AICharacter->CanReactToPerception()) return false; // inert NPCs never re-acquire

	const FVector MyLoc = AICharacter->GetActorLocation();
	const FVector Eye = MyLoc + FVector(0.f, 0.f, 60.f);

	// Fast path: player-only AI just checks the player pawn directly — no SightRadius overlap, no per-candidate loop.
	if (AICharacter->OnlyDetectsPlayers())
	{
		APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
		if (!Player || !AICharacter->IsValidTargetCandidate(Player) || !IsThreatInTerritory(Player)) return false;
		if (FVector::DistSquared(MyLoc, Player->GetActorLocation()) > FMath::Square(SightRadius)) return false;

		FCollisionQueryParams P;
		P.AddIgnoredActor(AICharacter);
		P.AddIgnoredActor(Player);
		FHitResult Block;
		if (W->LineTraceSingleByChannel(Block, Eye, Player->GetActorLocation() + FVector(0.f, 0.f, 60.f), ECollisionChannel::ECC_Visibility, P))
			return false; // obstructed

		AICharacter->OnPerceiveTarget(Player);
		return AICharacter->GetCurrentTarget() != nullptr;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(AICharacter);
	W->OverlapMultiByObjectType(Overlaps, MyLoc, FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
		FCollisionShape::MakeSphere(SightRadius), Params);

	AActor* Best = nullptr;
	float BestSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* Hit = O.GetActor();
		if (!AICharacter->IsValidTargetCandidate(Hit) || !IsThreatInTerritory(Hit)) continue;

		// Ignore the candidate itself so only WORLD geometry can block — character capsules often don't
		// block Visibility, which would otherwise make the trace sail past the target and reject it.
		FCollisionQueryParams LoSParams = Params;
		LoSParams.AddIgnoredActor(Hit);
		FHitResult Block;
		const bool bObstructed = W->LineTraceSingleByChannel(Block, Eye,
			Hit->GetActorLocation() + FVector(0.f, 0.f, 60.f), ECollisionChannel::ECC_Visibility, LoSParams);
		if (bObstructed) continue; // something solid sits between us and the candidate

		const float DSq = FVector::DistSquared(MyLoc, Hit->GetActorLocation());
		if (DSq < BestSq) { BestSq = DSq; Best = Hit; }
	}

	if (Best) { AICharacter->OnPerceiveTarget(Best); return AICharacter->GetCurrentTarget() != nullptr; }
	return false;
}

void ABaseAIController::CheckProximityDetection()
{
	if (!AICharacter || AICharacter->GetCurrentAIState() == EAIState::Dead || AICharacter->IsDormant()) return;

	// Already engaged (has a target) — no need to scan. Fleeing-without-target still falls through to acquire one.
	if (AICharacter->GetCurrentTarget()) return;

	const FVector MyLoc = AICharacter->GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(ProximityRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(AICharacter);

	GetWorld()->OverlapMultiByObjectType(Overlaps, MyLoc, FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), Sphere, Params);

	AActor* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* Hit = O.GetActor();
		if (!AICharacter->IsValidTargetCandidate(Hit)) continue;

		const float DSq = FVector::DistSquared(MyLoc, Hit->GetActorLocation());
		if (DSq < NearestDistSq) { NearestDistSq = DSq; Nearest = Hit; }
	}

	if (!Nearest) return;

	if (AICharacter->GetCurrentAIState() == EAIState::Fleeing)
	{
		if (!AICharacter->GetCurrentTarget())
			AICharacter->SetTarget(Nearest);
	}
	else
	{
		if (!IsThreatInTerritory(Nearest)) return;
		LostSightTimer = -1.f;
		if (bUseDetectionRamp && AICharacter->CanReactToPerception())
		{
			// Feed the detection ramp (so a stealth approach has a window) instead of hard-acquiring instantly.
			if (!PendingDetectTarget.IsValid()) PendingDetectTarget = Nearest;
			ProximitySeen = Nearest;
			ProximitySeenTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		}
		else
		{
			AICharacter->OnPerceiveTarget(Nearest);
		}
	}
}

bool ABaseAIController::IsTargetCurrentlySeen(AActor* Target)
{
	if (!Target || !AICharacter) return false;
	// Point-blank counts as sensed regardless of FOV (you can't hide at arm's length).
	if (FVector::DistSquared(AICharacter->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(ProximityRadius * 1.2f))
		return true;
	if (const UAIPerceptionComponent* PC = GetPerceptionComponent())
		return PC->HasActiveStimulus(*Target, UAISense::GetSenseID<UAISense_Sight>());
	return false;
}

void ABaseAIController::TickDetection(float DeltaTime)
{
	if (!bUseDetectionRamp) return;

	AActor* P = PendingDetectTarget.Get();
	if (!P || AICharacter->GetCurrentTarget())
	{
		PendingDetectTarget = nullptr; // already engaged or candidate gone → drop the pending meter
		DetectionProgress = 0.f;
		return;
	}

	// Is the candidate sensed THIS frame? Sight is polled from the perception component (continuous, so it
	// reflects losing line of sight), proximity from the last 360° poll record — unifying both sources in one meter.
	bool bFromSight = false;
	if (UAIPerceptionComponent* PC = GetPerceptionComponent())
		bFromSight = PC->HasActiveStimulus(*P, UAISense::GetSenseID<UAISense_Sight>());
	bool bSensed = bFromSight;
	if (!bSensed && ProximitySeen.Get() == P && GetWorld()
		&& (GetWorld()->GetTimeSeconds() - ProximitySeenTime) < (ProximityCheckInterval * 1.5f))
		bSensed = true;

	const float Rate = 1.f / FMath::Max(SightDetectionTime, 0.05f);

	if (bSensed)
	{
		// Fresh attempt → roll a small reaction-time variance so two identical guards don't detect in lockstep.
		if (DetectionProgress <= 0.f) DetectionReactionScale = FMath::FRandRange(0.8f, 1.25f);

		// Closer targets fill faster (point-blank ≈ instant); far targets take the full SightDetectionTime.
		const float CloseScale = FMath::GetMappedRangeValueClamped(
			FVector2D(0.f, SightRadius), FVector2D(3.f, 1.f),
			FVector::Dist(AICharacter->GetActorLocation(), P->GetActorLocation()));

		// Foveal vs peripheral: a target dead-centre fills faster than one at the FOV edge (sight only; 360° proximity is full-rate).
		float AngScale = 1.f;
		if (bFromSight)
		{
			const FVector ToT = (P->GetActorLocation() - AICharacter->GetActorLocation()).GetSafeNormal2D();
			if (!ToT.IsNearlyZero())
			{
				const float Ang = FMath::RadiansToDegrees(FMath::Acos(
					FMath::Clamp(FVector::DotProduct(AICharacter->GetActorForwardVector().GetSafeNormal2D(), ToT), -1.f, 1.f)));
				AngScale = FMath::GetMappedRangeValueClamped(FVector2D(0.f, SightFOVDegrees), FVector2D(1.f, PeripheralDetectionScale), Ang);
			}
		}

		DetectionProgress += DeltaTime * Rate * CloseScale * AngScale / FMath::Max(DetectionReactionScale, 0.1f);

		if (DetectionProgress >= SuspiciousThreshold && DetectionProgress < 1.f
			&& AICharacter->GetAwarenessLevel() < EAIAwarenessLevel::Alert)
		{
			AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Suspicious);
			if (NoticedActor.Get() != P) NoticeActor(P); // turn to look at the suspect — the "huh?" head-turn beat
		}

		if (DetectionProgress >= 1.f)
		{
			AActor* Acquired = P;
			PendingDetectTarget = nullptr;
			DetectionProgress = 0.f;
			AICharacter->OnPerceiveTarget(Acquired); // full detection → hard-acquire
		}
	}
	else
	{
		DetectionProgress -= DeltaTime * Rate * DetectionDecayRate;
		if (DetectionProgress <= 0.f)
		{
			DetectionProgress = 0.f;
			PendingDetectTarget = nullptr;
			if (AICharacter->GetAwarenessLevel() == EAIAwarenessLevel::Suspicious)
				AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
		}
	}
}

void ABaseAIController::InvestigateThreat(AActor* Threat)
{
	if (!AICharacter || !Threat) return;
	if (AICharacter->GetCurrentAIState() == EAIState::Dead || AICharacter->IsDormant()) return;
	BeginInvestigate(Threat->GetActorLocation());
}

void ABaseAIController::NoticeActor(AActor* Actor)
{
	if (!Actor) return;
	NoticedActor = Actor;
	NoticeTimer = NoticeDuration;
}

void ABaseAIController::Investigate(const FVector& Location)
{
	if (!AICharacter || AICharacter->GetCurrentAIState() == EAIState::Dead || AICharacter->IsDormant()) return;
	// Don't pull an already-engaged member off its own fight to go search.
	const EAIState S = AICharacter->GetCurrentAIState();
	if (S == EAIState::Chasing || S == EAIState::Attacking || S == EAIState::Fleeing) return;
	BeginInvestigate(Location);
}

void ABaseAIController::ReconcileHostility()
{
	if (!AICharacter) return;

	// Re-arm or disable the senses to match the new hostility/perception ability (they may have been off at spawn).
	if (UAIPerceptionComponent* PC = GetPerceptionComponent())
	{
		const bool bEnable = AICharacter->CanReactToPerception();
		PC->SetSenseEnabled(UAISense_Sight::StaticClass(), bEnable);
		PC->SetSenseEnabled(UAISense_Hearing::StaticClass(), bEnable);
		PC->RequestStimuliListenerUpdate();
	}

	// If it can no longer engage but was mid-fight, disengage cleanly instead of chasing forever.
	const EAIState S = AICharacter->GetCurrentAIState();
	if (!AICharacter->ShouldEngageTargets() && (S == EAIState::Chasing || S == EAIState::Attacking))
	{
		ReleaseAttackTokenHeld();
		AICharacter->ClearTarget();
		AICharacter->SetAIState(EAIState::Returning);
	}
}

void ABaseAIController::BeginInvestigate(const FVector& Location)
{
	InvestigateLocation = Location;
	InvestigateTimer = InvestigateDuration;
	AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Suspicious);
	if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
	{
		MC->StopPatrol();
		MC->SetDesiredSpeed(MC->PatrolSpeed);
		MC->MoveToLocation(Location);
	}
	ReacquireCooldown = 0.f; // allow an immediate look-around check on arrival
	AICharacter->SetAIState(EAIState::Investigating);
}

void ABaseAIController::HandleInvestigateState(float DeltaTime)
{
	UAIMovementComponent* MC = AICharacter->GetAIMovement();
	if (!MC) { AICharacter->SetAIState(EAIState::Returning); return; }

	// Self-leash only (no target during investigate, so CheckLeashAndReturn's target logic doesn't apply).
	if (FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin) > FMath::Square(AICharacter->GetLeashRange() * 1.15f))
	{
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

	InvestigateTimer -= DeltaTime;

	// Look for the target periodically while searching (throttled overlap+traces).
	ReacquireCooldown -= DeltaTime;
	if (ReacquireCooldown <= 0.f)
	{
		ReacquireCooldown = 0.4f;
		if (ReacquireOnReturn()) return; // spotted it → engage
	}

	if (InvestigateTimer <= 0.f)
	{
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

	// Reached the current spot but time remains → sweep to a fresh nav point around the noise (search the area,
	// don't just stand at one coordinate).
	if (MC->HasReachedDestination())
	{
		if (const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation NextPt;
			if (Nav->GetRandomReachablePointInRadius(InvestigateLocation, InvestigateSearchRadius, NextPt))
			{ MC->SetDesiredSpeed(MC->PatrolSpeed); MC->MoveToLocation(NextPt.Location); }
		}
	}
}

/* ═══════════ Helpers ═══════════ */

float ABaseAIController::GetEffectiveAttackRange() const
{
	if (UAICombatComponent* C = AICharacter->GetAICombat())
		if (C->GetAttacks().Num() > 0) return C->GetEffectiveAttackRange();
	return AttackRange;
}

float ABaseAIController::GetChaseSpeed() const
{
	float Speed = AICharacter->GetAIMovement() ? AICharacter->GetAIMovement()->ChaseSpeed : 500.f;
	if (const UAICombatComponent* C = AICharacter->GetAICombat())
		Speed *= C->GetPhaseSpeedMultiplier();
	return Speed;
}

void ABaseAIController::ApproachTarget(AActor* Target, float DesiredDistance)
{
	UAIMovementComponent* MC = AICharacter->GetAIMovement();
	if (!MC || !Target) return;

	const FVector MyLoc = AICharacter->GetActorLocation();
	const FVector TgtLoc = Target->GetActorLocation();
	FVector Away = (MyLoc - TgtLoc).GetSafeNormal2D();
	if (Away.IsNearlyZero()) Away = AICharacter->GetActorForwardVector();

	const FVector Standoff = TgtLoc + Away * FMath::Max(DesiredDistance, 0.f);
	MC->SetDesiredSpeed(GetChaseSpeed());
	MC->MoveToLocation(Standoff);
}

float ABaseAIController::GetCombatApproachDistance(float Range) const
{
	float Dist = Range * CombatEngageRangeRatio;
	Dist = FMath::Min(Dist, Range - CombatPositionAcceptance - 15.f);
	return FMath::Max(Dist, 1.f);
}

float ABaseAIController::GetCombatReach(const AActor* Target) const
{
	float Reach = 0.f;
	if (AICharacter)
		if (const UCapsuleComponent* MyCap = AICharacter->GetCapsuleComponent())
			Reach += MyCap->GetScaledCapsuleRadius();
	if (const ACharacter* C = Cast<ACharacter>(Target))
		if (const UCapsuleComponent* TCap = C->GetCapsuleComponent())
			Reach += TCap->GetScaledCapsuleRadius();
	return Reach;
}

bool ABaseAIController::IsThreatInTerritory(const AActor* Threat) const
{
	if (!Threat || !AICharacter) return false;
	const float OuterSq = FMath::Square(AICharacter->GetLeashRange() * 1.15f);          // the give-up domain
	const float InnerSq = FMath::Square(AICharacter->GetLeashRange() * LeashReengageRatio); // re-engage dead band
	// In territory if the threat is within the leash DOMAIN of home (1.15), OR the WOLF itself is back within the
	// inner re-engage radius. The wolf-vs-spawn clause uses the inner ratio so a returning wolf can't flip at the
	// boundary (give up at 1.15, re-acquire only once back inside Inner) — that hysteresis kills the stutter.
	return FVector::DistSquared(Threat->GetActorLocation(), SpawnOrigin) <= OuterSq
		|| FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin) <= InnerSq;
}

bool ABaseAIController::IsSelfOutsideLeash() const
{
	if (!AICharacter) return false;
	// Inner re-engage radius (not the 1.15 give-up) so a leashed wolf must travel back inside the dead band before it
	// may cold-acquire again — no knife-edge flip at the tether.
	return FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin)
		> FMath::Square(AICharacter->GetLeashRange() * LeashReengageRatio);
}

bool ABaseAIController::CheckLeashAndReturn()
{
	const float GiveUpSq = FMath::Square(AICharacter->GetLeashRange() * 1.15f);
	AActor* T = AICharacter->GetCurrentTarget();

	// A wolf toe-to-toe with its target is committed — don't yank it home through the player just because the fight
	// drifted far from ITS spawn. Only the SELF-tether give-up is suppressed in melee; bTargetGone still ends a fight
	// the target genuinely fled, and the leash resumes the moment the target leaves attack range.
	const bool bInMelee = T && FVector::DistSquared(AICharacter->GetActorLocation(), T->GetActorLocation())
		<= FMath::Square(GetEffectiveAttackRange());
	const bool bSelfTooFar = !bInMelee && FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin) > GiveUpSq;
	// Give up on the TARGET by how far it is from US (the chaser), not from spawn — so walking toward the pack can
	// never make a wolf quit on a player standing next to it. The wolf's own tether (bSelfTooFar) stays spawn-relative.
	const bool bTargetGone = !T || FVector::DistSquared(AICharacter->GetActorLocation(), T->GetActorLocation()) > GiveUpSq;
	if (!bSelfTooFar && !bTargetGone) return false;

	ReleaseAttackTokenHeld();
	LostSightTimer = -1.f;
	AICharacter->ClearTarget();
	AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Alert);
	AICharacter->SetAIState(EAIState::Returning);
	return true;
}

/* ═══════════ State Handlers ═══════════ */

void ABaseAIController::HandleIdleState(float DeltaTime)
{
	UAIAnimationComponent* Anim = AICharacter->GetAIAnimation();
	if (Anim && Anim->IsPlayingIdleVariation()) return;

	IdleTimer += DeltaTime;
	if (IdleTimer >= NextIdleAnimTime)
	{
		IdleTimer = 0.f;
		NextIdleAnimTime = IdleAnimInterval + FMath::FRandRange(0.f, IdleAnimRandomDeviation);
		if (Anim) Anim->PlayRandomIdle();
	}

	// Don't snap back to Unaware while a detection is building, we're looking at something, or a heard-noise
	// suspicion is still alive (the Tick timer handles that fade — same lifetime in Idle and Patrol).
	if (AICharacter->GetAwarenessLevel() > EAIAwarenessLevel::Unaware
		&& !PendingDetectTarget.IsValid() && !NoticedActor.IsValid() && SuspicionTimer <= 0.f)
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);

	// Ambient look-around: an unaware, stationary NPC slowly turns its head now and then instead of staring straight.
	if (AICharacter->GetAwarenessLevel() == EAIAwarenessLevel::Unaware
		&& !NoticedActor.IsValid() && !PendingDetectTarget.IsValid()
		&& AICharacter->GetVelocity().SizeSquared2D() < 10000.f)
	{
		ScanTimer -= DeltaTime;
		if (ScanTimer <= 0.f)
		{
			ScanTimer = FMath::FRandRange(2.5f, 5.f);
			const float Sign = (FMath::FRand() < 0.5f) ? -1.f : 1.f;
			ScanGoalYaw = AICharacter->GetActorRotation().Yaw + Sign * FMath::FRandRange(30.f, 70.f);
		}
		const FRotator Cur = AICharacter->GetActorRotation();
		AICharacter->SetActorRotation(FMath::RInterpTo(Cur, FRotator(Cur.Pitch, ScanGoalYaw, Cur.Roll), DeltaTime, 2.f));
	}
}

void ABaseAIController::HandlePatrolState(float DeltaTime)
{
	UAIAnimationComponent* Anim = AICharacter->GetAIAnimation();
	if (Anim && Anim->IsPlayingIdleVariation()) return;

	if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
	{
		if (MC->HasReachedDestination()) HandleIdleState(DeltaTime);
		else IdleTimer = 0.f;
	}
}

void ABaseAIController::HandleChaseState(float DeltaTime)
{
	AActor* Target = AICharacter->GetCurrentTarget();
	if (!Target || AICharacter->IsTargetDeadOrInvalid(Target))
	{ ReleaseAttackTokenHeld(); AICharacter->ClearTarget(); AICharacter->SetAIState(EAIState::Returning); return; }
	if (CheckLeashAndReturn()) return;

	// Custom (scripted) offense: skip the built-in approach/slot loop entirely. Lock into the Attacking state and
	// let BP drive everything (stationary bosses, arena-wide hazards, etc.). BP owns engagement, not the range gate.
	if (bUseCustomAttackLogic)
	{
		if (UAIMovementComponent* MC = AICharacter->GetAIMovement()) MC->StopMovement();
		AICharacter->SetAIState(EAIState::Attacking);
		return;
	}

	const float Range = GetEffectiveAttackRange();
	const float Dist = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());
	const float SurfDist = FMath::Max(0.f, Dist - GetCombatReach(Target)); // body-surface gap (capsule-aware)

	// In range → commit to the attack regardless of LoS flicker (the hit-window itself enforces line of sight).
	if (SurfDist <= Range)
	{
		AICharacter->SetAIState(EAIState::Attacking);
		if (UAIMovementComponent* MC = AICharacter->GetAIMovement()) MC->StopMovement();
		return;
	}

	// Lost sight while still OUT of range → head to where we last saw THIS target instead of tracking it through
	// walls. (LostSightTimer eventually fires → search. Gated on LastKnownActor so a switched target uses live pos.)
	if (!IsTargetCurrentlySeen(Target) && LastKnownActor.Get() == Target && !LastKnownLocation.IsZero())
	{
		if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
		{ MC->SetDesiredSpeed(GetChaseSpeed()); MC->MoveToLocation(LastKnownLocation, MC->AcceptanceRadius); }
		return;
	}

	if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
	{ MC->SetDesiredSpeed(GetChaseSpeed()); MC->MoveToLocation(GetAttackSlotLocation(Target, GetCombatApproachDistance(Range), DeltaTime), CombatPositionAcceptance); }
}

void ABaseAIController::HandleAttackState(float DeltaTime)
{
	AActor* Target = AICharacter->GetCurrentTarget();
	if (!Target || AICharacter->IsTargetDeadOrInvalid(Target))
	{ ReleaseAttackTokenHeld(); AICharacter->ClearTarget(); AICharacter->SetAIState(EAIState::Returning); return; }
	if (CheckLeashAndReturn()) return;

	const float Range = GetEffectiveAttackRange();
	const float Dist  = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());
	const float SurfDist = FMath::Max(0.f, Dist - GetCombatReach(Target)); // body-surface gap; range checks use this, positions use Dist

	// Custom (scripted) offense: BP owns the whole attack pattern. Skip the built-in melee/slot/token/evade/kite
	// loop and never auto-disengage on range — the boss stays engaged until BP/perception says otherwise.
	if (bUseCustomAttackLogic)
	{
		UAICombatComponent* CC = AICharacter->GetAICombat();
		if (bCustomLogicFacesTarget && (!CC || !CC->IsAttacking()))
			FaceTargetYawOnly(Target, DeltaTime);
		TickCustomAttackLogic(Target, Dist, DeltaTime);
		return;
	}

	if (SurfDist > Range * CombatDisengageRangeRatio)
	{
		ReleaseAttackTokenHeld();
		// Chasing is also a combat state, so this transition won't ExitCombat — cancel a wind-up charge
		// explicitly so it doesn't auto-release into empty air after the target backed off.
		if (UAICombatComponent* C = AICharacter->GetAICombat()) C->CancelCharge();
		AICharacter->SetAIState(EAIState::Chasing);
		return;
	}

	UAICombatComponent* Combat = AICharacter->GetAICombat();
	UAIMovementComponent* MC = AICharacter->GetAIMovement();

	// Commit the swing direction: stop re-aiming once the active attack is underway, so dodging
	// perpendicular actually leaves the attack's arc (kills the aim-bot tracking).
	if (!Combat || !Combat->IsAttacking())
		FaceTargetYawOnly(Target, DeltaTime);

	// Rooted, punishable recovery after a committed attack — don't move or attack during it.
	if (Combat && Combat->IsInRecovery())
	{
		if (MC) MC->StopMovement();
		return;
	}

	// First-contact size-up: a brief beat the first time we reach a NEW target so the AI doesn't swing the exact
	// frame it arrives in range (facing is already handled by the gate above).
	if (Target != EngagedTarget.Get())
	{
		EngagedTarget = Target;
		EngageReactionTimer = (EngageReactionTime > 0.f) ? FMath::FRandRange(0.15f, EngageReactionTime) : 0.f;
	}
	if (EngageReactionTimer > 0.f)
	{
		EngageReactionTimer -= DeltaTime;
		if (MC) MC->StopMovement();
		return;
	}

	// Reactive evade: an Elite/Boss sidesteps/back-hops when the target winds up an attack. (Cooldown ticks in Tick.)
	if (bEvading)
	{
		EvadeTimer -= DeltaTime;
		if (EvadeTimer > 0.f) return; // committed to the dodge
		bEvading = false;
	}
	if (bUseReactiveEvade && MC && EvadeCooldownTimer <= 0.f
		&& static_cast<uint8>(AICharacter->GetRank()) >= static_cast<uint8>(EAIRank::Elite)
		&& (!Combat || (!Combat->IsAttacking() && !Combat->IsCharging() && !Combat->IsStaggered()))
		&& AICharacter->IsTargetWindingUpAttack(Target) && FMath::FRand() < EvadeChance)
	{
		const FVector ToTarget = (Target->GetActorLocation() - AICharacter->GetActorLocation()).GetSafeNormal2D();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, ToTarget);
		const float Sign = (FMath::FRand() < 0.5f) ? 1.f : -1.f;
		const FVector EvadeDir = (Right * Sign - ToTarget * 0.3f).GetSafeNormal(); // mostly lateral, slight back
		ReleaseAttackTokenHeld();
		MC->SetDesiredSpeed(EvadeSpeed);
		MC->MoveToLocation(AICharacter->GetActorLocation() + EvadeDir * EvadeDistance, CombatPositionAcceptance);
		bEvading = true;
		EvadeTimer = EvadeDuration;
		EvadeCooldownTimer = EvadeCooldown;
		return;
	}

	// Kiting: if the target is inside our comfort range, back off to keep distance (ranged/skittish enemies).
	if (MinComfortRange > 0.f && MC && Dist < MinComfortRange && (!Combat || !Combat->IsAttacking()))
	{
		ReleaseAttackTokenHeld(); // we've decided not to attack this frame — free the slot for an ally
		FVector Away = (AICharacter->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		if (Away.IsNearlyZero()) Away = AICharacter->GetActorForwardVector().GetSafeNormal2D();
		MC->SetDesiredSpeed(StrafeSpeed);
		MC->MoveToLocation(Target->GetActorLocation() + Away * MinComfortRange, CombatPositionAcceptance);
		return;
	}

	if (Combat && Combat->GetAttacks().Num() > 0)
	{
		if (Combat->IsAttacking() || Combat->IsCharging() || Combat->IsStaggered())
		{
			if (bHoldingAttackToken) TryTakeAttackTurn(Target);
			return;
		}

		const float SlotRadius = GetCombatApproachDistance(Range);
		const FVector SlotLoc = GetAttackSlotLocation(Target, SlotRadius, DeltaTime);
		// "At slot" only once we've genuinely reached our standoff (tight tolerance) OR our body surface is within the
		// standoff of the target — so the wolf closes IN before swinging instead of attacking from the far edge of range.
		const bool bAtSlot = (FVector::Dist(AICharacter->GetActorLocation(), SlotLoc) <= CombatPositionAcceptance) || (SurfDist <= SlotRadius);

		// Reposition that never recedes past the player: keep the reserved encirclement angle but clamp the radius to
		// no more than our current distance, so an in-close wolf strafes/holds instead of back-pedalling out of range.
		auto GetReposSlot = [&](float CurDist) -> FVector
		{
			const float EffRadius = FMath::Min(SlotRadius, CurDist);
			const FVector Dir = (SlotLoc - Target->GetActorLocation()).GetSafeNormal2D();
			return Dir.IsNearlyZero() ? AICharacter->GetActorLocation()
									  : Target->GetActorLocation() + Dir * FMath::Max(EffRadius, 1.f);
		};

		// Hold back on a wider "wait ring" instead of crowding the target — but never recede OUT of attack range.
		auto WaitOnRing = [&]()
		{
			if (!MC) return;
			// Already in range but maybe short of our slot (e.g. waiting out an attack cooldown): drift onto the
			// reserved slot angle at SlotRadius (radius clamped <= current Dist so we never recede out of range), then
			// hold once settled — ready to pounce the instant the cooldown/turn frees, instead of freezing at the edge.
			if (SurfDist <= Range)
			{
				const FVector InSlot = GetReposSlot(Dist);
				if (FVector::Dist(AICharacter->GetActorLocation(), InSlot) <= CombatPositionAcceptance) MC->StopMovement();
				else { MC->SetDesiredSpeed(StrafeSpeed); MC->MoveToLocation(InSlot, CombatPositionAcceptance); }
				return;
			}
			const FVector Dir = (SlotLoc - Target->GetActorLocation()).GetSafeNormal2D();
			// Clamp the ring UNDER the disengage band so reaching it can't trip Dist>Range*Disengage → Chasing (a flicker).
			const float WaitRadius = FMath::Min(Range * WaitRingRatio, Range * CombatDisengageRangeRatio - 1.f);
			const FVector WaitSlot = Dir.IsNearlyZero() ? SlotLoc : Target->GetActorLocation() + Dir * WaitRadius;
			MC->SetDesiredSpeed(StrafeSpeed);
			MC->MoveToLocation(WaitSlot, CombatPositionAcceptance);
		};

		// Overflow waiter (no attack turn this round): HOLD at our own encirclement slot — never recede. GetReposSlot
		// clamps the radius to our current distance, so as the player advances we hold ground (or close in), we don't
		// back away. Keep facing the target (handled above) and menace/howl on a throttle while we wait our turn.
		auto WaitYourTurn = [&]()
		{
			ReleaseAttackTokenHeld();
			bool bSettled = false;
			if (MC)
			{
				const FVector Slot = GetReposSlot(Dist);
				if (FVector::Dist(AICharacter->GetActorLocation(), Slot) <= CombatPositionAcceptance * 1.5f)
				{ MC->StopMovement(); bSettled = true; }
				else { MC->SetDesiredSpeed(StrafeSpeed); MC->MoveToLocation(Slot, CombatPositionAcceptance); }
			}
			// Menace only while holding still (so a DirectPlayback howl doesn't foot-slide), on a throttle.
			if (bSettled)
			{
				MenaceTimer -= DeltaTime;
				if (MenaceTimer <= 0.f)
				{
					MenaceTimer = CombatWaitMenaceInterval * FMath::FRandRange(0.7f, 1.3f); // desync the pack's howls
					if (UAIAnimationComponent* Anim = AICharacter->GetAIAnimation())
						if (!Anim->IsPlayingAction()) Anim->PlayMenace();
				}
			}
		};

		// Bait: against a defending target, commit to a short hold (no per-frame re-roll) and DON'T hold a token,
		// so an ally can apply pressure instead. Decided before consuming a turn.
		if (BaitTimer > 0.f) BaitTimer -= DeltaTime;
		if (Combat->CanAttack() && AICharacter->IsTargetDefending(Target)
			&& (BaitTimer > 0.f || FMath::FRand() < DefenseBaitChance))
		{
			if (BaitTimer <= 0.f) BaitTimer = BaitHoldDuration; // commit to a deliberate hold window
			ReleaseAttackTokenHeld();
			WaitOnRing();
			return;
		}
		BaitTimer = 0.f;

		// Claim/refresh our attack turn (slot). No slot left → overflow: hold back and wait our turn instead of
		// crowding the melee (fewer bodies in the fray = no pathing jam) while keeping aggro.
		if (!TryTakeAttackTurn(Target))
		{
			WaitYourTurn();
			return;
		}

		if (Combat->CanAttack() && SurfDist <= Range && bAtSlot && HasUsableAttack(Combat, SurfDist) && IsAttackWindowOpen(Target))
		{
			if (Combat->ExecuteRandomAttack(SurfDist))
			{
				NotifyAttackStarted(Target); // stamp the group window ONLY on a confirmed swing
				if (MC) MC->StopMovement();
			}
			else if (MC)
			{
				MC->SetDesiredSpeed(StrafeSpeed);
				MC->MoveToLocation(GetReposSlot(Dist), CombatPositionAcceptance);
			}
		}
		else if (MC)
		{
			// Hold our slot, close in, ready to bite on our turn (own cooldown / group pacing). Never recede out of range.
			MC->SetDesiredSpeed(SurfDist <= Range ? StrafeSpeed : GetChaseSpeed());
			MC->MoveToLocation(GetReposSlot(Dist), CombatPositionAcceptance);
		}
	}
	else if (MC)
	{
		if (Dist > Range) ApproachTarget(Target, GetCombatApproachDistance(Range));
		else MC->StopMovement();
	}
}

FVector ABaseAIController::GetAttackSlotLocation(AActor* Target, float Radius, float DeltaTime)
{
	SlotTimer -= DeltaTime;
	if (!bHasCachedSlot || SlotTimer <= 0.f)
	{
		SlotTargetAngle = ComputeFreeSlotAngle(Target, Radius);
		if (!bHasCachedSlot) { CachedSlotAngle = SlotTargetAngle; bHasCachedSlot = true; }
		SlotTimer = SlotUpdateInterval;
	}

	// Rotate the held angle smoothly toward the chosen slot (shortest way) instead of snapping 45° — no lateral twitch.
	const float Delta = FMath::FindDeltaAngleRadians(CachedSlotAngle, SlotTargetAngle);
	const float Step = 2.5f * DeltaTime; // ~143°/s
	CachedSlotAngle += FMath::Clamp(Delta, -Step, Step);

	const FVector Dir(FMath::Cos(CachedSlotAngle), FMath::Sin(CachedSlotAngle), 0.f);
	return Target->GetActorLocation() + Dir * FMath::Max(Radius, 1.f);
}

float ABaseAIController::ComputeFreeSlotAngle(AActor* Target, float Radius)
{
	const FVector PlayerLoc = Target->GetActorLocation();
	FVector ToMe = (AICharacter->GetActorLocation() - PlayerLoc).GetSafeNormal2D();
	if (ToMe.IsNearlyZero()) ToMe = -AICharacter->GetActorForwardVector().GetSafeNormal2D();
	const float MyAngle = FMath::Atan2(ToMe.Y, ToMe.X);

	// Global slot coordination: reserve a distinct angular lane through the director so packmates spread around
	// the target instead of all driving to the same side. The lane we currently hold is our preferred lane, so we
	// keep it unless another attacker already claimed it. Falls back to the local scan below when no director exists.
	if (bCoordinateAttackSlots)
	{
		if (UAICombatDirectorSubsystem* Dir = GetCombatDirector())
		{
			const float SlotLease = FMath::Max(SlotUpdateInterval * 3.f, 1.f);
			return Dir->ReserveAttackAngle(Target, AICharacter, MyAngle,
				FMath::DegreesToRadians(SlotSeparationDegrees), SlotLease);
		}
	}

	TArray<float> Occupied;
	if (UWorld* W = GetWorld())
	{
		TArray<FOverlapResult> Overlaps;
		const FCollisionShape Sphere = FCollisionShape::MakeSphere(FMath::Max(Radius * 2.5f, 400.f));
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(AICharacter);
		Params.AddIgnoredActor(Target);
		W->OverlapMultiByObjectType(Overlaps, PlayerLoc, FQuat::Identity,
			FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), Sphere, Params);

		for (const FOverlapResult& O : Overlaps)
		{
			AActor* A = O.GetActor();
			if (!A || A == AICharacter || A == Target || !Cast<ABaseAICharacter>(A)) continue;
			const FVector D = (A->GetActorLocation() - PlayerLoc).GetSafeNormal2D();
			if (!D.IsNearlyZero()) Occupied.Add(FMath::Atan2(D.Y, D.X));
		}
	}

	if (Occupied.Num() == 0) return MyAngle;

	const int32 NumCandidates = 8;
	float BestAngle = MyAngle;
	float BestScore = TNumericLimits<float>::Lowest();
	for (int32 i = 0; i < NumCandidates; ++i)
	{
		const float Ang = MyAngle + (2.f * PI * i / NumCandidates);
		float MinClear = PI;
		for (const float OA : Occupied)
			MinClear = FMath::Min(MinClear, FMath::Abs(FMath::FindDeltaAngleRadians(Ang, OA)));

		// Stronger pull toward the lane we already hold (0.6) so we don't hop slots over minor crowd changes.
		const float Score = MinClear - FMath::Abs(FMath::FindDeltaAngleRadians(Ang, MyAngle)) * 0.6f;
		if (Score > BestScore) { BestScore = Score; BestAngle = Ang; }
	}
	return BestAngle;
}

void ABaseAIController::HandleReturnState(float DeltaTime)
{
	UAIMovementComponent* MC = AICharacter->GetAIMovement();
	if (!MC) return;

	// Keep scanning on the way home so a player standing next to a returning AI re-aggros it — BUT only once we're
	// back inside our own leash. Re-aggroing while still beyond the tether would be undone next tick by
	// CheckLeashAndReturn's bSelfTooFar, producing a Return->Chase->Return flip every 0.4s (the jerk/rollback).
	const bool bWolfInLeash = FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin)
		<= FMath::Square(AICharacter->GetLeashRange() * LeashReengageRatio); // inner ratio: re-aggro only once well home
	if (bWolfInLeash)
	{
		ReacquireCooldown -= DeltaTime;
		if (ReacquireCooldown <= 0.f)
		{
			ReacquireCooldown = 0.4f;
			if (ReacquireOnReturn()) return; // spotted the player → engage
		}
	}

	MC->SetDesiredSpeed(MC->PatrolSpeed);
	const bool bMoving = MC->MoveToLocation(SpawnOrigin);

	// 2D arrival + generous Z band so standing on a slope/step still counts as "home".
	const bool bArrived =
		FVector::DistSquared2D(AICharacter->GetActorLocation(), SpawnOrigin) <= FMath::Square(MC->AcceptanceRadius + 50.f)
		&& FMath::Abs(AICharacter->GetActorLocation().Z - SpawnOrigin.Z) <= 200.f;

	// Watchdog: if home is unreachable (knocked off-nav, blocked door…), hard-teleport instead of shuffling forever.
	// A return move that can't even be issued (no path) accelerates the watchdog so a wedged AI un-sticks sooner —
	// but only 2x: the teleport is a visible position snap, and 4x rushed it on a transient one-frame repath miss.
	ReturnTimer += bMoving ? DeltaTime : DeltaTime * 2.f;
	if (!bArrived && ReturnTimer >= ReturnTimeout)
	{
		AICharacter->TeleportToSpawn(SpawnOrigin); // land on the reachable, nav-projected origin (not the raw spawn)
		return;
	}

	if (bArrived)
	{
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
		if (ReacquireOnReturn()) return;
		if (MC->PatrolMode != EPatrolMode::Stationary) { AICharacter->SetAIState(EAIState::Patrolling); MC->StartPatrol(); }
		else AICharacter->SetAIState(EAIState::Idle);
	}
}

void ABaseAIController::HandleFleeState(float DeltaTime)
{
	UAIMovementComponent* MC = AICharacter->GetAIMovement();
	if (!MC) return;

	// Panicked NPCs may leave their territory: only a GENEROUS hard cap pulls them back. The normal leash
	// would yank them home through the threat — bad for prey.
	if (FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin) > FMath::Square(AICharacter->GetLeashRange() * 2.f))
	{
		AICharacter->ClearTarget();
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

	AActor* Target = AICharacter->GetCurrentTarget();

	// No target — return home
	if (!Target)
	{
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

	const float DistToThreat = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());

	// Safe distance — stop fleeing
	if (DistToThreat >= FleeSafeDistance)
	{
		AICharacter->ClearTarget();
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

	// ── PANIC MODE: threat is very close — recalc flee on a fast throttle (not literally every frame,
	// which would fire a NavMesh projection storm for a whole herd) ──
	if (DistToThreat <= FleePanicRadius)
	{
		FleeReevalTimer -= DeltaTime;
		if (FleeReevalTimer <= 0.f) { FleeReevalTimer = FleePanicReevalInterval; bFleeCornered = !MC->FleeFrom(Target); }
		if (bFleeCornered) { MC->StopMovement(); FaceTargetYawOnly(Target, DeltaTime); } // trapped — turn and face the threat
		return;
	}

	// ── NORMAL MODE: recalc on timer or when destination reached ──
	FleeReevalTimer -= DeltaTime;
	if (FleeReevalTimer <= 0.f || MC->HasReachedDestination())
	{
		FleeReevalTimer = FleeReevalInterval;
		bFleeCornered = !MC->FleeFrom(Target);
	}
	if (bFleeCornered) { MC->StopMovement(); FaceTargetYawOnly(Target, DeltaTime); }
}

void ABaseAIController::HandleInteractState(float DeltaTime)
{
	AActor* Partner = AICharacter->GetInteractionPartner();

	// Safety net: if the partner is gone/dead or has walked away, leave dialogue ourselves (the BP flow normally
	// calls EndInteraction, but the player can die/disconnect/run off without it).
	const bool bPartnerGone = !IsValid(Partner) || AICharacter->IsTargetDeadOrInvalid(Partner);
	const bool bTooFar = Partner && FVector::DistSquared(AICharacter->GetActorLocation(), Partner->GetActorLocation())
		> FMath::Square(MaxInteractDistance);
	if (bPartnerGone || bTooFar)
	{
		AICharacter->EndInteraction(); // clears partner, fires OnInteractionEnded, resumes patrol/idle
		return;
	}

	// Hold position and face the conversation partner; the BP dialogue flow calls EndInteraction when done.
	FaceTargetYawOnly(Partner, DeltaTime);
}

void ABaseAIController::HandleStaggerState(float DeltaTime)
{
	UAICombatComponent* C = AICharacter->GetAICombat();
	if (!C || !C->IsStaggered())
	{
		AActor* T = AICharacter->GetCurrentTarget();
		if (T && AICharacter->ShouldEngageTargets())
		{
			AICharacter->SetAIState(EAIState::Chasing);
		}
		else if (T && AICharacter->CanFlee())
		{
			// A staggered fleer (passive/neutral) resumes fleeing instead of walking home into the threat.
			AICharacter->SetAIState(EAIState::Fleeing);
			if (UAIMovementComponent* MC = AICharacter->GetAIMovement()) MC->FleeFrom(T);
		}
		else
		{
			AICharacter->SetAIState(EAIState::Returning);
		}
	}
}

/* ═══════════ Debug ═══════════ */

void ABaseAIController::DrawDebugPerception() const
{
#if ENABLE_DRAW_DEBUG
	if (!AICharacter) return;
	const UWorld* W = GetWorld();
	if (!W) return;
	const FVector Loc = AICharacter->GetActorLocation();
	const FVector Fwd = AICharacter->GetActorForwardVector();

	if (bUseProximityDetection)
		DrawDebugSphere(W, Loc, ProximityRadius, 20, FColor::Magenta, false, -1.f, 0, 1.5f);

	const float HalfFOV = FMath::DegreesToRadians(SightFOVDegrees);
	for (int32 i = 0; i < 24; ++i)
	{
		const float A0 = -HalfFOV + (2.f * HalfFOV * i / 24);
		const float A1 = -HalfFOV + (2.f * HalfFOV * (i + 1) / 24);
		DrawDebugLine(W, Loc + Fwd.RotateAngleAxis(FMath::RadiansToDegrees(A0), FVector::UpVector) * SightRadius,
			Loc + Fwd.RotateAngleAxis(FMath::RadiansToDegrees(A1), FVector::UpVector) * SightRadius, FColor::Green, false, -1.f, 0, 1.5f);
	}
	DrawDebugLine(W, Loc, Loc + Fwd.RotateAngleAxis(-SightFOVDegrees, FVector::UpVector) * SightRadius, FColor::Green, false, -1.f, 0, 1.5f);
	DrawDebugLine(W, Loc, Loc + Fwd.RotateAngleAxis(SightFOVDegrees, FVector::UpVector) * SightRadius, FColor::Green, false, -1.f, 0, 1.5f);

	DrawDebugCircle(W, Loc, HearingRange, 32, FColor::Yellow, false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);
	DrawDebugCircle(W, SpawnOrigin, AICharacter->GetLeashRange(), 32, FColor(150,150,150), false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);
	DrawDebugCircle(W, Loc, GetEffectiveAttackRange(), 16, FColor::Orange, false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);

	const FString State = StaticEnum<EAIState>()->GetNameStringByValue(static_cast<int64>(AICharacter->GetCurrentAIState()));
	DrawDebugString(W, Loc + FVector(0,0,100), State, nullptr, FColor::White, -1.f, true);

	if (AActor* T = AICharacter->GetCurrentTarget())
	{
		DrawDebugLine(W, Loc, T->GetActorLocation(), FColor::Red, false, -1.f, 0, 3.f);
		DrawDebugSphere(W, T->GetActorLocation(), 50.f, 8, FColor::Red, false, -1.f, 0, 2.f);
	}
#endif
}