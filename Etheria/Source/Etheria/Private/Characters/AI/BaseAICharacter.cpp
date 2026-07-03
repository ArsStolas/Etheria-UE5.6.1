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
#include "Components/Combat/CombatComponent.h"
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
	if (GetMesh()) MeshRelativeTransform = GetMesh()->GetRelativeTransform();

	if (UCharacterMovementComponent* MC = GetCharacterMovement())
		MC->RotationRate = FRotator(0.f, AIMovementComponent->MovementRotationRate, 0.f);

	if (bEnableAnimURO && GetMesh())
		GetMesh()->bEnableUpdateRateOptimizations = true;

	if (AICombatComponent && !ShouldEngageTargets())
		AICombatComponent->SetComponentTickEnabled(false);

	if (AIMovementComponent && PatrolSpline)
		AIMovementComponent->SetPatrolSpline(PatrolSpline);

	if (bShowDetectionDecal)
		SetDetectionDecalVisible(true);

	OnTakeAnyDamage.AddDynamic(this, &ABaseAICharacter::HandleTakeAnyDamage);

	CachedHealthComponent = FindComponentByClass<UHealthComponent>();
	if (CachedHealthComponent)
	{
		CachedHealthComponent->OnHealthChanged.AddDynamic(this, &ABaseAICharacter::HandleHealthChanged);

		if (!bCanReceiveDamage) CachedHealthComponent->SetInvulnerable(true);

		CachedHealthComponent->SetMinHealth(IsKillable() ? 0.f : 1.f);
	}

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			DormancyCheckTimerHandle, this,
			&ABaseAICharacter::UpdateDormancy,
			DormancyCheckInterval, true,
			0.5f);
	}

	if (bStartDormant)
		SetDormant(true);

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
			AIMovementComponent->StartPatrol();
	}

#if ENABLE_DRAW_DEBUG
	if (bShowDebugDormancy) DrawDebugDormancy();
#endif
}

float ABaseAICharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{

	if (!bCanReceiveDamage)
	{
		OnAIDamageBlocked.Broadcast(DamageCauser, DamageAmount);
		return 0.f;
	}

	if (CurrentState == EAIState::Dead) return 0.f;

	if (!IsKillable())
	{
		const APawn* CauserPawn = Cast<APawn>(DamageCauser);
		const bool bFromPlayer = (EventInstigator && EventInstigator->IsPlayerController())
			|| (CauserPawn && CauserPawn->IsPlayerControlled());
		if (bFromPlayer)
		{
			OnAIDamageBlocked.Broadcast(DamageCauser, DamageAmount);
			return 0.f;
		}
	}

	if (DamageEvent.GetTypeID() == FPointDamageEvent::ClassID)
		LastHitDirection = static_cast<const FPointDamageEvent&>(DamageEvent).ShotDirection;
	else if (DamageCauser)
		LastHitDirection = (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal();
	else
		LastHitDirection = FVector::ZeroVector;

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

void ABaseAICharacter::SetAIState(EAIState NewState)
{
	if (CurrentState == NewState) return;

	const EAIState OldState = CurrentState;
	CurrentState = NewState;
	OnAIStateChanged.Broadcast(OldState, NewState);

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

	if (AICombatComponent)
		AICombatComponent->SetComponentTickEnabled(ShouldEngageTargets());

	if (CachedHealthComponent)
		CachedHealthComponent->SetMinHealth(IsKillable() ? 0.f : 1.f);

	if (ABaseAIController* AIC = Cast<ABaseAIController>(GetController()))
		AIC->ReconcileHostility();
}

bool ABaseAICharacter::IsKillable() const
{
	switch (Killability)
	{
	case EAIKillability::Killable:   return true;
	case EAIKillability::Unkillable: return false;
	default: break;
	}

	if (bIsInteractable) return false;
	return ShouldEngageTargets();
}

void ABaseAICharacter::SetAwarenessLevel(EAIAwarenessLevel NewLevel)
{
	if (AwarenessLevel == NewLevel) return;
	const EAIAwarenessLevel Old = AwarenessLevel;
	AwarenessLevel = NewLevel;
	OnAwarenessChanged.Broadcast(Old, NewLevel);
}

bool ABaseAICharacter::ShouldEngageTargets() const
{
	return HostilityType == EAIHostilityType::Aggressive
		|| (HostilityType == EAIHostilityType::Neutral && bFightBackWhenAttacked);
}

bool ABaseAICharacter::CanReactToPerception() const
{

	if (HostilityType == EAIHostilityType::Aggressive) return true;
	return bCanFlee || bNoticeReactions;
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
		return MatchesFleeTag(Candidate);
	if (bOnlyDetectPlayers)
	{
		const APawn* P = Cast<APawn>(Candidate);
		if (!P || !P->IsPlayerControlled())
			return MatchesFleeTag(Candidate);
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

static UCombatComponent* ResolveTargetCombat(const AActor* Target,
	TWeakObjectPtr<UCombatComponent>& Cache, TWeakObjectPtr<const AActor>& CacheOwner)
{
	if (!Target) return nullptr;
	if (CacheOwner.Get() != Target || !Cache.IsValid())
	{
		CacheOwner = Target;
		Cache = Target->FindComponentByClass<UCombatComponent>();
	}
	return Cache.Get();
}

bool ABaseAICharacter::IsTargetDefending_Implementation(AActor* Target) const
{

	const UCombatComponent* CC = ResolveTargetCombat(Target, CachedTargetCombat, CachedTargetCombatOwner);
	return CC && CC->IsParryHeld();
}

bool ABaseAICharacter::IsTargetWindingUpAttack_Implementation(AActor* Target) const
{

	const UCombatComponent* CC = ResolveTargetCombat(Target, CachedTargetCombat, CachedTargetCombatOwner);
	if (!CC) return false;
	return (CC->IsAttackActive() && !CC->IsInAttackWindow()) || CC->IsMeleeCharging();
}

void ABaseAICharacter::AddThreat(AActor* Source, float Amount)
{

	if (!bUseThreatSystem || Amount <= 0.f || !IsValidTargetCandidate(Source) || IsTargetDeadOrInvalid(Source)) return;
	ThreatTable.FindOrAdd(Source) += Amount;
}

void ABaseAICharacter::EvaluateThreatSwitch()
{

	if (!bUseThreatSystem || !CurrentTarget) return;

	const ABaseAIController* AIC = Cast<ABaseAIController>(GetController());
	const float CurThreat = ThreatTable.FindRef(CurrentTarget.Get());
	AActor* Best = CurrentTarget;
	float BestThreat = CurThreat;
	for (const TPair<TWeakObjectPtr<AActor>, float>& Pair : ThreatTable)
	{
		AActor* A = Pair.Key.Get();
		if (!A || A == CurrentTarget) continue;

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
		if (!bFromDamage) return false;
		EndInteraction();
	}

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

	if (!bFromDamage && !bAlreadyEngaging && !bAlreadyFleeing && !bWantsFlee && CurrentState == EAIState::Returning)
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
		RallyNearbyAllies(Threat);
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
		AlarmNearbyAllies(Threat);
		return true;
	}

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
	if (!IsValidTargetCandidate(PerceivedActor)) return;
	ReactToThreat(PerceivedActor, false);
}

void ABaseAICharacter::OnReceiveDamage(AActor* DamageInstigator, float DamageAmount)
{
	if (CurrentState == EAIState::Dead || bIsDormant) return;

	OnAIDamaged.Broadcast(DamageInstigator);
	AddThreat(DamageInstigator, DamageAmount * ThreatPerDamage);

	if (DamageInstigator && DamageInstigator == CurrentTarget)
		if (ABaseAIController* AIC = Cast<ABaseAIController>(GetController()))
			AIC->NotifyTargetConfirmedByDamage(DamageInstigator);

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (AICombatComponent && HitCountDecayTime > 0.f && (Now - LastDamageTime) > HitCountDecayTime)
		AICombatComponent->CurrentHitCount = 0;
	LastDamageTime = Now;

	const float MaxHP = CachedHealthComponent ? CachedHealthComponent->GetMaxHealth() : 0.f;
	const float DamageFraction = (MaxHP > 0.f) ? DamageAmount / MaxHP : 0.f;

	bool bStaggering = false;
	if (ShouldEngageTargets() && AICombatComponent && !AICombatComponent->IsStaggerImmune())
	{
		AICombatComponent->CurrentHitCount++;
		if (AICombatComponent->CurrentHitCount >= AICombatComponent->StaggerThreshold)
			bStaggering = true;

		if (HeavyHitStaggerFraction > 0.f && DamageFraction >= HeavyHitStaggerFraction)
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

		const bool bTooLightToFlinch = (LightHitFlinchFraction > 0.f && MaxHP > 0.f && DamageFraction < LightHitFlinchFraction);
		if (!bUnderHyperArmor && !bTooLightToFlinch)
			if (AIAnimationComponent->PlayDirectionalHitReaction(LastHitDirection))
				if (AIMovementComponent) AIMovementComponent->StopMovement();
	}

	if (!DamageInstigator) return;

	if (!bStaggering && bCanFlee && FleeHealthThreshold > 0.f && LastHealthFraction <= FleeHealthThreshold
		&& CurrentState != EAIState::Fleeing)
	{
		SetAwarenessLevel(EAIAwarenessLevel::Alert);
		SetTarget(DamageInstigator);
		SetAIState(EAIState::Fleeing);
		if (GetWorld()) MoraleBreakUntil = GetWorld()->GetTimeSeconds() + MoraleBreakCooldown;
		if (AIAnimationComponent) AIAnimationComponent->PlayStartle();
		if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->FleeFrom(DamageInstigator); }
		return;
	}

	if (bStaggering)
		SetTarget(DamageInstigator);
	else
		ReactToThreat(DamageInstigator, true);

	EvaluateThreatSwitch();
}

void ABaseAICharacter::Interact_Implementation(AActor* Target)
{
	BeginInteraction(Target);
}

void ABaseAICharacter::CanReceiveTrace_Implementation()
{

}

void ABaseAICharacter::BeginInteraction(AActor* Interactor)
{
	if (!bIsInteractable || CurrentState == EAIState::Dead || bIsDormant) return;
	if (CurrentState == EAIState::Interacting) return;

	PreInteractionState = CurrentState;
	InteractionPartner = Interactor;

	if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->StopMovement(); }
	SetAIState(EAIState::Interacting);
	if (AIAnimationComponent) AIAnimationComponent->PlayInteraction();

	OnInteractionStarted.Broadcast(Interactor, NPCRole, DialogueID);
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

	if (!bAlarmsNearbyAllies || AlarmRadius <= 0.f || !Threat || bSuppressAlarmBroadcast) return;
	UWorld* W = GetWorld();
	if (!W) return;

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
		O->bSuppressAlarmBroadcast = true;
		O->OnPerceiveTarget(Threat);
		O->bSuppressAlarmBroadcast = false;
	}
}

