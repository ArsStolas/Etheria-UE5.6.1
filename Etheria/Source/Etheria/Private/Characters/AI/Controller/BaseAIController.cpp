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
#include "Perception/AISenseConfig_Hearing.h"
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

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(3.f);

	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	PC->ConfigureSense(*HearingConfig);

	PC->SetDominantSense(UAISense_Hearing::StaticClass());
	PC->OnTargetPerceptionUpdated.AddDynamic(this, &ABaseAIController::OnPerceptionUpdated);
}

void ABaseAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AICharacter = Cast<ABaseAICharacter>(InPawn);
	if (!AICharacter) return;

	SpawnOrigin = AICharacter->GetActorLocation();

	if (const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Proj;
		if (Nav->ProjectPointToNavigation(SpawnOrigin, Proj, FVector(200.f, 200.f, 300.f)))
			SpawnOrigin = Proj.Location;
	}

	AICharacter->OnAIStateChanged.AddDynamic(this, &ABaseAIController::HandleAIStateChanged);
	AICharacter->OnAIDormancyChanged.AddDynamic(this, &ABaseAIController::HandleDormancyChanged);
	ConfigureCrowdAvoidance();
	AICharacter->SetDetectionDecalRadius(DetectionRadius);

	if (UAICombatComponent* Combat = AICharacter->GetAICombat())
		Combat->OnAIAttackResolved.AddDynamic(this, &ABaseAIController::HandleOwnAttackResolved);

	if (UAIPerceptionComponent* PerComp = GetPerceptionComponent())
	{
		HearingConfig->HearingRange = HearingRange;
		PerComp->RequestStimuliListenerUpdate();
	}

	if (!AICharacter->CanReactToPerception())
		if (UAIPerceptionComponent* PerComp = GetPerceptionComponent())
			PerComp->SetSenseEnabled(UAISense_Hearing::StaticClass(), false);

	DetectionScanTimer = FMath::FRandRange(0.f, 0.12f);
	OrbitDir = (FMath::FRand() < 0.5f) ? -1.f : 1.f;

	NextIdleAnimTime = IdleAnimInterval + FMath::FRandRange(0.f, IdleAnimRandomDeviation);

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
		if (UAICombatComponent* Combat = AICharacter->GetAICombat())
			Combat->OnAIAttackResolved.RemoveDynamic(this, &ABaseAIController::HandleOwnAttackResolved);
	}
	if (UWorld* W = GetWorld())
		W->GetTimerManager().ClearTimer(InitialPatrolTimerHandle);
	Super::OnUnPossess();
}

