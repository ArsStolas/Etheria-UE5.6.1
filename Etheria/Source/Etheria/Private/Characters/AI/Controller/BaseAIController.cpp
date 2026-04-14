/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAIController - Source"
 * Notes: Improved flee with re-evaluation and direction variety.
 *        Idle variations complete before patrol resumes.
 *        Yaw-only rotation toward target.
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

	if (UAIPerceptionComponent* PerComp = GetPerceptionComponent())
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = SightFOVDegrees;
		HearingConfig->HearingRange = HearingRange;
		PerComp->RequestStimuliListenerUpdate();
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
		if (ProximityTimer <= 0.f) { ProximityTimer = ProximityCheckInterval; CheckProximityDetection(); }
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

/* ═══════════ Rotation ═══════════ */

void ABaseAIController::FaceTargetYawOnly(AActor* Target, float DeltaTime)
{
	if (!Target || !AICharacter) return;
	const FVector Dir = (Target->GetActorLocation() - AICharacter->GetActorLocation()).GetSafeNormal2D();
	if (Dir.IsNearlyZero()) return;

	const FRotator TargetRot = FRotator(0.f, Dir.Rotation().Yaw, 0.f);
	const FRotator Current = AICharacter->GetActorRotation();
	const FRotator Smooth = FMath::RInterpTo(Current, TargetRot, DeltaTime, FaceTargetRotationSpeed);
	AICharacter->SetActorRotation(FRotator(0.f, Smooth.Yaw, 0.f));
}

/* ═══════════ Perception ═══════════ */

void ABaseAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!AICharacter || !Actor || AICharacter->IsDormant()) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		AICharacter->OnPerceiveTarget(Actor);
	}
	else if (AICharacter->GetCurrentTarget() == Actor)
	{
		const EAIState Cur = AICharacter->GetCurrentAIState();
		AICharacter->ClearTarget();
		if (Cur == EAIState::Chasing || Cur == EAIState::Attacking)
		{
			AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Alert);
			AICharacter->SetAIState(EAIState::Returning);
		}
		else if (Cur == EAIState::Fleeing)
			AICharacter->SetAIState(EAIState::Returning);
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
		AActor* Hit = Overlap.GetActor();
		if (!Hit || Cast<ABaseAICharacter>(Hit)) continue;
		if (AICharacter->OnlyDetectsPlayers())
		{
			APawn* P = Cast<APawn>(Hit);
			if (!P || !P->IsPlayerControlled()) continue;
		}
		AICharacter->OnPerceiveTarget(Hit);
		break;
	}
}

/* ═══════════ Helpers ═══════════ */

float ABaseAIController::GetEffectiveAttackRange() const
{
	if (UAICombatComponent* C = AICharacter->GetAICombat())
		if (C->GetAttacks().Num() > 0) return C->GetEffectiveAttackRange();
	return AttackRange;
}

bool ABaseAIController::CheckLeashAndTeleport()
{
	if (FVector::Dist(AICharacter->GetActorLocation(), SpawnOrigin) > AICharacter->GetLeashRange())
	{
		AICharacter->TeleportToSpawn();
		return true;
	}
	return false;
}

/* ═══════════ State Handlers ═══════════ */

void ABaseAIController::HandleIdleState(float DeltaTime)
{
	// Don't trigger idle anims if one is already playing
	UAIAnimationComponent* Anim = AICharacter->GetAIAnimation();
	if (Anim && Anim->IsPlayingIdleVariation()) return;

	IdleTimer += DeltaTime;
	if (IdleTimer >= NextIdleAnimTime)
	{
		IdleTimer = 0.f;
		NextIdleAnimTime = IdleAnimInterval + FMath::FRandRange(0.f, IdleAnimRandomDeviation);
		if (Anim) Anim->PlayRandomIdle();
	}

	if (AICharacter->GetAwarenessLevel() > EAIAwarenessLevel::Unaware)
		AICharacter->SetAwarenessLevel(EAIAwarenessLevel::Unaware);
}

void ABaseAIController::HandlePatrolState(float DeltaTime)
{
	UAIAnimationComponent* Anim = AICharacter->GetAIAnimation();

	// Wait for idle variation to finish before resuming patrol movement
	if (Anim && Anim->IsPlayingIdleVariation()) return;

	if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
	{
		if (MC->HasReachedDestination())
			HandleIdleState(DeltaTime);
		else
			IdleTimer = 0.f;
	}
}

void ABaseAIController::HandleChaseState(float DeltaTime)
{
	AActor* Target = AICharacter->GetCurrentTarget();
	if (!Target) { AICharacter->SetAIState(EAIState::Returning); return; }
	if (CheckLeashAndTeleport()) return;

	const float Dist = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());
	if (Dist <= GetEffectiveAttackRange())
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
	if (Dist > GetEffectiveAttackRange() * 1.3f) { AICharacter->SetAIState(EAIState::Chasing); return; }

	FaceTargetYawOnly(Target, DeltaTime);

	UAICombatComponent* Combat = AICharacter->GetAICombat();
	if (Combat && Combat->GetAttacks().Num() > 0)
	{
		if (!Combat->IsAttacking() && !Combat->IsCharging() && !Combat->IsStaggered())
			Combat->ExecuteRandomAttack(Dist);

		if (bStrafeInCombat && !Combat->IsAttacking())
		{
			StrafeTimer -= DeltaTime;
			if (StrafeTimer <= 0.f)
			{
				StrafeTimer = StrafeDirectionChangeInterval + FMath::FRandRange(-0.5f, 0.5f);
				StrafeDirection *= -1;
			}
			if (UAIMovementComponent* MC = AICharacter->GetAIMovement())
			{
				MC->SetDesiredSpeed(MC->PatrolSpeed * 0.8f);
				MC->MoveToLocation(AICharacter->GetActorLocation() + AICharacter->GetActorRightVector() * StrafeDirection * 200.f);
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
			if (MC->PatrolMode != EPatrolMode::Stationary) { AICharacter->SetAIState(EAIState::Patrolling); MC->StartPatrol(); }
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
		const float DistToThreat = FVector::Dist(AICharacter->GetActorLocation(), Target->GetActorLocation());

		// Safe distance reached — return home
		if (DistToThreat >= FleeSafeDistance)
		{
			AICharacter->ClearTarget();
			AICharacter->SetAIState(EAIState::Returning);
			return;
		}

		// Re-evaluate flee direction periodically (avoids running into walls)
		FleeReevalTimer -= DeltaTime;
		if (FleeReevalTimer <= 0.f || MC->HasReachedDestination())
		{
			FleeReevalTimer = FleeReevalInterval;
			MC->FleeFrom(Target);
		}
	}
	else
	{
		// Threat gone
		AICharacter->SetAIState(EAIState::Returning);
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
		DrawDebugLine(W,
			Loc + Fwd.RotateAngleAxis(FMath::RadiansToDegrees(A0), FVector::UpVector) * SightRadius,
			Loc + Fwd.RotateAngleAxis(FMath::RadiansToDegrees(A1), FVector::UpVector) * SightRadius,
			FColor::Green, false, -1.f, 0, 1.5f);
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
