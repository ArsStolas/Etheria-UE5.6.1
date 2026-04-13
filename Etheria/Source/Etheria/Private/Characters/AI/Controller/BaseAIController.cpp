/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAIController - Source"
 * Notes: Yaw-only rotation fix. Teleport on leash. Player-only proximity.
 */

#include "Characters/AI/Controller/BaseAIController.h"

#include "Characters/AI/BaseAICharacter.h"
#include "Characters/AI/Movements/AIMovementComponent.h"
#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Characters/AI/Combat/AICombatComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "GameFramework/Character.h"
#include "Engine/OverlapResult.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"

ABaseAIController::ABaseAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	SetupPerception();
}

void ABaseAIController::SetupPerception()
{
	UAIPerceptionComponent* PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SetPerceptionComponent(*PerceptionComp);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = SightFOVDegrees;
	SightConfig->SetMaxAge(5.f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	PerceptionComp->ConfigureSense(*SightConfig);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(3.f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	PerceptionComp->ConfigureSense(*HearingConfig);

	PerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ABaseAIController::OnPerceptionUpdated);
}

void ABaseAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AICharacter = Cast<ABaseAICharacter>(InPawn);
	if (!AICharacter) return;

	SpawnOrigin = AICharacter->GetActorLocation();

	if (UAIPerceptionComponent* PerceptionComp = GetPerceptionComponent())
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = SightFOVDegrees;
		HearingConfig->HearingRange = HearingRange;
		PerceptionComp->RequestStimuliListenerUpdate();
	}

	if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
	{
		if (MC->PatrolMode != EPatrolMode::Stationary)
		{
			AICharacter->SetAIState(EAIState::Patrolling);
			MC->StartPatrol();
		}
	}

	NextIdleAnimTime = IdleAnimInterval + FMath::FRandRange(0.f, IdleAnimRandomDeviation);
}

void ABaseAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!AICharacter || AICharacter->IsDormant()) return;

	if (bUseProximityDetection)
	{
		ProximityTimer -= DeltaTime;
		if (ProximityTimer <= 0.f)
		{
			ProximityTimer = ProximityCheckInterval;
			CheckProximityDetection();
		}
	}

	switch (AICharacter->GetCurrentAIState())
	{
	case EAIState::Idle:       HandleIdleState(DeltaTime);    break;
	case EAIState::Patrolling: HandlePatrolState(DeltaTime);  break;
	case EAIState::Chasing:    HandleChaseState(DeltaTime);   break;
	case EAIState::Attacking:  HandleAttackState(DeltaTime);  break;
	case EAIState::Returning:  HandleReturnState(DeltaTime);  break;
	case EAIState::Fleeing:    HandleFleeState(DeltaTime);    break;
	case EAIState::Staggered:  HandleStaggerState(DeltaTime); break;
	default: break;
	}

#if ENABLE_DRAW_DEBUG
	if (AICharacter->ShouldShowDebugPerception()) DrawDebugPerception();
#endif
}

/* ═══════════ Yaw-only face target ═══════════ */

void ABaseAIController::FaceTargetYawOnly(AActor* Target, float DeltaTime)
{
	if (!Target || !AICharacter) return;

	const FVector MyLoc = AICharacter->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const FVector Dir = (TargetLoc - MyLoc).GetSafeNormal2D(); // 2D = no pitch

	if (Dir.IsNearlyZero()) return;

	const FRotator TargetRot = FRotator(0.f, Dir.Rotation().Yaw, 0.f); // Pitch=0, Roll=0
	const FRotator Current = AICharacter->GetActorRotation();
	const FRotator SmoothRot = FMath::RInterpTo(Current, TargetRot, DeltaTime, FaceTargetRotationSpeed);

	AICharacter->SetActorRotation(FRotator(0.f, SmoothRot.Yaw, 0.f)); // Extra safety: force pitch/roll=0
}

/* ═══════════ Perception ═══════════ */

void ABaseAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!AICharacter || !Actor || AICharacter->IsDormant()) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		AICharacter->OnPerceiveTarget(Actor);
	}
	else
	{
		if (AICharacter->GetCurrentTarget() == Actor)
		{
			const EAIState Cur = AICharacter->GetCurrentAIState();
			AICharacter->ClearTarget();

			if (Cur == EAIState::Chasing || Cur == EAIState::Attacking)
			{
				AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Alert);
				AICharacter->SetAIState(EAIState::Returning);
			}
			else if (Cur == EAIState::Fleeing)
			{
				AICharacter->SetAIState(EAIState::Returning);
			}
		}
	}
}