void ABaseAIController::ConfigureCrowdAvoidance()
{

	if (UCrowdFollowingComponent* Crowd = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
		Crowd->SetCrowdSimulationState(ECrowdSimulationState::Disabled);
}

void ABaseAIController::HandleAIStateChanged(EAIState OldState, EAIState NewState)
{
	if ((OldState == EAIState::Attacking && NewState != EAIState::Attacking) || NewState == EAIState::Dead)
		ReleaseAttackTokenHeld();

	if (NewState == EAIState::Returning) ReturnTimer = 0.f;
	if (NewState == EAIState::Fleeing) { bFleeCornered = false; FleeReevalTimer = 0.f; bFleeingHome = false; FleeCalmTimer = 0.f; FleeLostSightTimer = -1.f; }

	if (OldState == EAIState::Attacking && NewState != EAIState::Attacking)
	{
		bEvading = false; EvadeTimer = 0.f;
		bWindupObserved = false; bEvadeCommitted = false;
		bDefendObserved = false; bBaitCommitted = false;
		PlannedAttackRange = -1.f;
		CombatStallTimer = 0.f;
		bSteerCombat = false;
		bSteerSettled = false;
		bStallRepath = false;
		StallRepathTime = 0.f;
		if (AICharacter)
			if (UCharacterMovementComponent* Move = AICharacter->GetCharacterMovement())
				Move->bCanWalkOffLedges = true;
	}

	if (NewState == EAIState::Returning || NewState == EAIState::Idle || NewState == EAIState::Patrolling
		|| NewState == EAIState::Investigating || NewState == EAIState::Dead)
	{ EngagedTarget = nullptr; EngageReactionTimer = 0.f; }

	if (NewState == EAIState::Returning || NewState == EAIState::Idle || NewState == EAIState::Patrolling)
	{ LastKnownActor = nullptr; LastKnownLocation = FVector::ZeroVector; LastKnownVelocity = FVector::ZeroVector; }

	if (AICharacter)
		if (UCharacterMovementComponent* MC = AICharacter->GetCharacterMovement())
			MC->bOrientRotationToMovement = (NewState != EAIState::Attacking);
}

void ABaseAIController::HandleDormancyChanged()
{
	if (!AICharacter) return;
	if (AICharacter->IsDormant()) { ReleaseAttackTokenHeld(); return; }

	const EAIState S = AICharacter->GetCurrentAIState();
	if (S == EAIState::Interacting) { AICharacter->EndInteraction(); return; }
	if (S == EAIState::Investigating || S == EAIState::Staggered)
	{
		UAICombatComponent* C = AICharacter->GetAICombat();
		if (S == EAIState::Staggered && C && C->IsStaggered()) return;
		AICharacter->SetAIState((AICharacter->GetCurrentTarget() && AICharacter->ShouldEngageTargets())
			? EAIState::Chasing : EAIState::Returning);
		return;
	}

	if (S == EAIState::Idle || S == EAIState::Patrolling)
		if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
			if (MC->PatrolMode != EPatrolMode::Stationary && !MC->IsPatrolling())
			{
				AICharacter->SetAIState(EAIState::Patrolling);
				MC->StartPatrol();
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
	if (!bUseAttackTokens || (AttackInterval <= 0.f && AttackHitGrace <= 0.f)) return true;
	if (UAICombatDirectorSubsystem* Dir = GetCombatDirector())
		return Dir->IsAttackWindowOpen(Target, AttackInterval, AttackHitGrace);
	return true;
}

void ABaseAIController::HandleOwnAttackResolved(const FAIAttackData& Attack, AActor* Target, bool bHitConnected)
{

	if (bHitConnected && bUseAttackTokens && AttackHitGrace > 0.f && Target)
		if (UAICombatDirectorSubsystem* Dir = GetCombatDirector())
			Dir->NotifyAttackConnected(Target);
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

bool ABaseAIController::IsNavmeshReadyNear(const FVector& Loc) const
{
	const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!Nav) return false;
	FNavLocation Proj;
	return Nav->ProjectPointToNavigation(Loc, Proj, FVector(300.f, 300.f, 500.f));
}

void ABaseAIController::TryStartInitialPatrol()
{

	if (!AICharacter || AICharacter->IsDead() || AICharacter->IsDormant()) return;

	UAIMovementComponent* MC = AICharacter->GetAIMovement();
	if (!MC || MC->PatrolMode == EPatrolMode::Stationary) return;

	const EAIState State = AICharacter->GetCurrentAIState();
	if (State != EAIState::Idle && State != EAIState::Patrolling) return;

	if (!IsNavmeshReadyNear(AICharacter->GetActorLocation()) && InitialPatrolAttempts++ < MaxInitialPatrolAttempts)
	{
		if (UWorld* W = GetWorld())
			W->GetTimerManager().SetTimer(InitialPatrolTimerHandle, this,
				&ABaseAIController::TryStartInitialPatrol, 0.5f, false);
		return;
	}

	AICharacter->SetAIState(EAIState::Patrolling);
	MC->StartPatrol();
}

void ABaseAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!AICharacter || AICharacter->IsDormant()) return;

	if (EvadeCooldownTimer > 0.f) EvadeCooldownTimer -= DeltaTime;

	if (LostSightTimer >= 0.f)
	{
		LostSightTimer -= DeltaTime;
		if (LostSightTimer <= 0.f)
		{
			const EAIState Cur = AICharacter->GetCurrentAIState();
			if (Cur == EAIState::Chasing || Cur == EAIState::Attacking) StartSearchAtLastKnown();
			else { LostSightTimer = -1.f; AICharacter->ClearTarget(); }
		}
	}

	UpdateDetection(DeltaTime);

	if (NoticedActor.IsValid() && NoticeTimer > 0.f)
	{
		const EAIState St = AICharacter->GetCurrentAIState();
		if (St == EAIState::Idle || St == EAIState::Patrolling)
		{

			if (AICharacter->GetVelocity().SizeSquared2D() < 10000.f)
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

	if (SuspicionTimer > 0.f)
	{
		SuspicionTimer -= DeltaTime;

		if (SuspicionTimer <= 0.f && !PendingDetectTarget.IsValid()
			&& AICharacter->GetAwarenessLevel() == EAIAwarenessLevel::Suspicious)
			AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
	}

	if (!bHomeAnchored && AICharacter->GetCurrentAIState() == EAIState::Idle
		&& AICharacter->GetAwarenessLevel() == EAIAwarenessLevel::Unaware
		&& AICharacter->GetVelocity().SizeSquared() < 400.f)
	{
		bHomeAnchored = true;
		SpawnOrigin = AICharacter->GetActorLocation();
		if (const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{ FNavLocation Proj; if (Nav->ProjectPointToNavigation(SpawnOrigin, Proj, FVector(200.f, 200.f, 300.f))) SpawnOrigin = Proj.Location; }
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

void ABaseAIController::FaceTargetYawOnly(AActor* Target, float DeltaTime, float RotationSpeedOverride)
{
	if (!Target || !AICharacter) return;
	const FVector Dir = (Target->GetActorLocation() - AICharacter->GetActorLocation()).GetSafeNormal2D();
	if (Dir.IsNearlyZero()) return;
	const float Speed = (RotationSpeedOverride > 0.f) ? RotationSpeedOverride : FaceTargetRotationSpeed;
	const FRotator Cur = AICharacter->GetActorRotation();
	const FRotator Goal = FRotator(Cur.Pitch, Dir.Rotation().Yaw, Cur.Roll);
	AICharacter->SetActorRotation(FMath::RInterpTo(Cur, Goal, DeltaTime, Speed));
}

void ABaseAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!AICharacter || !Actor || AICharacter->IsDormant()) return;

	if (Stimulus.Type != UAISense::GetSenseID<UAISense_Hearing>()) return;
	if (!Stimulus.WasSuccessfullySensed()) return;
	if (!IsThreatInTerritory(Actor)) return;
	if (!AICharacter->IsValidTargetCandidate(Actor)) return;
	if (!AICharacter->CanReactToPerception()) return;

	const EAIState Cur = AICharacter->GetCurrentAIState();
	if (AICharacter->GetCurrentTarget() == Actor) return;
	if (Cur == EAIState::Chasing || Cur == EAIState::Attacking) return;

	if (bInvestigateNoises && AICharacter->GetHostilityType() == EAIHostilityType::Aggressive)
		BeginInvestigate(Stimulus.StimulusLocation);
	else if (AICharacter->GetAwarenessLevel() < EAIAwarenessLevel::Alert)
	{
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Suspicious);
		SuspicionTimer = SuspicionDuration;
	}
}

bool ABaseAIController::ReacquireOnReturn()
{
	UWorld* W = GetWorld();
	if (!AICharacter || !W) return false;
	if (!AICharacter->CanReactToPerception()) return false;

	const FVector MyLoc = AICharacter->GetActorLocation();
	const FVector Eye = MyLoc + FVector(0.f, 0.f, 60.f);

	if (AICharacter->OnlyDetectsPlayers())
	{
		APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
		if (!Player || !AICharacter->IsValidTargetCandidate(Player) || !IsThreatInTerritory(Player)) return false;
		if (FVector::DistSquared(MyLoc, Player->GetActorLocation()) > FMath::Square(DetectionRadius)) return false;

		FCollisionQueryParams P;
		P.AddIgnoredActor(AICharacter);
		P.AddIgnoredActor(Player);
		FHitResult Block;
		if (W->LineTraceSingleByChannel(Block, Eye, Player->GetActorLocation() + FVector(0.f, 0.f, 60.f), ECollisionChannel::ECC_Visibility, P))
			return false;

		AICharacter->OnPerceiveTarget(Player);
		return AICharacter->GetCurrentTarget() != nullptr;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(AICharacter);
	W->OverlapMultiByObjectType(Overlaps, MyLoc, FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
		FCollisionShape::MakeSphere(DetectionRadius), Params);

	AActor* Best = nullptr;
	float BestSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* Hit = O.GetActor();
		if (!AICharacter->IsValidTargetCandidate(Hit) || !IsThreatInTerritory(Hit)) continue;

		FCollisionQueryParams LoSParams = Params;
		LoSParams.AddIgnoredActor(Hit);
		FHitResult Block;
		const bool bObstructed = W->LineTraceSingleByChannel(Block, Eye,
			Hit->GetActorLocation() + FVector(0.f, 0.f, 60.f), ECollisionChannel::ECC_Visibility, LoSParams);
		if (bObstructed) continue;

		const float DSq = FVector::DistSquared(MyLoc, Hit->GetActorLocation());
		if (DSq < BestSq) { BestSq = DSq; Best = Hit; }
	}

	if (Best) { AICharacter->OnPerceiveTarget(Best); return AICharacter->GetCurrentTarget() != nullptr; }
	return false;
}

AActor* ABaseAIController::FindPerceptibleTarget(float Radius) const
{
	UWorld* W = GetWorld();
	if (!AICharacter || !W) return nullptr;

	const FVector MyLoc = AICharacter->GetActorLocation();
	const FVector Eye = MyLoc + FVector(0.f, 0.f, 60.f);

	auto HasLineOfSight = [&](AActor* T) -> bool
	{
		FCollisionQueryParams P;
		P.AddIgnoredActor(AICharacter);
		P.AddIgnoredActor(T);
		FHitResult Block;
		return !W->LineTraceSingleByChannel(Block, Eye, T->GetActorLocation() + FVector(0.f, 0.f, 60.f),
			ECollisionChannel::ECC_Visibility, P);
	};

	if (AICharacter->OnlyDetectsPlayers())
	{
		APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
		if (!Player || !AICharacter->IsValidTargetCandidate(Player)) return nullptr;
		if (FVector::DistSquared(MyLoc, Player->GetActorLocation()) > FMath::Square(Radius)) return nullptr;
		return HasLineOfSight(Player) ? Player : nullptr;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(AICharacter);
	W->OverlapMultiByObjectType(Overlaps, MyLoc, FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);

	AActor* Best = nullptr;
	float BestSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* Hit = O.GetActor();
		if (!AICharacter->IsValidTargetCandidate(Hit)) continue;
		const float DSq = FVector::DistSquared(MyLoc, Hit->GetActorLocation());
		if (DSq < BestSq && HasLineOfSight(Hit)) { BestSq = DSq; Best = Hit; }
	}
	return Best;
}

void ABaseAIController::UpdateDetection(float DeltaTime)
{
	if (!AICharacter || AICharacter->IsDormant() || AICharacter->GetCurrentAIState() == EAIState::Dead) return;
	if (!AICharacter->CanReactToPerception()) return;

	if (AActor* Cur = AICharacter->GetCurrentTarget())
	{
		const EAIState State = AICharacter->GetCurrentAIState();
		if (IsTargetCurrentlySeen(Cur))
		{
			LostSightTimer = -1.f;
			FleeLostSightTimer = -1.f;
			LastKnownLocation = Cur->GetActorLocation();
			LastKnownVelocity = Cur->GetVelocity();
			LastKnownActor = Cur;
		}
		else if (State == EAIState::Chasing || State == EAIState::Attacking)
		{
			if (TargetMemoryDuration > 0.f && LostSightTimer < 0.f)
				LostSightTimer = TargetMemoryDuration;
		}
		else if (State == EAIState::Fleeing)
		{

			if (FleeLostSightTimer < 0.f) FleeLostSightTimer = FleeMemoryDuration;
			FleeLostSightTimer -= DeltaTime;
			if (FleeLostSightTimer <= 0.f)
			{
				FleeLostSightTimer = -1.f;
				AICharacter->ClearTarget();
			}
		}
		DetectionProgress = 0.f;
		PendingDetectTarget = nullptr;
		CachedDetectionCandidate = nullptr;
		return;
	}

	DetectionScanTimer -= DeltaTime;
	if (DetectionScanTimer <= 0.f)
	{
		DetectionScanTimer = 0.12f;

		float EffRadius = DetectionRadius;
		if (AICharacter->ShouldEngageTargets() && AICharacter->GetLeashRange() > 0.f)
			EffRadius = FMath::Min(DetectionRadius, AICharacter->GetLeashRange() * 1.15f);
		CachedDetectionCandidate = FindPerceptibleTarget(EffRadius);
	}

	AActor* Cand = CachedDetectionCandidate.Get();
	if (!Cand)
	{
		if (DetectionProgress > 0.f)
			DetectionProgress = FMath::Max(0.f, DetectionProgress - DeltaTime / FMath::Max(DetectionReactionTime, 0.05f));
		if (DetectionProgress <= 0.f)
		{
			PendingDetectTarget = nullptr;

			if (AICharacter->GetAwarenessLevel() == EAIAwarenessLevel::Alert)
				AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Suspicious);
			else if (AICharacter->GetAwarenessLevel() == EAIAwarenessLevel::Suspicious
				&& SuspicionTimer <= 0.f && !NoticedActor.IsValid())
				AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
		}
		return;
	}

	if (DetectionProgress <= 0.f) DetectionReactionScale = FMath::FRandRange(0.85f, 1.3f);
	PendingDetectTarget = Cand;
	if (DetectionReactionTime <= 0.05f) { DetectionProgress = 0.f; PendingDetectTarget = nullptr; CachedDetectionCandidate = nullptr; AICharacter->OnPerceiveTarget(Cand); return; }

	const float D = FVector::Dist(AICharacter->GetActorLocation(), Cand->GetActorLocation());
	const float CloseScale = FMath::GetMappedRangeValueClamped(FVector2D(0.f, DetectionRadius), FVector2D(3.f, 1.f), D);
	DetectionProgress += DeltaTime * CloseScale / (FMath::Max(DetectionReactionTime, 0.05f) * DetectionReactionScale);

	if (DetectionProgress >= SuspiciousThreshold && AICharacter->GetAwarenessLevel() < EAIAwarenessLevel::Suspicious)
	{
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Suspicious);
		if (NoticedActor.Get() != Cand) NoticeActor(Cand);
	}
	if (DetectionProgress >= AlertThreshold && AICharacter->GetAwarenessLevel() < EAIAwarenessLevel::Alert)
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Alert);

	if (DetectionProgress >= 1.f)
	{
		DetectionProgress = 0.f;
		PendingDetectTarget = nullptr;
		CachedDetectionCandidate = nullptr;
		AICharacter->OnPerceiveTarget(Cand);
	}
}

