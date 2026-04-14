/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAICharacter - Source"
 * Notes: Connects to HealthComponent for hit reactions. Fight-back for neutrals.
 *        Pack follow, respawn, dormancy, detection decals.
 */

#include "Characters/AI/BaseAICharacter.h"

#include "Characters/AI/Movements/AIMovementComponent.h"
#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Characters/AI/Combat/AICombatComponent.h"
#include "Characters/AI/Controller/BaseAIController.h"
#include "Components/SplineComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

ABaseAICharacter::ABaseAICharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AIMovementComponent = CreateDefaultSubobject<UAIMovementComponent>(TEXT("AIMovementComponent"));
	AIAnimationComponent = CreateDefaultSubobject<UAIAnimationComponent>(TEXT("AIAnimationComponent"));
	AICombatComponent = CreateDefaultSubobject<UAICombatComponent>(TEXT("AICombatComponent"));

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

	// Prevent the mesh from pitching toward the target
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

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

	// Apply configured rotation rate
	if (UCharacterMovementComponent* MC = GetCharacterMovement())
	{
		MC->RotationRate = FRotator(0.f, AIMovementComponent->MovementRotationRate, 0.f);
	}

	if (AIMovementComponent && PatrolSpline)
		AIMovementComponent->SetPatrolSpline(PatrolSpline);

	if (bShowDetectionDecal)
		SetDetectionDecalVisible(true);

	// Bind to the native damage system so hit reactions trigger automatically
	OnTakeAnyDamage.AddDynamic(this, &ABaseAICharacter::HandleTakeAnyDamage);
}

void ABaseAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DormancyTimer -= DeltaTime;
	if (DormancyTimer <= 0.f)
	{
		DormancyTimer = DormancyCheckInterval;
		UpdateDormancy();
	}

	if (bIsDormant) return;

	if (PackID != NAME_None && !IsPackLeader() && CurrentState == EAIState::Patrolling)
		UpdatePackFollow(DeltaTime);
}

/* ═══════════ Native Damage Hook ═══════════ */

void ABaseAICharacter::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	OnReceiveDamage(DamageCauser, Damage);
}

/* ═══════════ State ═══════════ */

void ABaseAICharacter::SetAIState(EAIState NewState)
{
	if (CurrentState == NewState) return;

	const EAIState OldState = CurrentState;
	CurrentState = NewState;
	OnAIStateChanged.Broadcast(OldState, NewState);

	// Auto-manage combat component
	if (AICombatComponent)
	{
		const bool bNowInCombat = (NewState == EAIState::Attacking || NewState == EAIState::Chasing);
		const bool bWasInCombat = (OldState == EAIState::Attacking || OldState == EAIState::Chasing);

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

void ABaseAICharacter::SetAwarenessLevel(EAIAwarenessLevel NewLevel)
{
	if (AwarenessLevel == NewLevel) return;
	const EAIAwarenessLevel Old = AwarenessLevel;
	AwarenessLevel = NewLevel;
	OnAwarenessChanged.Broadcast(Old, NewLevel);
}

/* ═══════════ Perception ═══════════ */

void ABaseAICharacter::OnPerceiveTarget(AActor* PerceivedActor)
{
	if (!PerceivedActor || CurrentState == EAIState::Dead || bIsDormant) return;

	if (bOnlyDetectPlayers)
	{
		APawn* P = Cast<APawn>(PerceivedActor);
		if (!P || !P->IsPlayerControlled()) return;
	}

	switch (HostilityType)
	{
	case EAIHostilityType::Passive:
		if (bCanFlee && CurrentState != EAIState::Fleeing)
		{
			bool bShouldFlee = FleeFromTags.Num() == 0;
			for (const FName& Tag : FleeFromTags)
				if (PerceivedActor->ActorHasTag(Tag)) { bShouldFlee = true; break; }

			if (bShouldFlee)
			{
				SetAwarenessLevel(EAIAwarenessLevel::Alert);
				SetTarget(PerceivedActor);
				SetAIState(EAIState::Fleeing);
				if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->FleeFrom(PerceivedActor); }
				if (PackID != NAME_None) AlertPack(PerceivedActor);
			}
		}
		break;

	case EAIHostilityType::Neutral:
		if (bCanFlee && CurrentState != EAIState::Fleeing && CurrentState != EAIState::Chasing && CurrentState != EAIState::Attacking)
		{
			bool bShouldFlee = FleeFromTags.Num() == 0;
			for (const FName& Tag : FleeFromTags)
				if (PerceivedActor->ActorHasTag(Tag)) { bShouldFlee = true; break; }

			if (bShouldFlee)
			{
				SetAwarenessLevel(EAIAwarenessLevel::Alert);
				SetTarget(PerceivedActor);
				SetAIState(EAIState::Fleeing);
				if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->FleeFrom(PerceivedActor); }
				if (PackID != NAME_None) AlertPack(PerceivedActor);
			}
		}
		break;

	case EAIHostilityType::Aggressive:
		if (CurrentState != EAIState::Chasing && CurrentState != EAIState::Attacking)
		{
			SetAwarenessLevel(EAIAwarenessLevel::InCombat);
			SetTarget(PerceivedActor);
			SetAIState(EAIState::Chasing);
			if (AIMovementComponent)
			{
				AIMovementComponent->StopPatrol();
				AIMovementComponent->SetDesiredSpeed(AIMovementComponent->ChaseSpeed);
				AIMovementComponent->MoveToLocation(PerceivedActor->GetActorLocation());
			}
			if (PackID != NAME_None) AlertPack(PerceivedActor);
		}
		break;
	}
}

