/**
 * Etheria's End Project, 2025
 * Created by: Mato
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
#include "Components/Characters/HealthComponent.h"
#include "Components/SplineComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
}

void ABaseAICharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

	if (PackID != NAME_None && !IsPackLeader() && CurrentState == EAIState::Patrolling)
		UpdatePackFollow(DeltaTime);

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

bool ABaseAICharacter::ReactToThreat(AActor* Threat, bool bFromDamage)
{
	if (!Threat || CurrentState == EAIState::Dead || bIsDormant) return false;

	const bool bAlreadyEngaging = (CurrentState == EAIState::Chasing || CurrentState == EAIState::Attacking);
	const bool bAlreadyFleeing  = (CurrentState == EAIState::Fleeing);

	const bool bWantsFight =
		(HostilityType == EAIHostilityType::Aggressive) ||
		(bFromDamage && HostilityType == EAIHostilityType::Neutral && bFightBackWhenAttacked);

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
		return true;
	}

	if (bWantsFlee)
	{
		if (bAlreadyFleeing) { SetTarget(Threat); return false; }
		SetAwarenessLevel(EAIAwarenessLevel::Alert);
		SetTarget(Threat);
		SetAIState(EAIState::Fleeing);
		if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->FleeFrom(Threat); }
		if (PackID != NAME_None) AlertPack(Threat);
		return true;
	}

	return false;
}

void ABaseAICharacter::OnPerceiveTarget(AActor* PerceivedActor)
{
	if (!PerceivedActor || CurrentState == EAIState::Dead || bIsDormant) return;

	if (bOnlyDetectPlayers)
	{
		APawn* P = Cast<APawn>(PerceivedActor);
		if (!P || !P->IsPlayerControlled()) return;
	}

	ReactToThreat(PerceivedActor, false);
}

void ABaseAICharacter::OnReceiveDamage(AActor* DamageInstigator, float DamageAmount)
{
	if (CurrentState == EAIState::Dead || bIsDormant) return;

	OnAIDamaged.Broadcast(DamageInstigator);

	bool bStaggering = false;
	if (AICombatComponent)
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
			AIAnimationComponent->PlayHitReaction();
	}

	if (!DamageInstigator) return;
	if (bStaggering)
		SetTarget(DamageInstigator);
	else
		ReactToThreat(DamageInstigator, true);
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
	const float RadiusSq = PackAlertRadius * PackAlertRadius;
	for (TActorIterator<ABaseAICharacter> It(GetWorld()); It; ++It)
	{
		ABaseAICharacter* O = *It;
		if (O == this || O->GetPackID() != PackID || O->IsDead()) continue;
		if (FVector::DistSquared(MyLoc, O->GetActorLocation()) <= RadiusSq) Members.Add(O);
	}
	return Members;
}

ABaseAICharacter* ABaseAICharacter::GetPackLeader() const
{
	if (PackID == NAME_None) return nullptr;

	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	if (CachedPackLeader.IsValid() && !CachedPackLeader->IsDead()
		&& CachedPackLeader->GetPackID() == PackID
		&& (Now - PackLeaderCacheStamp) < PACK_LEADER_CACHE_TTL)
	{
		return CachedPackLeader.Get();
	}

	ABaseAICharacter* Leader = nullptr;
	for (TActorIterator<ABaseAICharacter> It(GetWorld()); It; ++It)
	{
		ABaseAICharacter* O = *It;
		if (O->GetPackID() != PackID || O->IsDead()) continue;
		if (!Leader || O->GetUniqueID() < Leader->GetUniqueID()) Leader = O;
	}

	CachedPackLeader = Leader;
	PackLeaderCacheStamp = Now;
	return Leader;
}

bool ABaseAICharacter::IsPackLeader() const { return GetPackLeader() == this; }

void ABaseAICharacter::UpdatePackFollow(float DeltaTime)
{
	ABaseAICharacter* Leader = GetPackLeader();
	if (!Leader || Leader == this) return;

	const float FollowThreshold = PackFollowDistance + PackSpreadRadius;
	if (FVector::DistSquared(GetActorLocation(), Leader->GetActorLocation()) > FollowThreshold * FollowThreshold && AIMovementComponent)
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
	SetAIState(EAIState::Idle);

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
	// Dead AIs don't need dormancy management — they're either fading or already hidden
	if (CurrentState == EAIState::Dead) return;

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