bool ABaseAIController::IsTargetCurrentlySeen(AActor* Target)
{
	if (!Target || !AICharacter) return false;

	const bool bEngagedOnIt = (AICharacter->GetCurrentTarget() == Target);
	const float Radius = bEngagedOnIt ? FMath::Max(DetectionRadius, LoseSightRadius) : DetectionRadius;
	const float DistSq = FVector::DistSquared(AICharacter->GetActorLocation(), Target->GetActorLocation());
	if (!(AICharacter->WantsInfiniteSightPursuit() && bEngagedOnIt) && DistSq > FMath::Square(Radius))
		return false;

	if (DistSq <= FMath::Square(GetCombatReach(Target) + GetEffectiveAttackRange() + 150.f))
		return true;
	UWorld* W = GetWorld();
	if (!W) return true;
	FCollisionQueryParams P;
	P.AddIgnoredActor(AICharacter);
	P.AddIgnoredActor(Target);
	FHitResult Block;
	const FVector Eye = AICharacter->GetActorLocation() + FVector(0.f, 0.f, 60.f);
	return !W->LineTraceSingleByChannel(Block, Eye, Target->GetActorLocation() + FVector(0.f, 0.f, 60.f),
		ECollisionChannel::ECC_Visibility, P);
}

void ABaseAIController::NotifyTargetConfirmedByDamage(AActor* InstigatorActor)
{
	if (!AICharacter || !InstigatorActor || AICharacter->GetCurrentTarget() != InstigatorActor) return;
	LostSightTimer = -1.f;
	FleeLostSightTimer = -1.f;
	LastKnownLocation = InstigatorActor->GetActorLocation();
	LastKnownVelocity = InstigatorActor->GetVelocity();
	LastKnownActor = InstigatorActor;
}