void ABaseAICharacter::OnReceiveDamage(AActor* DamageInstigator, float DamageAmount)
{
	if (CurrentState == EAIState::Dead || bIsDormant) return;

	OnAIDamaged.Broadcast(DamageInstigator);

	// ── Hit reaction animation ──
	if (AIAnimationComponent)
	{
		AIAnimationComponent->PlayHitReaction();
	}

	// ── Stagger tracking ──
	if (AICombatComponent)
	{
		AICombatComponent->CurrentHitCount++;
		if (AICombatComponent->CurrentHitCount >= AICombatComponent->StaggerThreshold)
			AICombatComponent->ApplyStagger(1.f);
	}

	// ── Neutral: fight back when attacked (if configured) ──
	if (HostilityType == EAIHostilityType::Neutral && DamageInstigator)
	{
		if (bFightBackWhenAttacked)
		{
			// Stop fleeing and engage
			SetAwarenessLevel(EAIAwarenessLevel::InCombat);
			SetTarget(DamageInstigator);
			SetAIState(EAIState::Chasing);
			if (AIMovementComponent)
			{
				AIMovementComponent->StopPatrol();
				AIMovementComponent->SetDesiredSpeed(AIMovementComponent->ChaseSpeed);
				AIMovementComponent->MoveToLocation(DamageInstigator->GetActorLocation());
			}
			if (PackID != NAME_None) AlertPack(DamageInstigator);
		}
		else if (bCanFlee && CurrentState != EAIState::Fleeing)
		{
			// Keep fleeing
			SetTarget(DamageInstigator);
			SetAIState(EAIState::Fleeing);
			if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->FleeFrom(DamageInstigator); }
		}
	}

	// ── Passive: flee from attacker ──
	if (HostilityType == EAIHostilityType::Passive && bCanFlee && DamageInstigator)
	{
		SetTarget(DamageInstigator);
		SetAIState(EAIState::Fleeing);
		if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->FleeFrom(DamageInstigator); }
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

void ABaseAICharacter::OnPackAlert(ABaseAICharacter* Alerter, AActor* Threat)
{
	if (!Threat || CurrentState == EAIState::Dead || bIsDormant) return;
	if (CurrentState == EAIState::Chasing || CurrentState == EAIState::Attacking || CurrentState == EAIState::Fleeing) return;
	OnPackAlerted.Broadcast(Alerter, Threat);
	OnPerceiveTarget(Threat);
}

TArray<ABaseAICharacter*> ABaseAICharacter::GetPackMembers() const
{
	TArray<ABaseAICharacter*> Members;
	if (PackID == NAME_None) return Members;
	const FVector MyLoc = GetActorLocation();
	for (TActorIterator<ABaseAICharacter> It(GetWorld()); It; ++It)
	{
		ABaseAICharacter* O = *It;
		if (O == this || O->GetPackID() != PackID || O->IsDead()) continue;
		if (FVector::Dist(MyLoc, O->GetActorLocation()) <= PackAlertRadius) Members.Add(O);
	}
	return Members;
}

ABaseAICharacter* ABaseAICharacter::GetPackLeader() const
{
	if (PackID == NAME_None) return nullptr;
	ABaseAICharacter* Leader = nullptr;
	for (TActorIterator<ABaseAICharacter> It(GetWorld()); It; ++It)
	{
		ABaseAICharacter* O = *It;
		if (O->GetPackID() != PackID || O->IsDead()) continue;
		if (!Leader || O->GetUniqueID() < Leader->GetUniqueID()) Leader = O;
	}
	return Leader;
}