void ABaseAIController::CheckProximityDetection()
{
	if (!AICharacter || AICharacter->GetCurrentAIState() == EAIState::Dead || AICharacter->IsDormant()) return;

	const FVector MyLoc = AICharacter->GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(ProximityRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(AICharacter);

	GetWorld()->OverlapMultiByObjectType(Overlaps, MyLoc, FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), Sphere, Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor) continue;

		// Skip other AI
		if (Cast<ABaseAICharacter>(HitActor)) continue;

		// Player filter
		if (AICharacter->OnlyDetectsPlayers())
		{
			APawn* P = Cast<APawn>(HitActor);
			if (!P || !P->IsPlayerControlled()) continue;
		}

		AICharacter->OnPerceiveTarget(HitActor);
		break;
	}
}

/* ═══════════ Helpers ═══════════ */

float ABaseAIController::GetEffectiveAttackRange() const
{
	if (UAICombatComponent* C = AICharacter->GetAICombat())
		if (C->GetAttacks().Num() > 0)
			return C->GetEffectiveAttackRange();
	return AttackRange;
}

bool ABaseAIController::CheckLeashAndTeleport()
{
	const float Dist = FVector::Dist(AICharacter->GetActorLocation(), SpawnOrigin);
	if (Dist > AICharacter->GetLeashRange())
	{
		AICharacter->TeleportToSpawn();
		return true;
	}
	return false;
}

/* ═══════════ State Handlers ═══════════ */

void ABaseAIController::HandleIdleState(float DeltaTime)
{
	IdleTimer += DeltaTime;
	if (IdleTimer >= NextIdleAnimTime)
	{
		IdleTimer = 0.f;
		NextIdleAnimTime = IdleAnimInterval + FMath::FRandRange(0.f, IdleAnimRandomDeviation);
		if (UAIAnimationComponent* A = AICharacter->GetAIAnimation()) A->PlayRandomIdle();
	}

	if (AICharacter->GetAwarenessLevel() > EAIAwarenessLevel::Unaware)
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
}

void ABaseAIController::HandlePatrolState(float DeltaTime)
{
	if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
	{
		if (MC->HasReachedDestination()) HandleIdleState(DeltaTime);
		else IdleTimer = 0.f;
	}
}

void ABaseAIController::HandleChaseState(float DeltaTime)
{
	AActor* Target = AICharacter->GetCurrentTarget();
	if (!Target) { AICharacter->SetAIState(EAIState::Returning); return; }
	if (CheckLeashAndTeleport()) return;

	const float Dist = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());
	const float EffRange = GetEffectiveAttackRange();

	if (Dist <= EffRange)
	{
		AICharacter->SetAIState(EAIState::Attacking);
		if (UAIMovementComponent* MC = AICharacter->GetAIMovement()) MC->StopMovement();
		return;
	}

	if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
	{
		MC->SetDesiredSpeed(MC->ChaseSpeed);
		MC->MoveToLocation(Target->GetActorLocation());
	}
}

void ABaseAIController::HandleAttackState(float DeltaTime)
{
	AActor* Target = AICharacter->GetCurrentTarget();
	if (!Target) { AICharacter->SetAIState(EAIState::Returning); return; }

	const float Dist = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());
	const float EffRange = GetEffectiveAttackRange();

	if (Dist > EffRange * 1.3f) { AICharacter->SetAIState(EAIState::Chasing); return; }

	// ──── YAW ONLY rotation ────
	FaceTargetYawOnly(Target, DeltaTime);

	// Combat component attacks
	UAICombatComponent* Combat = AICharacter->GetAICombat();
	if (Combat && Combat->GetAttacks().Num() > 0)
	{
		if (!Combat->IsAttacking() && !Combat->IsCharging() && !Combat->IsStaggered())
			Combat->ExecuteRandomAttack(Dist);

		// Strafe
		if (bStrafeInCombat && !Combat->IsAttacking())
		{
			StrafeTimer -= DeltaTime;
			if (StrafeTimer <= 0.f)
			{
				StrafeTimer = StrafeDirectionChangeInterval + FMath::FRandRange(-0.5f, 0.5f);
				StrafeDirection *= -1;
			}
			const FVector Right = AICharacter->GetActorRightVector() * StrafeDirection * 200.f;
			if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
			{
				MC->SetDesiredSpeed(MC->PatrolSpeed * 0.8f);
				MC->MoveToLocation(AICharacter->GetActorLocation() + Right);
			}
		}
	}
	else
	{
		AttackTimer -= DeltaTime;
		if (AttackTimer <= 0.f)
		{
			AttackTimer = AttackCooldown;
			if (UAIAnimationComponent* A = AICharacter->GetAIAnimation()) A->PlayRandomAttack();
		}
	}
}