void ABaseAIController::StartSearchAtLastKnown()
{
	LostSightTimer = -1.f;
	AICharacter->ClearTarget();
	AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Alert);

	if (!LastKnownLocation.IsZero())
	{
		AICharacter->AlertPackSearch(LastKnownLocation);
		BeginInvestigate(LastKnownLocation);
	}
	else AICharacter->SetAIState(EAIState::Returning);
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

	const EAIState S = AICharacter->GetCurrentAIState();
	if (S == EAIState::Chasing || S == EAIState::Attacking || S == EAIState::Fleeing) return;
	BeginInvestigate(Location);
}

void ABaseAIController::ReconcileHostility()
{
	if (!AICharacter) return;

	if (UAIPerceptionComponent* PC = GetPerceptionComponent())
	{
		PC->SetSenseEnabled(UAISense_Hearing::StaticClass(), AICharacter->CanReactToPerception());
		PC->RequestStimuliListenerUpdate();
	}

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
	ReacquireCooldown = 0.f;
	AICharacter->SetAIState(EAIState::Investigating);
}

void ABaseAIController::HandleInvestigateState(float DeltaTime)
{
	UAIMovementComponent* MC = AICharacter->GetAIMovement();
	if (!MC) { AICharacter->SetAIState(EAIState::Returning); return; }

	if (FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin) > FMath::Square(AICharacter->GetLeashRange() * 1.15f))
	{
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

	InvestigateTimer -= DeltaTime;

	ReacquireCooldown -= DeltaTime;
	if (ReacquireCooldown <= 0.f)
	{
		ReacquireCooldown = 0.4f;
		if (ReacquireOnReturn()) return;
	}

	if (InvestigateTimer <= 0.f)
	{
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

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
	MC->MoveToLocation(Standoff, -1.f, true);
}

float ABaseAIController::GetCombatApproachDistance(float Range) const
{
	float Dist = Range * CombatEngageRangeRatio;
	Dist = FMath::Min(Dist, Range - CombatPositionAcceptance - 15.f);
	return FMath::Max(Dist, 1.f);
}

float ABaseAIController::GetCapsuleScale() const
{
	if (AICharacter)
		if (const UCapsuleComponent* Cap = AICharacter->GetCapsuleComponent())
		{
			const float Unscaled = Cap->GetUnscaledCapsuleRadius();
			if (Unscaled > 1.f) return FMath::Max(Cap->GetScaledCapsuleRadius() / Unscaled, 0.1f);
		}
	return 1.f;
}

void ABaseAIController::SteerToPoint(const FVector& Point, float MaxSpeed, float SettleRadius)
{
	UAIMovementComponent* MC = AICharacter->GetAIMovement();
	if (MC)
	{
		MC->CancelPathMove();
		MC->SetDesiredSpeed(MaxSpeed);
	}
	if (UCharacterMovementComponent* Move = AICharacter->GetCharacterMovement())
	{
		if (!Move->IsActive()) Move->Activate(true);
		if (Move->MovementMode == MOVE_None) Move->SetMovementMode(MOVE_Walking);
	}
	FVector To = Point - AICharacter->GetActorLocation();
	To.Z = 0.f;
	const float D = To.Size();
	if (bSteerSettled)
	{
		if (D <= SettleRadius * 1.8f) return;
		bSteerSettled = false;
	}
	else if (D <= SettleRadius)
	{
		bSteerSettled = true;
		return;
	}
	const float InputScale = FMath::Clamp(D / (SettleRadius + 120.f * GetCapsuleScale()), 0.3f, 1.f);
	AICharacter->AddMovementInput(To / D, InputScale);
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
	const float OuterSq = FMath::Square(AICharacter->GetLeashRange() * 1.15f);
	const float InnerSq = FMath::Square(AICharacter->GetLeashRange() * LeashReengageRatio);

	return FVector::DistSquared(Threat->GetActorLocation(), SpawnOrigin) <= OuterSq
		|| FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin) <= InnerSq;
}