void ABaseAICharacter::RallyNearbyAllies(AActor* Threat)
{

	if (!bCallForHelpOnEngage || CombatAlertRadius <= 0.f || !Threat || bSuppressRallyBroadcast) return;
	if (HostilityType != EAIHostilityType::Aggressive) return;
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
		if (!O->IsValidTargetCandidate(Threat)) continue;

		const EAIState S = O->GetCurrentAIState();
		if (S == EAIState::Chasing || S == EAIState::Attacking || S == EAIState::Fleeing || S == EAIState::Dead) continue;

		O->bSuppressRallyBroadcast = true;
		if (bRallyEngagesDirectly)
			O->OnPerceiveTarget(Threat);
		else if (ABaseAIController* AIC = Cast<ABaseAIController>(O->GetController()))
			AIC->InvestigateThreat(Threat);
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

	if (HostilityType == EAIHostilityType::Aggressive)
	{
		if (ABaseAIController* AIC = Cast<ABaseAIController>(GetController()))
		{
			AIC->InvestigateThreat(Threat);
			return;
		}
	}

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
	Reg->GetPackMembers(PackID, All);
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
				AIC->Investigate(Location);
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
			if (!Leader || O->GetUniqueID() < Leader->GetUniqueID()) Leader = O;
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

		if (!bHasFollowOffset || FollowReevalTimer <= 0.f)
		{
			const FVector2D R = FMath::RandPointInCircle(PackSpreadRadius);
			CachedFollowOffset = FVector(R.X, R.Y, 0.f);
			bHasFollowOffset = true;
			FollowReevalTimer = 1.5f;
		}

		if (AIAnimationComponent && AIAnimationComponent->IsPlayingIdleVariation())
			AIAnimationComponent->StopCurrentAction();

		const FVector DirToLeader = (Leader->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		const FVector Goal = Leader->GetActorLocation() - DirToLeader * PackFollowDistance + CachedFollowOffset;

		const float SpeedScale = FMath::GetMappedRangeValueClamped(
			FVector2D(FollowThreshold, FollowThreshold * 3.f), FVector2D(1.f, 1.4f), FMath::Sqrt(DistSq));
		AIMovementComponent->SetDesiredSpeed(AIMovementComponent->PatrolSpeed * SpeedScale);
		AIMovementComponent->MoveToLocation(Goal);
	}
	else
	{
		bHasFollowOffset = false;
	}
}