bool ABaseAICharacter::IsPackLeader() const { return GetPackLeader() == this; }

void ABaseAICharacter::UpdatePackFollow(float DeltaTime)
{
	ABaseAICharacter* Leader = GetPackLeader();
	if (!Leader || Leader == this) return;

	const float Dist = FVector::Dist(GetActorLocation(), Leader->GetActorLocation());
	if (Dist > PackFollowDistance + PackSpreadRadius && AIMovementComponent)
	{
		const FVector DirToLeader = (Leader->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		const FVector RandOffset = FMath::VRand().GetSafeNormal2D() * FMath::FRandRange(0.f, PackSpreadRadius);
		const FVector Target = Leader->GetActorLocation() - DirToLeader * PackFollowDistance + FVector(RandOffset.X, RandOffset.Y, 0.f);
		AIMovementComponent->SetDesiredSpeed(AIMovementComponent->PatrolSpeed * 1.2f);
		AIMovementComponent->MoveToLocation(Target);
	}
}

/* ═══════════ Leash ═══════════ */

void ABaseAICharacter::TeleportToSpawn()
{
	ClearTarget();
	if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->StopMovement(); }
	if (AICombatComponent) { AICombatComponent->InterruptAttack(); AICombatComponent->ExitCombat(); }

	SetActorHiddenInGame(true);
	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnRotation);
	SetActorHiddenInGame(false);

	SetAwarenessLevel(EAIAwarenessLevel::Unaware);
	CurrentState = EAIState::Idle;

	if (AIMovementComponent && AIMovementComponent->PatrolMode != EPatrolMode::Stationary)
	{
		SetAIState(EAIState::Patrolling);
		AIMovementComponent->StartPatrol();
	}
}

/* ═══════════ Respawn ═══════════ */

void ABaseAICharacter::Die()
{
	if (CurrentState == EAIState::Dead) return;

	const EAIState OldState = CurrentState;
	CurrentState = EAIState::Dead;
	OnAIStateChanged.Broadcast(OldState, EAIState::Dead);

	ClearTarget();
	if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->StopMovement(); }
	if (AICombatComponent) AICombatComponent->ExitCombat();
	if (AIAnimationComponent) AIAnimationComponent->PlayDeath();

	if (GetCharacterMovement()) GetCharacterMovement()->DisableMovement();
	if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OnAIDied.Broadcast();

	if (RespawnCondition == EAIRespawnCondition::OnTimer)
	{
		GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this,
			&ABaseAICharacter::HandleRespawnTimer, RespawnTimerDuration, false);
	}
}

void ABaseAICharacter::Respawn()
{
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle);

	SetActorHiddenInGame(false);
	SetActorLocation(SpawnLocation);
	SetActorRotation(SpawnRotation);

	if (GetCharacterMovement()) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	SetAwarenessLevel(EAIAwarenessLevel::Unaware);
	CurrentState = EAIState::Idle;

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
	if (AIMovementComponent) AIMovementComponent->SetComponentTickEnabled(!bNewDormant);
	if (AIAnimationComponent) AIAnimationComponent->SetComponentTickEnabled(!bNewDormant);
	if (AICombatComponent) AICombatComponent->SetComponentTickEnabled(!bNewDormant);
	if (GetCharacterMovement()) GetCharacterMovement()->SetComponentTickEnabled(!bNewDormant);
	OnAIDormancyChanged.Broadcast();
}

void ABaseAICharacter::UpdateDormancy()
{
	if (CurrentState == EAIState::Dead) return;
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn()) return;
	const float Dist = FVector::Dist(GetActorLocation(), PC->GetPawn()->GetActorLocation());
	if (!bIsDormant && Dist > DormantDistance) SetDormant(true);
	else if (bIsDormant && Dist < DormantDistance * 0.8f) SetDormant(false);
}

/* ═══════════ Detection Decal ═══════════ */

void ABaseAICharacter::SetDetectionDecalVisible(bool bVisible)
{
	if (ProximityDecal && ProximityDecalMaterial) { ProximityDecal->SetDecalMaterial(ProximityDecalMaterial); ProximityDecal->SetVisibility(bVisible); }
	if (SightDecal && SightDecalMaterial) { SightDecal->SetDecalMaterial(SightDecalMaterial); SightDecal->SetVisibility(bVisible); }
}