bool ABaseAIController::IsSelfOutsideLeash() const
{
	if (!AICharacter) return false;

	return FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin)
		> FMath::Square(AICharacter->GetLeashRange() * LeashReengageRatio);
}

bool ABaseAIController::CheckLeashAndReturn()
{
	const float GiveUpSq = FMath::Square(AICharacter->GetLeashRange() * 1.15f);
	AActor* T = AICharacter->GetCurrentTarget();

	const bool bInMelee = T && (FVector::Dist(AICharacter->GetActorLocation(), T->GetActorLocation()) - GetCombatReach(T))
		<= GetEffectiveAttackRange();
	const bool bRelentless = AICharacter->WantsInfiniteSightPursuit() && T && IsTargetCurrentlySeen(T);
	const bool bSelfTooFar = !bInMelee && !bRelentless
		&& FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin) > GiveUpSq;

	if (!bSelfTooFar && T) return false;

	ReleaseAttackTokenHeld();
	LostSightTimer = -1.f;
	AICharacter->ClearTarget();
	AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Alert);
	AICharacter->SetAIState(EAIState::Returning);
	return true;
}

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

	if (AICharacter->GetAwarenessLevel() > EAIAwarenessLevel::Unaware
		&& !PendingDetectTarget.IsValid() && !NoticedActor.IsValid() && SuspicionTimer <= 0.f)
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);

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

	if (!bUseCustomAttackLogic && CheckLeashAndReturn()) return;

	if (UAIAnimationComponent* Anim = AICharacter->GetAIAnimation())
		if (Anim->IsPlayingHitReact()) return;

	if (bUseCustomAttackLogic)
	{
		if (UAIMovementComponent* MC = AICharacter->GetAIMovement()) MC->StopMovement();
		AICharacter->SetAIState(EAIState::Attacking);
		return;
	}

	const float Range = GetEffectiveAttackRange();
	const float Reach = GetCombatReach(Target);
	const float Dist = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());
	const float SurfDist = FMath::Max(0.f, Dist - Reach);
	float ApproachBand = Range * ApproachPercent;
	{
		const UAICombatComponent* CC = AICharacter->GetAICombat();
		if (!CC || CC->GetCombatStyle() != EAICombatStyle::Ranged)
			ApproachBand = FMath::Min(ApproachBand, MaxMeleeStandoff * GetCapsuleScale());
	}
	const float Standoff = Reach + ApproachBand;

	if (SurfDist <= Range)
	{
		AICharacter->SetAIState(EAIState::Attacking);
		if (UAIMovementComponent* MC = AICharacter->GetAIMovement()) MC->StopMovement();
		return;
	}

	if (!IsTargetCurrentlySeen(Target) && LastKnownActor.Get() == Target && !LastKnownLocation.IsZero())
	{
		if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
		{
			const FVector Pursuit = LastKnownLocation + LastKnownVelocity * LostSightPredictTime;
			MC->SetDesiredSpeed(GetChaseSpeed());
			MC->MoveToLocation(Pursuit, MC->AcceptanceRadius, true);
			if (MC->HasReachedDestination())
			{
				MC->StopMovement();

				if (!LastKnownVelocity.IsNearlyZero())
				{
					const FRotator Cur = AICharacter->GetActorRotation();
					const FRotator Goal(Cur.Pitch, LastKnownVelocity.Rotation().Yaw, Cur.Roll);
					AICharacter->SetActorRotation(FMath::RInterpTo(Cur, Goal, DeltaTime, 4.f));
				}
			}
		}
		return;
	}

	if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
	{
		MC->SetDesiredSpeed(GetChaseSpeed());

		MC->MoveToActorDirect(Target, Standoff);
	}
}