void ABaseAICharacter::TeleportToSpawn(FVector OverrideLocation)
{
	ClearTarget();
	if (AIMovementComponent) { AIMovementComponent->StopPatrol(); AIMovementComponent->StopMovement(); }
	if (AICombatComponent) { AICombatComponent->InterruptAttack(); AICombatComponent->ExitCombat(); }

	const FVector Dest = OverrideLocation.IsNearlyZero() ? SpawnLocation : OverrideLocation;
	SetActorHiddenInGame(true);
	SetActorLocation(Dest);
	SetActorRotation(SpawnRotation);
	SetActorHiddenInGame(false);

	SetAwarenessLevel(EAIAwarenessLevel::Unaware);
	SetAIState(EAIState::Idle);

	if (ABaseAIController* AIC = Cast<ABaseAIController>(GetController()))
		if (AIC->ReacquireOnReturn()) return;

	if (AIMovementComponent && AIMovementComponent->PatrolMode != EPatrolMode::Stationary)
	{
		SetAIState(EAIState::Patrolling);
		AIMovementComponent->StartPatrol();
	}
}

void ABaseAICharacter::Die()
{
	if (CurrentState == EAIState::Dead) return;

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

	if (GetCharacterMovement()) GetCharacterMovement()->DisableMovement();
	if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (AController* AC = GetController()) AC->SetActorTickEnabled(false);

	UAnimMontage* DeathMontage = nullptr;
	if (AIAnimationComponent) DeathMontage = AIAnimationComponent->PlayDeath();

	if (!DeathMontage && bRagdollFallbackDeath && GetMesh())
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetMesh()->SetSimulatePhysics(true);
		if (!LastHitDirection.IsNearlyZero())
			GetMesh()->AddImpulse(LastHitDirection.GetSafeNormal() * DeathRagdollImpulse, NAME_None, true);
	}

	OnAIDied.Broadcast();

	const float MontageLength = (DeathMontage ? DeathMontage->GetPlayLength() : 1.5f);
	const float TriggerAt = FMath::Max(0.f, MontageLength - DeathVFXTimeBeforeEnd);

	if (TriggerAt <= KINDA_SMALL_NUMBER)
	{

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

	if (DeathVFX)
	{
		USkeletalMeshComponent* MMesh = GetMesh();
		if (DeathVFXSocket != NAME_None && MMesh && MMesh->DoesSocketExist(DeathVFXSocket))
		{
			SpawnedDeathVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
				DeathVFX, MMesh, DeathVFXSocket,
				FVector::ZeroVector, FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget, true);
		}
		else
		{
			SpawnedDeathVFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(), DeathVFX, GetActorLocation(), GetActorRotation());
		}
	}

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

	if (bDissolveParamFound && DeathDissolveParameterName != NAME_None)
	{
		for (UMaterialInstanceDynamic* MID : CachedDynamicMaterials)
			if (MID) MID->SetScalarParameterValue(DeathDissolveParameterName, Alpha);
	}
	else if (bUseScaleFallback)
	{

		const float Scale = FMath::Lerp(1.f, 0.01f, Alpha);
		SetActorScale3D(SpawnScale * Scale);
	}

	if (DeathSinkDistance > 0.f)
	{
		FVector NewLoc = DeathStartLocation;
		NewLoc.Z -= Alpha * DeathSinkDistance;
		SetActorLocation(NewLoc, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (Alpha >= 1.f) FinishDeathFade();
}

void ABaseAICharacter::FinishDeathFade()
{
	bIsDeathFading = false;
	SetActorHiddenInGame(true);

	if (SpawnedDeathVFX)
		SpawnedDeathVFX->Deactivate();

	OnAIDeathFadeCompleted.Broadcast();

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

	if (USkeletalMeshComponent* M = GetMesh(); M && M->IsSimulatingPhysics())
	{
		M->SetSimulatePhysics(false);
		M->SetCollisionProfileName(TEXT("CharacterMesh"));
		M->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		M->SetRelativeTransform(MeshRelativeTransform);
	}

	if (UWorld* W = GetWorld())
		W->GetTimerManager().ClearTimer(DeathVFXTimerHandle);

	if (SpawnedDeathVFX)
	{
		SpawnedDeathVFX->DestroyComponent();
		SpawnedDeathVFX = nullptr;
	}

	if (DeathDissolveParameterName != NAME_None)
	{
		for (UMaterialInstanceDynamic* MID : CachedDynamicMaterials)
			if (MID) MID->SetScalarParameterValue(DeathDissolveParameterName, 0.f);
	}

	SetActorScale3D(SpawnScale);

	SetActorHiddenInGame(false);
}

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

void ABaseAICharacter::SetDormant(bool bNewDormant)
{
	if (bIsDormant == bNewDormant) return;
	bIsDormant = bNewDormant;

	SetActorHiddenInGame(bNewDormant);
	SetActorTickEnabled(!bNewDormant);

	if (bNewDormant && AICombatComponent) AICombatComponent->EndHitStop();

	if (AIMovementComponent)  AIMovementComponent->SetComponentTickEnabled(!bNewDormant);
	if (AIAnimationComponent) AIAnimationComponent->SetComponentTickEnabled(!bNewDormant);
	if (AICombatComponent)    AICombatComponent->SetComponentTickEnabled(!bNewDormant);
	if (GetCharacterMovement()) GetCharacterMovement()->SetComponentTickEnabled(!bNewDormant);

	if (AController* AC = GetController()) AC->SetActorTickEnabled(!bNewDormant);

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		Capsule->SetCollisionEnabled(bNewDormant ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);

	OnAIDormancyChanged.Broadcast();
}

void ABaseAICharacter::UpdateDormancy()
{

	if (CurrentState == EAIState::Dead || CurrentState == EAIState::Interacting) return;

	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn()) return;

	const float DistSq = FVector::DistSquared(GetActorLocation(), PC->GetPawn()->GetActorLocation());

	const float SleepSq = DormantDistance * DormantDistance;
	const float WakeSq  = (DormantDistance * 0.8f) * (DormantDistance * 0.8f);
	if (!bIsDormant && DistSq > SleepSq)
		SetDormant(true);
	else if (bIsDormant && DistSq < WakeSq)
		SetDormant(false);
}

void ABaseAICharacter::SetDetectionDecalVisible(bool bVisible)
{

	if (ProximityDecal && ProximityDecalMaterial) { ProximityDecal->SetDecalMaterial(ProximityDecalMaterial); ProximityDecal->SetVisibility(bVisible); }
	if (SightDecal) SightDecal->SetVisibility(false);
}

void ABaseAICharacter::SetDetectionDecalRadius(float Radius)
{

	if (ProximityDecal) ProximityDecal->DecalSize = FVector(400.f, Radius, Radius);
}

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