void ABaseAIController::HandleReturnState(float DeltaTime)
{
	if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
	{
		MC->SetDesiredSpeed(MC->PatrolSpeed);
		MC->MoveToLocation(SpawnOrigin);

		if (FVector::Dist(AICharacter->GetActorLocation(), SpawnOrigin) < MC->AcceptanceRadius)
		{
			AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
			if (MC->PatrolMode != EPatrolMode::Stationary)
			{
				AICharacter->SetAIState(EAIState::Patrolling);
				MC->StartPatrol();
			}
			else AICharacter->SetAIState(EAIState::Idle);
		}
	}
}

void ABaseAIController::HandleFleeState(float DeltaTime)
{
	UAIMovementComponent* MC = AICharacter->GetAIMovement();
	if (!MC) return;

	if (CheckLeashAndTeleport()) return;

	AActor* Target = AICharacter->GetCurrentTarget();
	if (Target)
	{
		const float Dist = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());
		if (Dist >= FleeSafeDistance)
		{
			AICharacter->ClearTarget();
			AICharacter->SetAIState(EAIState::Returning);
			return;
		}
	}

	if (MC->HasReachedDestination())
	{
		if (Target) MC->FleeFrom(Target);
		else AICharacter->SetAIState(EAIState::Returning);
	}
}

void ABaseAIController::HandleStaggerState(float DeltaTime)
{
	UAICombatComponent* C = AICharacter->GetAICombat();
	if (!C || !C->IsStaggered())
	{
		if (AICharacter->GetCurrentTarget()) AICharacter->SetAIState(EAIState::Chasing);
		else AICharacter->SetAIState(EAIState::Returning);
	}
}

/* ═══════════ Debug ═══════════ */

void ABaseAIController::DrawDebugPerception() const
{
#if ENABLE_DRAW_DEBUG
	if (!AICharacter) return;
	const UWorld* World = GetWorld();
	if (!World) return;

	const FVector Loc = AICharacter->GetActorLocation();
	const FVector Fwd = AICharacter->GetActorForwardVector();

	// Proximity (magenta)
	if (bUseProximityDetection)
		DrawDebugSphere(World, Loc, ProximityRadius, 20, FColor::Magenta, false, -1.f, 0, 1.5f);

	// Sight cone (green)
	const float HalfFOV = FMath::DegreesToRadians(SightFOVDegrees);
	for (int32 i = 0; i < 24; ++i)
	{
		const float A0 = -HalfFOV + (2.f * HalfFOV * i / 24);
		const float A1 = -HalfFOV + (2.f * HalfFOV * (i + 1) / 24);
		const FVector D0 = Fwd.RotateAngleAxis(FMath::RadiansToDegrees(A0), FVector::UpVector);
		const FVector D1 = Fwd.RotateAngleAxis(FMath::RadiansToDegrees(A1), FVector::UpVector);
		DrawDebugLine(World, Loc + D0 * SightRadius, Loc + D1 * SightRadius, FColor::Green, false, -1.f, 0, 1.5f);
	}
	DrawDebugLine(World, Loc, Loc + Fwd.RotateAngleAxis(-SightFOVDegrees, FVector::UpVector) * SightRadius, FColor::Green, false, -1.f, 0, 1.5f);
	DrawDebugLine(World, Loc, Loc + Fwd.RotateAngleAxis(SightFOVDegrees, FVector::UpVector) * SightRadius, FColor::Green, false, -1.f, 0, 1.5f);

	// Hearing (yellow)
	DrawDebugCircle(World, Loc, HearingRange, 32, FColor::Yellow, false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);

	// Leash (gray)
	DrawDebugCircle(World, SpawnOrigin, AICharacter->GetLeashRange(), 32, FColor(150,150,150), false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);

	// Attack range (orange)
	DrawDebugCircle(World, Loc, GetEffectiveAttackRange(), 16, FColor::Orange, false, -1.f, 0, 1.f, FVector(1,0,0), FVector(0,1,0), false);

	// State label
	const FString State = StaticEnum<EAIState>()->GetNameStringByValue(static_cast<int64>(AICharacter->GetCurrentAIState()));
	const FString Aware = StaticEnum<EAIAwarenessLevel>()->GetNameStringByValue(static_cast<int64>(AICharacter->GetAwarenessLevel()));
	DrawDebugString(World, Loc + FVector(0,0,100), FString::Printf(TEXT("%s | %s"), *State, *Aware), nullptr, FColor::White, -1.f, true);

	// Target
	if (AActor* T = AICharacter->GetCurrentTarget())
	{
		DrawDebugLine(World, Loc, T->GetActorLocation(), FColor::Red, false, -1.f, 0, 3.f);
		DrawDebugSphere(World, T->GetActorLocation(), 50.f, 8, FColor::Red, false, -1.f, 0, 2.f);
	}
#endif
}