void ABaseAIController::HandleAttackState(float DeltaTime)
{
	AActor* Target = AICharacter->GetCurrentTarget();
	if (!Target || AICharacter->IsTargetDeadOrInvalid(Target))
	{ ReleaseAttackTokenHeld(); AICharacter->ClearTarget(); AICharacter->SetAIState(EAIState::Returning); return; }

	if (!bUseCustomAttackLogic && CheckLeashAndReturn()) return;

	UAICombatComponent* Combat = AICharacter->GetAICombat();
	UAIMovementComponent* MC = AICharacter->GetAIMovement();

	if (Combat && Combat->IsBroken())
	{
		if (MC) MC->StopMovement();
		return;
	}

	if (UAIAnimationComponent* HitAnim = AICharacter->GetAIAnimation())
		if (HitAnim->IsPlayingHitReact()) { if (MC) MC->StopMovement(); return; }

	const float Reach = GetCombatReach(Target);
	const float Dist  = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());
	const float SurfDist = FMath::Max(0.f, Dist - Reach);

	if (bUseCustomAttackLogic)
	{
		if (bCustomLogicFacesTarget && (!Combat || !Combat->IsAttacking()))
			FaceTargetYawOnly(Target, DeltaTime);
		TickCustomAttackLogic(Target, Dist, DeltaTime);
		return;
	}

	const float Range = GetEffectiveAttackRange();
	if (SurfDist > Range * CombatDisengageRangeRatio)
	{
		ReleaseAttackTokenHeld();

		if (Combat) Combat->CancelCharge();
		AICharacter->SetAIState(EAIState::Chasing);
		return;
	}

	{
		bool bFreezeAim = false;
		float FaceSpeed = CombatFaceRotationSpeed;
		if (Combat)
		{
			if (Combat->IsInRecovery()) bFreezeAim = true;
			else if (Combat->IsAttacking())
			{
				if (Combat->HasHitWindowFired() || Combat->GetTimeUntilHitWindow() <= AttackAimLockTime) bFreezeAim = true;
				else FaceSpeed = WindupTrackRotationSpeed;
			}
			else if (Combat->IsCharging())
			{
				if (Combat->GetChargePercent() > 0.55f) bFreezeAim = true;
				else FaceSpeed = WindupTrackRotationSpeed;
			}
		}
		if (!bFreezeAim) FaceTargetYawOnly(Target, DeltaTime, FaceSpeed);
	}

	if (Combat && Combat->IsInRecovery())
	{
		if (MC) MC->StopMovement();
		return;
	}

	if (bEvading)
	{
		EvadeTimer -= DeltaTime;
		if (EvadeTimer > 0.f)
		{
			if (MC) { MC->CancelPathMove(); MC->SetDesiredSpeed(EvadeSpeed); }
			AICharacter->AddMovementInput(EvadeDir, 1.f);
			return;
		}
		bEvading = false;
	}
	if (bUseReactiveEvade && MC
		&& static_cast<uint8>(AICharacter->GetRank()) >= static_cast<uint8>(EAIRank::Elite)
		&& (!Combat || (!Combat->IsAttacking() && !Combat->IsCharging() && !Combat->IsStaggered())))
	{
		const bool bWindup = AICharacter->IsTargetWindingUpAttack(Target);
		if (bWindup && !bWindupObserved)
		{
			bWindupObserved = true;
			bEvadeCommitted = (EvadeCooldownTimer <= 0.f && FMath::FRand() < EvadeChance);
			EvadeReactDelay = FMath::FRandRange(0.06f, 0.2f);
		}
		else if (!bWindup) { bWindupObserved = false; bEvadeCommitted = false; }

		if (bWindup && bEvadeCommitted)
		{
			EvadeReactDelay -= DeltaTime;
			if (EvadeReactDelay <= 0.f)
			{
				bEvadeCommitted = false;
				const FVector ToTarget = (Target->GetActorLocation() - AICharacter->GetActorLocation()).GetSafeNormal2D();
				const FVector Right = FVector::CrossProduct(FVector::UpVector, ToTarget);
				const float Sign = (FMath::FRand() < 0.5f) ? 1.f : -1.f;
				EvadeDir = (Right * Sign - ToTarget * 0.3f).GetSafeNormal();
				ReleaseAttackTokenHeld();
				MC->CancelPathMove();
				MC->SetDesiredSpeed(EvadeSpeed);
				AICharacter->AddMovementInput(EvadeDir, 1.f);
				bEvading = true;
				EvadeTimer = FMath::Max(EvadeDuration, EvadeDistance / FMath::Max(EvadeSpeed, 100.f));
				EvadeCooldownTimer = EvadeCooldown;
				return;
			}
		}
	}

	if (MinComfortRange > 0.f && MC && Dist < MinComfortRange && (!Combat || !Combat->IsAttacking()))
	{
		ReleaseAttackTokenHeld();
		FVector Away = (AICharacter->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		if (Away.IsNearlyZero()) Away = AICharacter->GetActorForwardVector().GetSafeNormal2D();
		MC->CancelPathMove();
		MC->SetDesiredSpeed(StrafeSpeed);
		AICharacter->AddMovementInput(Away, 1.f);
		return;
	}

	if (Combat && Combat->GetAttacks().Num() > 0)
	{
		if (Combat->IsAttacking() || Combat->IsCharging() || Combat->IsStaggered())
		{
			if (bHoldingAttackToken) TryTakeAttackTurn(Target);
			return;
		}

		PlannedRangeTimer -= DeltaTime;
		if (PlannedAttackRange <= 0.f || PlannedRangeTimer <= 0.f)
		{
			PlannedAttackRange = Combat->PickApproachRange(SurfDist, PlannedMinRange);
			PlannedRangeTimer = 3.f;
		}

		const float CapScale = GetCapsuleScale();
		float StandoffBand = PlannedAttackRange * ApproachPercent;
		if (Combat->GetCombatStyle() != EAICombatStyle::Ranged)
			StandoffBand = FMath::Min(StandoffBand, MaxMeleeStandoff * CapScale);
		if (PlannedMinRange > 0.f)
			StandoffBand = FMath::Clamp(FMath::Max(StandoffBand, PlannedMinRange + 15.f), 0.f, FMath::Max(PlannedAttackRange - 10.f, 0.f));
		const float SlotRadius = Reach + StandoffBand;
		const float Settle = FMath::Max(CombatPositionAcceptance * CapScale, 20.f);

		const bool bWasSteer = bSteerCombat;
		if (StandoffBand > 350.f * CapScale) bSteerCombat = false;
		else if (bSteerCombat) { if (Dist > SlotRadius + 500.f * CapScale) bSteerCombat = false; }
		else if (Dist < SlotRadius + 300.f * CapScale) bSteerCombat = true;
		if (bSteerCombat != bWasSteer)
			if (UCharacterMovementComponent* Move = AICharacter->GetCharacterMovement())
				Move->bCanWalkOffLedges = !bSteerCombat;
		if (!bSteerCombat)
		{
			if (MC)
			{
				MC->SetDesiredSpeed(GetChaseSpeed());
				MC->MoveToActorDirect(Target, SlotRadius);
			}
			return;
		}

		const FVector SlotLoc = GetAttackSlotLocation(Target, SlotRadius, DeltaTime);

		auto OrbitStep = [&]()
		{
			if (CombatOrbitSpeed <= 0.f) return;
			if (UAIAnimationComponent* Anim = AICharacter->GetAIAnimation())
				if (Anim->IsPlayingAction()) return;
			OrbitDirTimer -= DeltaTime;
			if (OrbitDirTimer <= 0.f)
			{
				OrbitDirTimer = FMath::FRandRange(3.f, 6.f);
				if (FMath::FRand() < 0.35f) OrbitDir *= -1.f;
			}
			const float Drift = OrbitDir * CombatOrbitSpeed * DeltaTime;
			CachedSlotAngle += Drift;
			SlotTargetAngle += Drift;
		};

		auto SteerSlot = [&](float NearSpeed)
		{
			const float DSlot = FVector::Dist2D(AICharacter->GetActorLocation(), SlotLoc);
			SteerToPoint(SlotLoc, DSlot > 250.f * CapScale ? GetChaseSpeed() : NearSpeed, Settle);
		};

		auto WaitYourTurn = [&]()
		{
			ReleaseAttackTokenHeld();
			OrbitStep();
			SteerSlot(StrafeSpeed);
			if (AICharacter->GetVelocity().SizeSquared2D() < 2500.f)
			{
				MenaceTimer -= DeltaTime;
				if (MenaceTimer <= 0.f)
				{
					MenaceTimer = CombatWaitMenaceInterval * FMath::FRandRange(0.7f, 1.3f);
					if (UAIAnimationComponent* Anim = AICharacter->GetAIAnimation())
						if (!Anim->IsPlayingAction()) Anim->PlayMenace();
				}
			}
		};

		if (Target != EngagedTarget.Get())
		{
			EngagedTarget = Target;
			EngageReactionTimer = (EngageReactionTime > 0.f)
				? FMath::FRandRange(FMath::Min(0.15f, EngageReactionTime), EngageReactionTime) : 0.f;
		}
		if (EngageReactionTimer > 0.f)
		{
			EngageReactionTimer -= DeltaTime;
			OrbitStep();
			SteerSlot(StrafeSpeed * 0.6f);
			return;
		}

		if (BaitTimer > 0.f) BaitTimer -= DeltaTime;
		const bool bDefending = AICharacter->IsTargetDefending(Target);
		if (bDefending && !bDefendObserved) { bDefendObserved = true; bBaitCommitted = (FMath::FRand() < DefenseBaitChance); }
		else if (!bDefending && BaitTimer <= 0.f) { bDefendObserved = false; bBaitCommitted = false; }
		if (BaitTimer > 0.f || (bDefending && bBaitCommitted && Combat->CanAttack()))
		{
			if (BaitTimer <= 0.f) { BaitTimer = BaitHoldDuration; bBaitCommitted = false; }
			ReleaseAttackTokenHeld();
			OrbitStep();
			SteerSlot(StrafeSpeed);
			return;
		}

		if (!TryTakeAttackTurn(Target))
		{
			WaitYourTurn();
			return;
		}

		if (Combat->CanAttack() && SurfDist <= PlannedAttackRange
			&& Dist <= SlotRadius + Settle + 40.f && IsAttackWindowOpen(Target))
		{
			if (Combat->ExecuteRandomAttack(SurfDist))
			{
				NotifyAttackStarted(Target);
				PlannedAttackRange = -1.f;
				CombatStallTimer = 0.f;
				bStallRepath = false;
				StallRepathTime = 0.f;
				if (MC) MC->CancelPathMove();
			}
			else
			{
				OrbitStep();
				SteerSlot(StrafeSpeed);
			}
		}
		else
		{
			const bool bInGate = SurfDist <= PlannedAttackRange && Dist <= SlotRadius + Settle + 40.f;
			if (bStallRepath && MC)
			{
				StallRepathTime += DeltaTime;
				if (bInGate || StallRepathTime > 3.f)
				{
					if (!bInGate) bSteerCombat = false;
					bStallRepath = false;
					StallRepathTime = 0.f;
					MC->CancelPathMove();
				}
				else
				{
					if (!MC->IsPathMoveActive())
					{
						MC->SetDesiredSpeed(GetChaseSpeed());
						MC->MoveToLocation(Target->GetActorLocation(), Reach + FMath::Max(StandoffBand, 40.f), true);
					}
					return;
				}
			}

			if (Combat->CanAttack() && !bInGate
				&& AICharacter->GetVelocity().SizeSquared2D() < 2500.f)
			{
				CombatStallTimer += DeltaTime;
				if (CombatStallTimer > 0.6f)
				{
					CombatStallTimer = 0.f;
					bStallRepath = true;
					StallRepathTime = 0.f;
					return;
				}
			}
			else CombatStallTimer = 0.f;

			OrbitStep();
			SteerSlot(StrafeSpeed);
		}
	}
	else if (MC)
	{
		if (SurfDist > Range) ApproachTarget(Target, GetCombatApproachDistance(Range));
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

	const float Delta = FMath::FindDeltaAngleRadians(CachedSlotAngle, SlotTargetAngle);
	const float Step = 2.5f * DeltaTime;
	CachedSlotAngle += FMath::Clamp(Delta, -Step, Step);

	const FVector Dir(FMath::Cos(CachedSlotAngle), FMath::Sin(CachedSlotAngle), 0.f);
	FVector Slot = Target->GetActorLocation() + Dir * FMath::Max(Radius, 1.f);
	if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Proj;
		if (Nav->ProjectPointToNavigation(Slot, Proj, FVector(150.f, 150.f, 400.f)))
		{
			Slot.X = Proj.Location.X;
			Slot.Y = Proj.Location.Y;
		}
	}
	return Slot;
}

float ABaseAIController::ComputeFreeSlotAngle(AActor* Target, float Radius)
{
	const FVector PlayerLoc = Target->GetActorLocation();
	FVector ToMe = (AICharacter->GetActorLocation() - PlayerLoc).GetSafeNormal2D();
	if (ToMe.IsNearlyZero()) ToMe = -AICharacter->GetActorForwardVector().GetSafeNormal2D();
	const float MyAngle = FMath::Atan2(ToMe.Y, ToMe.X);

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

		const float Score = MinClear - FMath::Abs(FMath::FindDeltaAngleRadians(Ang, MyAngle)) * 0.6f;
		if (Score > BestScore) { BestScore = Score; BestAngle = Ang; }
	}
	return BestAngle;
}

void ABaseAIController::HandleReturnState(float DeltaTime)
{
	UAIMovementComponent* MC = AICharacter->GetAIMovement();
	if (!MC) return;

	if (AICharacter->CanFlee())
	{
		ReacquireCooldown -= DeltaTime;
		if (ReacquireCooldown <= 0.f)
		{
			ReacquireCooldown = 0.3f;
			if (ReacquireOnReturn()) return;
		}
	}
	else
	{

		const bool bWolfInLeash = FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin)
			<= FMath::Square(AICharacter->GetLeashRange() * LeashReengageRatio);
		if (bWolfInLeash)
		{
			ReacquireCooldown -= DeltaTime;
			if (ReacquireCooldown <= 0.f)
			{
				ReacquireCooldown = 0.4f;
				if (ReacquireOnReturn()) return;
			}
		}
	}

	const float HomeDist = FVector::Dist2D(AICharacter->GetActorLocation(), SpawnOrigin);
	MC->SetDesiredSpeed(HomeDist > 600.f ? GetChaseSpeed() : MC->PatrolSpeed);
	const bool bMoving = MC->MoveToLocation(SpawnOrigin);

	const bool bArrived =
		FVector::DistSquared2D(AICharacter->GetActorLocation(), SpawnOrigin) <= FMath::Square(MC->AcceptanceRadius + 50.f)
		&& FMath::Abs(AICharacter->GetActorLocation().Z - SpawnOrigin.Z) <= 200.f;

	ReturnTimer += bMoving ? DeltaTime : DeltaTime * 2.f;
	if (!bArrived && ReturnTimer >= ReturnTimeout)
	{
		AICharacter->TeleportToSpawn(SpawnOrigin);
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

	AActor* Target = AICharacter->GetCurrentTarget();

	if (!Target)
	{
		AICharacter->SetAIState(EAIState::Returning);
		return;
	}

	{
		const float LeashR = AICharacter->GetLeashRange();
		const float DistHomeSq = FVector::DistSquared(AICharacter->GetActorLocation(), SpawnOrigin);

		if (bFleeingHome) { if (DistHomeSq <= FMath::Square(LeashR * 1.5f)) bFleeingHome = false; }
		else if (DistHomeSq > FMath::Square(LeashR * 2.f)) bFleeingHome = true;
		if (bFleeingHome)
		{
			MC->SetDesiredSpeed(MC->FleeSpeed);
			MC->MoveToLocation(SpawnOrigin);
			return;
		}
	}

	const float DistToThreat = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());

	if (DistToThreat >= FleeSafeDistance)
	{
		MC->SetDesiredSpeed(0.f);
		FaceTargetYawOnly(Target, DeltaTime);
		FleeCalmTimer += DeltaTime;
		if (FleeCalmTimer >= FleeCalmDuration)
		{
			FleeCalmTimer = 0.f;
			AICharacter->ClearTarget();
			AICharacter->SetAIState(EAIState::Returning);
		}
		return;
	}
	FleeCalmTimer = 0.f;

	if (DistToThreat <= FleePanicRadius)
	{
		FleeReevalTimer -= DeltaTime;
		if (FleeReevalTimer <= 0.f) { FleeReevalTimer = FleePanicReevalInterval * FMath::FRandRange(0.85f, 1.2f); bFleeCornered = !MC->FleeFrom(Target); }
		if (bFleeCornered) { MC->StopMovement(); FaceTargetYawOnly(Target, DeltaTime); }
		return;
	}

	FleeReevalTimer -= DeltaTime;
	if (FleeReevalTimer <= 0.f || MC->HasReachedDestination())
	{
		FleeReevalTimer = FleeReevalInterval * FMath::FRandRange(0.85f, 1.2f);
		bFleeCornered = !MC->FleeFrom(Target);
	}
	if (bFleeCornered) { MC->StopMovement(); FaceTargetYawOnly(Target, DeltaTime); }
}

void ABaseAIController::HandleInteractState(float DeltaTime)
{
	AActor* Partner = AICharacter->GetInteractionPartner();

	const bool bPartnerGone = !IsValid(Partner) || AICharacter->IsTargetDeadOrInvalid(Partner);
	const bool bTooFar = Partner && FVector::DistSquared(AICharacter->GetActorLocation(), Partner->GetActorLocation())
		> FMath::Square(MaxInteractDistance);
	if (bPartnerGone || bTooFar)
	{
		AICharacter->EndInteraction();
		return;
	}

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

			AICharacter->SetAIState(EAIState::Fleeing);
			if (UAIMovementComponent* MC = AICharacter->GetAIMovement()) MC->FleeFrom(T);
		}
		else
		{
			AICharacter->SetAIState(EAIState::Returning);
		}
	}
}

void ABaseAIController::DrawDebugPerception() const
{
#if ENABLE_DRAW_DEBUG
	if (!AICharacter) return;
	const UWorld* W = GetWorld();
	if (!W) return;
	const FVector Loc = AICharacter->GetActorLocation();

	DrawDebugCircle(W, Loc, DetectionRadius, 32, FColor::Green, false, -1.f, 0, 1.5f, FVector(1,0,0), FVector(0,1,0), false);
	if (LoseSightRadius > DetectionRadius)
		DrawDebugCircle(W, Loc, LoseSightRadius, 32, FColor(0, 120, 0), false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);
	DrawDebugCircle(W, Loc, HearingRange, 32, FColor::Yellow, false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);
	DrawDebugCircle(W, SpawnOrigin, AICharacter->GetLeashRange(), 32, FColor(150,150,150), false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);
	DrawDebugCircle(W, Loc, GetEffectiveAttackRange(), 16, FColor::Orange, false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);

	const FString State = StaticEnum<EAIState>()->GetNameStringByValue(static_cast<int64>(AICharacter->GetCurrentAIState()));
	const FString Info = (DetectionProgress > 0.f)
		? FString::Printf(TEXT("%s  detect %.0f%%"), *State, DetectionProgress * 100.f) : State;
	DrawDebugString(W, Loc + FVector(0,0,100), Info, nullptr, FColor::White, -1.f, true);

	if (AActor* T = AICharacter->GetCurrentTarget())
	{
		DrawDebugLine(W, Loc, T->GetActorLocation(), FColor::Red, false, -1.f, 0, 3.f);
		DrawDebugSphere(W, T->GetActorLocation(), 50.f, 8, FColor::Red, false, -1.f, 0, 2.f);
	}
#endif
}
