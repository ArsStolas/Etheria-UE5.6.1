/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemBossComponent - Source"
 * Notes: Timer-driven attack brain. A cheap BrainInterval timer handles activation / phases / air-zones /
 *        facing / idle->begin, and one-shot timers drive Windup->Active->Recovery. Per-frame ticking is
 *        ENABLED ONLY during a live moving hazard (sweep/laser/ring/fissures/avalanche) so its VFX stays
 *        frame-smooth; the boss never ticks while idle/winding/recovering/cooling. C++ owns selection +
 *        geometry + AoE damage; every step broadcasts a BP dispatcher (anims/VFX/SFX/air-zone column).
 */

#include "Characters/AI/Boss/GolemBossComponent.h"

#include "Characters/AI/BaseAICharacter.h"
#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Characters/AI/Combat/AICombatComponent.h"
#include "Characters/AI/Boss/GolemFallingRock.h"
#include "Characters/AI/Boss/GolemCrystal.h"
#include "Characters/AI/Boss/GolemBossCharacter.h"
#include "Components/Characters/HealthComponent.h"
#include "Components/Combat/CombatComponent.h"
#include "UI/GolemBossBarWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/StaticMesh.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Engine/HitResult.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

namespace
{

	void ShatterCrystalActor(AActor* A)
	{
		if (AGolemCrystal* Cr = Cast<AGolemCrystal>(A)) Cr->Shatter();
		else if (IsValid(A)) A->Destroy();
	}

	void ApplyBeamParams(UNiagaraComponent* C, const FGolemBeamSegment& B)
	{
		if (!C) return;
		C->SetWorldLocation(B.Origin);
		C->SetWorldRotation((B.End - B.Origin).Rotation());
		const float Len = (B.End - B.Origin).Size();

		static const TCHAR* StartNames[] = { TEXT("BeamStart"), TEXT("Start"), TEXT("StartPosition"), TEXT("StartLocation"), TEXT("Source"), TEXT("Origin") };
		static const TCHAR* EndNames[]   = { TEXT("BeamEnd"),   TEXT("End"),   TEXT("EndPosition"),   TEXT("EndLocation"),   TEXT("Target"), TEXT("Destination") };
		for (const TCHAR* N : StartNames) { C->SetVariableVec3(FName(N), B.Origin); C->SetVariablePosition(FName(N), B.Origin); }
		for (const TCHAR* N : EndNames)   { C->SetVariableVec3(FName(N), B.End);    C->SetVariablePosition(FName(N), B.End); }
		C->SetVariableFloat(FName(TEXT("BeamWidth")), B.Width);
		C->SetVariableFloat(FName(TEXT("Width")), B.Width);
		C->SetVariableFloat(FName(TEXT("BeamLength")), Len);
		C->SetVariableFloat(FName(TEXT("Length")), Len);
	}

	float SegDist2D(const FVector& P, const FVector& A, const FVector& B)
	{
		const FVector p(P.X, P.Y, 0.f), a(A.X, A.Y, 0.f), b(B.X, B.Y, 0.f);
		return FMath::PointDistToSegment(p, a, b);
	}

	FVector YawRotate(const FVector& Dir, float YawDeg)
	{
		return FRotator(0.f, YawDeg, 0.f).RotateVector(Dir);
	}

#if ENABLE_DRAW_DEBUG
	void DrawFlatCircle(const UWorld* W, const FVector& C, float R, const FColor& Col, float Life, float Thick)
	{
		if (!W || R <= 0.f) return;
		DrawDebugCircle(W, C + FVector(0, 0, 6.f), R, 48, Col, false, Life, 0, Thick, FVector(1, 0, 0), FVector(0, 1, 0), false);
	}
#endif
}

UGolemBossComponent::UGolemBossComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	ArenaCrystalActorClass = AGolemCrystal::StaticClass();
	BigCrystalActorClass = AGolemCrystal::StaticClass();
}

void UGolemBossComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ABaseAICharacter>(GetOwner());
	if (OwnerCharacter)
	{
		OwnerHealth = OwnerCharacter->FindComponentByClass<UHealthComponent>();
		OwnerCharacter->OnAIDied.AddDynamic(this, &UGolemBossComponent::HandleOwnerDied);
		OwnerCharacter->OnAIDamaged.AddDynamic(this, &UGolemBossComponent::HandleOwnerDamaged);
		if (OwnerHealth) OwnerHealth->OnHealthChanged.AddDynamic(this, &UGolemBossComponent::HandleBossHealthChanged);
	}

	OnGolemActivated.AddDynamic(this, &UGolemBossComponent::ShowBossUIAndMusic);
	OnGolemDefeated.AddDynamic(this, &UGolemBossComponent::HideBossUIAndMusic);

	AttackReadyTimes.Init(0.f, Attacks.Num());

	for (FGolemWeakPoint& WP : WeakPoints)
	{
		WP.CurrentHealth = WP.Health;
		WP.bBroken = false;
		WP.bVulnerable = false;
		WP.CrystalHits = 0;
	}

	if (UWorld* W = GetWorld())
		W->GetTimerManager().SetTimer(BrainTimerHandle, this, &UGolemBossComponent::BrainTick,
			FMath::Max(0.02f, BrainInterval), true, 0.1f);
}

void UGolemBossComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(BrainTimerHandle);
		W->GetTimerManager().ClearTimer(WindupTimerHandle);
		W->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		W->GetTimerManager().ClearTimer(ToppleTimerHandle);
		W->GetTimerManager().ClearTimer(ExposeLingerTimerHandle);
		W->GetTimerManager().ClearTimer(RockReleaseTimerHandle);
	}
	if (OwnerCharacter)
	{
		OwnerCharacter->OnAIDied.RemoveDynamic(this, &UGolemBossComponent::HandleOwnerDied);
		OwnerCharacter->OnAIDamaged.RemoveDynamic(this, &UGolemBossComponent::HandleOwnerDamaged);
	}
	if (OwnerHealth) OwnerHealth->OnHealthChanged.RemoveDynamic(this, &UGolemBossComponent::HandleBossHealthChanged);
	HideBossUIAndMusic();
	ClearBeamVFX();
	ClearAirZones();
	EndBigCrystal();
	ClearArenaCrystals();
	if (HeldRock) { HeldRock->Destroy(); HeldRock = nullptr; }
	Super::EndPlay(EndPlayReason);
}

void UGolemBossComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (OwnerCharacter && State == EGolemAttackState::Active)
		TickActive(DeltaTime);
}

void UGolemBossComponent::BrainTick()
{
	if (!OwnerCharacter) return;

	if (AGolemBossCharacter* Golem = Cast<AGolemBossCharacter>(OwnerCharacter))
		Golem->EnforceImmovable();

	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;

	const float Elapsed = (LastBrainTime >= 0.f) ? FMath::Max(Now - LastBrainTime, 0.f) : BrainInterval;
	LastBrainTime = Now;

	TickAirZones(Elapsed);
	if (bActivated) TickContactRepulsion();

	if (!bActivated)
	{
		if (bAutoActivateOnTarget)
		{
			AActor* T = OwnerCharacter->GetCurrentTarget();
			if (T && !OwnerCharacter->IsTargetDeadOrInvalid(T)) ActivateBoss();
		}
	}
	else if (!OwnerCharacter->IsDead())
	{
		AActor* WatchT = OwnerCharacter->GetCurrentTarget();
		if (!WatchT || OwnerCharacter->IsTargetDeadOrInvalid(WatchT))
		{
			if (bHadTargetThisActivation && Now >= IntroEndTime)
			{
				if (TargetInvalidTime < 0.f) TargetInvalidTime = Now;
				else if (Now - TargetInvalidTime > 3.f)
				{
					ResetEncounter();
					return;
				}
			}
		}
		else
		{
			bHadTargetThisActivation = true;
			TargetInvalidTime = -1.f;
			const float ArenaR = GetArenaRadius();
			if (ArenaR > 0.f && FVector::Dist2D(WatchT->GetActorLocation(), ResolveArenaCentre()) > ArenaR * 1.6f)
			{
				if (TargetOutOfArenaTime < 0.f) TargetOutOfArenaTime = Now;
				else if (Now - TargetOutOfArenaTime > 8.f)
				{
					ResetEncounter();
					return;
				}
			}
			else TargetOutOfArenaTime = -1.f;
		}

		UpdatePhaseFromHealth();
		if (State == EGolemAttackState::Windup && !bToppled)
		{

			TickWindupAim(Elapsed);
		}
		else if (State == EGolemAttackState::Idle && !bToppled && Now >= IntroEndTime)
		{
			TickFacing(Elapsed);
			if (Now >= NextAttackReadyTime)
			{
				AActor* T = OwnerCharacter->GetCurrentTarget();
				const bool bHasTarget = (T && !OwnerCharacter->IsTargetDeadOrInvalid(T));

				bool bFacingOK = true;
				if (bFaceTarget && bHasTarget)
				{
					FVector To = T->GetActorLocation() - OwnerCharacter->GetActorLocation();
					To.Z = 0.f;
					if (!To.IsNearlyZero())
					{
						const float YawErr = FMath::Abs(FMath::FindDeltaAngleDegrees(
							OwnerCharacter->GetActorRotation().Yaw, To.Rotation().Yaw));
						bFacingOK = YawErr <= AttackFacingToleranceDeg;
					}
				}

				bool bTargetInArena = true;
				if (bHasTarget && PendingForcedAttack < 0)
				{
					const float ArenaR = GetArenaRadius();
					if (ArenaR > 0.f)
						bTargetInArena = FVector::Dist2D(T->GetActorLocation(), ResolveArenaCentre()) <= ArenaR * 1.1f;
				}

				if (bFacingOK && bTargetInArena && (bHasTarget || PendingForcedAttack >= 0))
				{
					const int32 Idx = SelectNextAttack();
					if (Idx >= 0) BeginAttack(Idx);
				}
			}
		}
	}

	if (bDrawDebugHazards) DrawLiveDebug(BrainInterval);
}

void UGolemBossComponent::TickWindupAim(float DeltaTime)
{
	if (!Attacks.IsValidIndex(CurrentAttackIndex) || !OwnerCharacter) return;
	const FGolemAttackConfig& Cfg = Attacks[CurrentAttackIndex];

	TickFacing(DeltaTime * 0.5f);

	if (Cfg.TargetMode != EGolemTargetMode::AtTarget) return;
	if (Cfg.Shape != EGolemHazardShape::RadialSlam && Cfg.Shape != EGolemHazardShape::ProjectileImpact) return;
	if (Cfg.Shape == EGolemHazardShape::ProjectileImpact && bRockLaunched) return;
	if (CurrentTelegraph.ImpactPoints.Num() == 0 || Cfg.WindupDuration <= 0.f) return;

	UWorld* W = GetWorld();
	if (!W) return;
	const float Remaining = W->GetTimerManager().GetTimerRemaining(WindupTimerHandle);
	if (Remaining < Cfg.WindupDuration * (1.f - FMath::Clamp(WindupAimTrackFraction, 0.f, 0.9f))) return;

	const FVector NewPt = FMath::VInterpTo(CurrentTelegraph.ImpactPoints[0], ResolveTargetLocation(), DeltaTime, 4.f);
	CurrentTelegraph.ImpactPoints[0] = NewPt;
	if (TrackedTelegraphDecal.IsValid())
		TrackedTelegraphDecal->SetWorldLocation(NewPt);
}

void UGolemBossComponent::ActivateBoss()
{
	if (bActivated || !OwnerCharacter) return;
	bActivated = true;
	CurrentPhase = 0;
	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;

	float IntroDelay = FMath::Max(0.f, IntroDuration);
	if (IntroMontage)
	{
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->PlayActionMontage(IntroMontage);
		if (IntroDelay <= 0.f) IntroDelay = IntroMontage->GetPlayLength();
	}
	IntroEndTime = Now + IntroDelay;
	NextAttackReadyTime = FMath::Max(IntroEndTime, Now + FMath::FRandRange(0.4f, 0.4f + GlobalCooldownRandom));
	TargetInvalidTime = -1.f;
	TargetOutOfArenaTime = -1.f;
	bHadTargetThisActivation = false;

	if (UAICombatComponent* Combat = OwnerCharacter->GetAICombat())
		Combat->EnterCombat();

	OnGolemActivated.Broadcast();
}

void UGolemBossComponent::DeactivateBoss()
{
	if (State != EGolemAttackState::Idle) FinishAttack(true);
	ClearAirZones();
	if (UAICombatComponent* Combat = OwnerCharacter ? OwnerCharacter->GetAICombat() : nullptr)
		Combat->ExitCombat();
	HideBossUIAndMusic();
	bActivated = false;
	PendingForcedAttack = -1;
	TargetInvalidTime = -1.f;
	TargetOutOfArenaTime = -1.f;
}

void UGolemBossComponent::ResetEncounter()
{
	if (OwnerCharacter)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->StopCurrentAction();
	if (State != EGolemAttackState::Idle) FinishAttack(true);
	if (bToppled) EndTopple();
	ClearAirZones();
	ClearArenaCrystals();
	EndBigCrystal();

	for (FGolemWeakPoint& WP : WeakPoints)
	{
		WP.CurrentHealth = WP.Health;
		WP.bBroken = false;
		WP.bVulnerable = false;
		WP.CrystalHits = 0;
	}

	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(ExposeLingerTimerHandle);
	ExposingAttackId = NAME_None;
	for (float& T : AttackReadyTimes) T = 0.f;
	LastAttackIndex = -1;

	CurrentPhase = 0;
	PendingForcedAttack = -1;
	NextAttackReadyTime = 0.f;
	IntroEndTime = 0.f;
	TargetInvalidTime = -1.f;
	TargetOutOfArenaTime = -1.f;
	bHadTargetThisActivation = false;

	if (OwnerCharacter)
	{
		if (UHealthComponent* HP = OwnerCharacter->FindComponentByClass<UHealthComponent>())
			HP->Heal(HP->GetMaxHealth());
		OwnerCharacter->ClearTarget();
		if (UAICombatComponent* Combat = OwnerCharacter->GetAICombat())
			Combat->ExitCombat();
	}

	HideBossUIAndMusic();
	bActivated = false;
	OnGolemEncounterReset.Broadcast();
}

bool UGolemBossComponent::ForceAttack(FName AttackId)
{
	const int32 Idx = FindAttackIndex(AttackId);
	if (Idx < 0) return false;
	PendingForcedAttack = Idx;
	return true;
}

void UGolemBossComponent::TriggerStrikeNow()
{
	if (State == EGolemAttackState::Windup)
	{
		if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(WindupTimerHandle);
		EnterActive();
	}
}

void UGolemBossComponent::EndAttackNow()
{
	if (State != EGolemAttackState::Idle) FinishAttack(false);
}

void UGolemBossComponent::LaunchRockNow()
{
	if (bRockLaunched || State != EGolemAttackState::Windup) return;
	if (!Attacks.IsValidIndex(CurrentAttackIndex)) return;
	const FGolemAttackConfig& Cfg = Attacks[CurrentAttackIndex];
	if (Cfg.Shape != EGolemHazardShape::ProjectileImpact) return;

	bRockLaunched = true;
	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(RockReleaseTimerHandle);

	const FVector Origin = HeldRock ? HeldRock->GetActorLocation() : GetThrowOrigin();
	const FVector Target = CurrentTelegraph.ImpactPoints.Num() > 0 ? CurrentTelegraph.ImpactPoints[0] : ResolveArenaCentre();

	float Travel = 0.05f;
	if (UWorld* W = GetWorld())
	{
		const float Rem = W->GetTimerManager().GetTimerRemaining(WindupTimerHandle);
		if (Rem > 0.f) Travel = Rem;

		if (RockThrowTravelTime > 0.f)
		{
			Travel = RockThrowTravelTime;
			W->GetTimerManager().SetTimer(WindupTimerHandle, this, &UGolemBossComponent::OnWindupElapsed, Travel, false);
		}
	}

	FGolemProjectileLaunch L;
	L.AttackId = Cfg.AttackId; L.Shape = Cfg.Shape;
	L.Origin = Origin; L.Target = Target; L.TravelTime = Travel; L.Index = 0; L.Count = 1;
	OnGolemProjectileLaunch.Broadcast(L);

	AGolemFallingRock* Rock = nullptr;
	if (HeldRock)
	{
		Rock = HeldRock;
		HeldRock = nullptr;
		Rock->Launch(Origin, Target, Travel, RockThrowArcHeight);
		InFlightRocks.Add(Rock);
	}
	else
	{
		Rock = SpawnFallingRock(Origin, Target, Travel, RockThrowArcHeight);
	}
	ApplyRockImpactDecal(Rock, Target, Cfg.ImpactRadius, Travel + 0.4f);
}

void UGolemBossComponent::InterruptAttack()
{
	if (State == EGolemAttackState::Idle) return;
	if (OwnerCharacter)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->StopCurrentAction();
	FinishAttack(true);
}

void UGolemBossComponent::ClearSequenceTimers()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(WindupTimerHandle);
		W->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		W->GetTimerManager().ClearTimer(RockReleaseTimerHandle);
	}
}

void UGolemBossComponent::TickFacing(float DeltaTime)
{
	if (!bFaceTarget || !OwnerCharacter) return;
	AActor* T = OwnerCharacter->GetCurrentTarget();
	if (!T) return;

	FVector ToTarget = T->GetActorLocation() - OwnerCharacter->GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero()) return;

	const float DesiredYaw = ToTarget.Rotation().Yaw;
	const FRotator Cur = OwnerCharacter->GetActorRotation();
	const float NewYaw = FMath::FixedTurn(Cur.Yaw, DesiredYaw, FaceTurnSpeed * DeltaTime);
	OwnerCharacter->SetActorRotation(FRotator(Cur.Pitch, NewYaw, Cur.Roll));
}

void UGolemBossComponent::TickContactRepulsion()
{
	if (!bRepelOnContact || !OwnerCharacter || OwnerCharacter->IsDead()) return;
	if (bToppled) return;

	ACharacter* Player = Cast<ACharacter>(OwnerCharacter->GetCurrentTarget());
	if (!Player) return;

	FVector Out = Player->GetActorLocation() - OwnerCharacter->GetActorLocation();
	Out.Z = 0.f;
	const float Dist = Out.Size();
	if (Dist <= 1.f || Dist >= RepulsionRadius) return;

	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	if (Now < NextRepulsionTime) return;
	NextRepulsionTime = Now + RepulsionInterval;

	const FVector Dir = Out / Dist;
	Player->LaunchCharacter(Dir * RepulsionForce + FVector(0.f, 0.f, RepulsionForce * 0.25f), true, false);
	OnGolemRepelledPlayer.Broadcast(Player, Dir);
}

void UGolemBossComponent::BeginAttack(int32 Index)
{
	if (!Attacks.IsValidIndex(Index) || !OwnerCharacter) return;

	const FGolemAttackConfig& Cfg = Attacks[Index];
	CurrentAttackIndex = Index;
	LastAttackIndex = Index;
	State = EGolemAttackState::Windup;
	DamageTickAccum = 0.f;
	HazardVictimsThisAttack.Reset();
	TrackedTelegraphDecal = nullptr;

	BuildTelegraph(Cfg, CurrentTelegraph);
	BombardImpactFired.Init(false, CurrentTelegraph.ImpactPoints.Num());
	BombardLaunched.Init(false, CurrentTelegraph.ImpactPoints.Num());

	UAnimMontage* MontageToPlay = Cfg.Montage;
	if (Cfg.Shape == EGolemHazardShape::SweepLine)
	{
		const bool bTravelsRight = FVector::DotProduct(CurrentTelegraph.LineDirection, OwnerCharacter->GetActorRightVector()) >= 0.f;
		UAnimMontage* DirMontage = bTravelsRight ? Cfg.SweepMontageLeftToRight : Cfg.SweepMontageRightToLeft;
		if (DirMontage) MontageToPlay = DirMontage;
	}
	if (MontageToPlay)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->PlayActionMontage(MontageToPlay);

	OnGolemAttackBegin.Broadcast(Cfg.AttackId, Cfg.Shape);
	OnGolemTelegraph.Broadcast(CurrentTelegraph);

	if (Cfg.WindupDuration > 0.f)
		if (UWorld* W = GetWorld())
			W->GetTimerManager().SetTimer(WindupTimerHandle, this, &UGolemBossComponent::OnWindupElapsed, Cfg.WindupDuration, false);

	bRockLaunched = false;
	if (Cfg.Shape == EGolemHazardShape::ProjectileImpact)
	{
		SpawnHeldRock();
		const float Release = FMath::Clamp(RockReleaseTime, 0.f, Cfg.WindupDuration);
		if (Release <= 0.f)
		{
			LaunchRockNow();
		}
		else if (UWorld* W = GetWorld())
		{
			W->GetTimerManager().SetTimer(RockReleaseTimerHandle, this, &UGolemBossComponent::LaunchRockNow, Release, false);
		}
	}

	if (bSpawnTelegraphDecals)
	{
		if (Cfg.Shape == EGolemHazardShape::RadialSlam)
		{
			if (CurrentTelegraph.ImpactPoints.Num() > 0)
				TrackedTelegraphDecal = SpawnTelegraphDecal(CurrentTelegraph.ImpactPoints[0], Cfg.ImpactRadius, Cfg.WindupDuration + 0.4f);
		}
		else if (Cfg.Shape == EGolemHazardShape::GroundFissures)
		{
			SpawnTelegraphDecal(ResolveArenaCentre(), GetArenaRadius(), Cfg.WindupDuration + Cfg.ActiveDuration + 0.4f);
		}
	}

	if (bDrawDebugHazards) DrawTelegraphDebug(CurrentTelegraph, FMath::Max(Cfg.WindupDuration, 0.05f));

	if (Cfg.WindupDuration <= 0.f) OnWindupElapsed();
}

void UGolemBossComponent::OnWindupElapsed()
{
	EnterActive();
}

void UGolemBossComponent::EnterActive()
{
	if (!Attacks.IsValidIndex(CurrentAttackIndex)) { FinishAttack(false); return; }
	const FGolemAttackConfig& Cfg = Attacks[CurrentAttackIndex];

	if (Cfg.Shape == EGolemHazardShape::ProjectileImpact && !bRockLaunched) LaunchRockNow();

	State = EGolemAttackState::Active;
	ActiveDurationCache = Cfg.ActiveDuration;
	ActiveTimer = Cfg.ActiveDuration;
	DamageTickAccum = 0.f;

	if (Cfg.bOpensAirZone)
	{
		const FVector Loc = CurrentTelegraph.ImpactPoints.Num() > 0 ? CurrentTelegraph.ImpactPoints[0] : ResolveArenaCentre();
		OpenAirZone(Loc, Cfg.AirZoneRadius, Cfg.AirZoneLifetime);
	}

	DoStrike();

	if (Cfg.bExposesArmWeakPoints)
	{
		if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(ExposeLingerTimerHandle);
		ExposeArmWeakPoints(true, Cfg.AttackId);
	}

	if (Cfg.ActiveDuration > 0.f)
		SetComponentTickEnabled(true);
	else
		EnterRecovery();
}

void UGolemBossComponent::TickActive(float DeltaTime)
{
	const bool bLastHazardFrame = (ActiveTimer - DeltaTime) <= 0.f;
	TickHazard(DeltaTime, bLastHazardFrame);
	ActiveTimer -= DeltaTime;
	if (ActiveTimer <= 0.f) EnterRecovery();
}

void UGolemBossComponent::EnterRecovery()
{
	SetComponentTickEnabled(false);
	ClearBeamVFX();
	if (!Attacks.IsValidIndex(CurrentAttackIndex)) { FinishAttack(false); return; }
	const FGolemAttackConfig& Cfg = Attacks[CurrentAttackIndex];

	if (Cfg.Shape == EGolemHazardShape::Bombardment)
	{
		const int32 Num = CurrentTelegraph.ImpactPoints.Num();
		for (int32 i = 0; i < Num; ++i)
		{
			if (BombardImpactFired.IsValidIndex(i) && BombardImpactFired[i]) continue;
			const FVector P = CurrentTelegraph.ImpactPoints[i];
			ApplyRadialBurst(P, Cfg.ImpactRadius, Cfg.Damage, Cfg.bAirborneIsSafe, Cfg.KnockbackForce, Cfg.AttackId, Cfg.MinSafeAltitude);
			if (BombardImpactFired.IsValidIndex(i)) BombardImpactFired[i] = true;

			FGolemStrikeEvent St;
			St.AttackId = Cfg.AttackId; St.Shape = Cfg.Shape; St.Location = P;
			St.Radius = Cfg.ImpactRadius; St.ImpactIndex = i; St.ImpactCount = Num;
			OnGolemStrike.Broadcast(St);
			if (bDrawDebugHazards) DrawImpactDebug(P, Cfg.ImpactRadius, Cfg.bAirborneIsSafe);
		}
	}

	State = EGolemAttackState::Recovery;
	OnGolemAttackRecovery.Broadcast(Cfg.AttackId);

	if (Cfg.RecoveryDuration > 0.f)
	{
		if (UWorld* W = GetWorld())
			W->GetTimerManager().SetTimer(RecoveryTimerHandle, this, &UGolemBossComponent::OnRecoveryElapsed, Cfg.RecoveryDuration, false);
	}
	else
	{
		FinishAttack(false);
	}
}

void UGolemBossComponent::OnRecoveryElapsed()
{
	FinishAttack(false);
}

void UGolemBossComponent::FinishAttack(bool bInterrupted)
{
	SetComponentTickEnabled(false);
	ClearSequenceTimers();
	ClearBeamVFX();

	if (HeldRock) { HeldRock->Destroy(); HeldRock = nullptr; }

	if (bInterrupted)
	{
		for (const TWeakObjectPtr<AActor>& R : InFlightRocks)
			if (AActor* Rock = R.Get()) Rock->Destroy();
	}
	InFlightRocks.Reset();

	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;

	if (Attacks.IsValidIndex(CurrentAttackIndex))
	{
		const FGolemAttackConfig& Cfg = Attacks[CurrentAttackIndex];
		if (AttackReadyTimes.IsValidIndex(CurrentAttackIndex))
			AttackReadyTimes[CurrentAttackIndex] = Now + Cfg.Cooldown * GetPhaseCooldownScale();
		OnGolemAttackEnd.Broadcast(Cfg.AttackId, bInterrupted);

		if (Cfg.bExposesArmWeakPoints && ExposingAttackId == Cfg.AttackId && !bToppled)
		{
			if (WeakPointExposeLinger > 0.f)
			{
				if (W)
					W->GetTimerManager().SetTimer(ExposeLingerTimerHandle, this, &UGolemBossComponent::OnExposeLingerElapsed, WeakPointExposeLinger, false);
			}
			else
			{
				ExposeArmWeakPoints(false, ExposingAttackId);
			}
		}
	}

	NextAttackReadyTime = Now + (GlobalCooldown + FMath::FRandRange(0.f, GlobalCooldownRandom)) * GetPhaseCooldownScale();
	State = EGolemAttackState::Idle;
	CurrentAttackIndex = -1;
	DamageTickAccum = 0.f;
	BombardImpactFired.Reset();
	BombardLaunched.Reset();
}

bool UGolemBossComponent::IsAttackUsable(int32 Index) const
{
	if (!Attacks.IsValidIndex(Index)) return false;
	const FGolemAttackConfig& A = Attacks[Index];
	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	const float Ready = AttackReadyTimes.IsValidIndex(Index) ? AttackReadyTimes[Index] : 0.f;
	if (Now < Ready) return false;
	if (A.MinPhase > CurrentPhase) return false;
	return true;
}

int32 UGolemBossComponent::FindAttackIndex(FName AttackId) const
{
	for (int32 i = 0; i < Attacks.Num(); ++i)
		if (Attacks[i].AttackId == AttackId) return i;
	return -1;
}

int32 UGolemBossComponent::FindAirZoneOpener() const
{
	for (int32 i = 0; i < Attacks.Num(); ++i)
		if (Attacks[i].bOpensAirZone && IsAttackUsable(i)) return i;
	for (int32 i = 0; i < Attacks.Num(); ++i)
		if (Attacks[i].bOpensAirZone) return i;
	return -1;
}

int32 UGolemBossComponent::SelectNextAttack()
{
	auto MarkReady = [this](int32 i) { if (AttackReadyTimes.IsValidIndex(i)) AttackReadyTimes[i] = 0.f; };

	bool bTargetHoveringSafe = false;
	if (OwnerCharacter)
		if (AActor* Tgt = OwnerCharacter->GetCurrentTarget())
			bTargetHoveringSafe = IsActorAirborne(Tgt) && IsLocationInAirZone(Tgt->GetActorLocation());

	auto WeightedPick = [this, bTargetHoveringSafe](bool bExcludeRequiresAirZone) -> int32
	{
		float Total = 0.f;
		TArray<TPair<int32, float>> Candidates;
		for (int32 i = 0; i < Attacks.Num(); ++i)
		{
			if (!IsAttackUsable(i)) continue;
			if (bExcludeRequiresAirZone && Attacks[i].bRequiresAirZoneEscape) continue;

			float Wt = Attacks[i].SelectionWeight;
			if (i == LastAttackIndex) Wt = FMath::Max(Wt * RepeatPenalty, KINDA_SMALL_NUMBER);
			if (bTargetHoveringSafe)
				Wt *= (Attacks[i].Shape == EGolemHazardShape::BeamSweep) ? 4.f : 0.2f;
			if (Wt <= 0.f) continue;
			Candidates.Add(TPair<int32, float>(i, Wt));
			Total += Wt;
		}
		if (Candidates.Num() == 0) return -1;
		float Roll = FMath::FRandRange(0.f, Total);
		for (const TPair<int32, float>& C : Candidates)
		{
			Roll -= C.Value;
			if (Roll <= 0.f) return C.Key;
		}
		return Candidates.Last().Key;
	};

	auto HasEscapeFor = [this](const FGolemAttackConfig& Cfg) -> bool
	{
		return HasAirZoneWithTime(Cfg.WindupDuration + Cfg.ActiveDuration + 1.f);
	};

	if (PendingForcedAttack >= 0)
	{
		const int32 Forced = PendingForcedAttack;
		PendingForcedAttack = -1;
		if (Attacks.IsValidIndex(Forced))
		{
			if (Attacks[Forced].bRequiresAirZoneEscape && !HasEscapeFor(Attacks[Forced]))
			{
				const int32 Opener = FindAirZoneOpener();
				if (Opener >= 0)
				{
					MarkReady(Opener);
					PendingForcedAttack = Forced;
					return Opener;
				}

			}
			else
			{
				MarkReady(Forced);
				return Forced;
			}
		}
	}

	const int32 Chosen = WeightedPick(false);
	if (Chosen < 0) return -1;

	if (Attacks[Chosen].bRequiresAirZoneEscape && !HasEscapeFor(Attacks[Chosen]))
	{
		const int32 Opener = FindAirZoneOpener();
		if (Opener >= 0)
		{
			MarkReady(Opener);
			PendingForcedAttack = Chosen;
			return Opener;
		}
		const int32 ZoneFree = WeightedPick(true);
		return ZoneFree >= 0 ? ZoneFree : Chosen;
	}

	return Chosen;
}

void UGolemBossComponent::BuildTelegraph(const FGolemAttackConfig& Cfg, FGolemTelegraph& Out)
{
	Out = FGolemTelegraph();
	Out.AttackId = Cfg.AttackId;
	Out.Shape = Cfg.Shape;
	Out.Duration = Cfg.WindupDuration;

	const FVector Centre = ResolveArenaCentre();
	const float Radius = GetArenaRadius();

	switch (Cfg.Shape)
	{
	case EGolemHazardShape::RadialSlam:
	case EGolemHazardShape::ProjectileImpact:
	{
		FVector Impact;
		switch (Cfg.TargetMode)
		{
		case EGolemTargetMode::ArenaCentre: Impact = Centre; break;
		case EGolemTargetMode::ArenaRandom: Impact = RandomArenaPoint(1.f); break;
		default:                            Impact = ResolveTargetLocation(); break;
		}
		Out.ImpactPoints.Add(Impact);
		Out.ImpactRadii.Add(Cfg.ImpactRadius);
		if (Cfg.bOpensAirZone)
		{
			Out.AirZoneLocation = Impact;
			Out.AirZoneRadius = Cfg.AirZoneRadius;
		}
		break;
	}
	case EGolemHazardShape::SweepLine:
	{

		FVector Side = OwnerCharacter ? OwnerCharacter->GetActorRightVector() : FVector::RightVector;
		Side.Z = 0.f;
		Side = Side.GetSafeNormal();
		if (Side.IsNearlyZero()) Side = FVector::RightVector;
		if (FMath::RandBool()) Side = -Side;

		Out.LineDirection = Side;
		Out.LineCentre = Centre - Side * Radius;
		Out.LineLength = Cfg.SweepLength;
		Out.LineThickness = Cfg.SweepThickness;
		break;
	}
	case EGolemHazardShape::GroundFissures:
	{
		const FVector Axis = YawRotate(FVector::ForwardVector, FMath::FRandRange(0.f, 360.f));

		Out.ImpactPoints.Add(ProjectToGround(Centre + Axis * Radius));
		Out.ImpactPoints.Add(ProjectToGround(Centre - Axis * Radius));
		Out.ImpactRadii.Add(Cfg.ImpactRadius);
		Out.ImpactRadii.Add(Cfg.ImpactRadius);
		break;
	}
	case EGolemHazardShape::BeamSweep:
	{
		if (Cfg.bBeamRiseFromGround)
		{
			const FVector GolemLoc = OwnerCharacter ? OwnerCharacter->GetActorLocation() : ResolveArenaCentre();
			BuildBeamsToPoint(Cfg, ProjectToGround(FMath::Lerp(GolemLoc, ResolveTargetLocation(), FMath::Clamp(Cfg.BeamSweepStartFraction, 0.f, 1.f))), Out.Beams);
		}
		else
			BuildBeams(Cfg, -Cfg.BeamSweepAngle * 0.5f, Out.Beams);
		break;
	}
	case EGolemHazardShape::Bombardment:
	{
		const int32 Count = FMath::Max(1, Cfg.ImpactCount);
		const float Window = FMath::Max(0.f, Cfg.ActiveDuration);
		const float Lead = FMath::Clamp(Cfg.BombardmentWarnLead, 0.f, Window);
		const float Spread = FMath::Max(0.f, Window - Lead);
		for (int32 i = 0; i < Count; ++i)
		{
			Out.ImpactPoints.Add(RandomArenaPoint(Cfg.BombardmentSpread));
			Out.ImpactRadii.Add(Cfg.ImpactRadius);

			Out.ImpactDelays.Add(Lead + (static_cast<float>(i) / static_cast<float>(Count)) * Spread);
		}
		break;
	}
	}
}

void UGolemBossComponent::DoStrike()
{
	if (!Attacks.IsValidIndex(CurrentAttackIndex)) return;
	const FGolemAttackConfig& Cfg = Attacks[CurrentAttackIndex];

	auto Broadcast = [this, &Cfg](const FVector& Loc, float R, int32 Idx, int32 Num)
	{
		FGolemStrikeEvent S;
		S.AttackId = Cfg.AttackId;
		S.Shape = Cfg.Shape;
		S.Location = Loc;
		S.Radius = R;
		S.ImpactIndex = Idx;
		S.ImpactCount = Num;
		OnGolemStrike.Broadcast(S);
	};

	TArray<FVector> SocketPts;
	const bool bSockets = ResolveSocketPoints(Cfg, SocketPts);

	switch (Cfg.Shape)
	{
	case EGolemHazardShape::RadialSlam:
	case EGolemHazardShape::ProjectileImpact:
	{
		const FVector C = bSockets ? SocketPts[0]
			: (CurrentTelegraph.ImpactPoints.Num() > 0 ? CurrentTelegraph.ImpactPoints[0] : ResolveArenaCentre());
		ApplyRadialBurst(C, Cfg.ImpactRadius, Cfg.Damage, Cfg.bAirborneIsSafe, Cfg.KnockbackForce, Cfg.AttackId, Cfg.MinSafeAltitude);
		Broadcast(C, Cfg.ImpactRadius, 0, 1);
		if (bDrawDebugHazards) DrawImpactDebug(C, Cfg.ImpactRadius, Cfg.bAirborneIsSafe);
		break;
	}
	case EGolemHazardShape::GroundFissures:
	{

		if (bSockets) CurrentTelegraph.ImpactPoints = SocketPts;

		for (FVector& P : CurrentTelegraph.ImpactPoints)
			P = ProjectToGround(ClampToArena(P, ArenaCrystalEdgeMargin));
		ApplyRadialBurst(ResolveArenaCentre(), GetArenaRadius(), Cfg.Damage, true, Cfg.KnockbackForce, Cfg.AttackId, Cfg.MinSafeAltitude);
		for (int32 i = 0; i < CurrentTelegraph.ImpactPoints.Num(); ++i)
			Broadcast(CurrentTelegraph.ImpactPoints[i], Cfg.ImpactRadius, i, CurrentTelegraph.ImpactPoints.Num());
		if (bSlamPlantsArenaCrystals) PlantArenaCrystals();
		break;
	}
	case EGolemHazardShape::SweepLine:
		Broadcast(CurrentTelegraph.LineCentre, CurrentTelegraph.LineThickness, 0, 1);
		break;
	case EGolemHazardShape::BeamSweep:
	{
		if (Cfg.bBeamRiseFromGround)
		{
			const FVector Tgt = ResolveTargetLocation();
			const FVector GolemLoc = OwnerCharacter ? OwnerCharacter->GetActorLocation() : ResolveArenaCentre();
			BeamRiseGround = ProjectToGround(FMath::Lerp(GolemLoc, Tgt, FMath::Clamp(Cfg.BeamSweepStartFraction, 0.f, 1.f)));
			BeamRiseHigh   = Tgt + FVector(0.f, 0.f, Cfg.BeamRiseExtraHeight);
			BuildBeamsToPoint(Cfg, BeamRiseGround, CurrentTelegraph.Beams);
		}
		else
		{
			BuildBeams(Cfg, 0.f, CurrentTelegraph.Beams);
		}
		SpawnBeamVFX(Cfg, CurrentTelegraph.Beams);
		SpawnBeamMesh(Cfg, CurrentTelegraph.Beams);

		if (Cfg.BeamFireVFX)
		{
			if (Cfg.BeamFireVFXDelay > 0.f)
			{
				UNiagaraSystem* const Sys = Cfg.BeamFireVFX;
				if (UWorld* W = GetWorld())
					W->GetTimerManager().SetTimer(BeamFireVFXTimerHandle, [this, Sys]() { SpawnBeamFireVFX(Sys); }, Cfg.BeamFireVFXDelay, false);
			}
			else
			{
				SpawnBeamFireVFX(Cfg.BeamFireVFX);
			}
		}
		const FVector Origin = CurrentTelegraph.Beams.Num() > 0 ? CurrentTelegraph.Beams[0].Origin : ResolveArenaCentre();
		Broadcast(Origin, Cfg.BeamWidth, 0, 1);
		break;
	}
	case EGolemHazardShape::Bombardment:
	{
		if (ActiveDurationCache <= KINDA_SMALL_NUMBER)
		{
			const int32 Num = CurrentTelegraph.ImpactPoints.Num();
			for (int32 i = 0; i < Num; ++i)
			{
				const FVector P = CurrentTelegraph.ImpactPoints[i];
				ApplyRadialBurst(P, Cfg.ImpactRadius, Cfg.Damage, Cfg.bAirborneIsSafe, Cfg.KnockbackForce, Cfg.AttackId, Cfg.MinSafeAltitude);
				if (BombardImpactFired.IsValidIndex(i)) BombardImpactFired[i] = true;
				if (BombardLaunched.IsValidIndex(i)) BombardLaunched[i] = true;
				Broadcast(P, Cfg.ImpactRadius, i, Num);
				if (bDrawDebugHazards) DrawImpactDebug(P, Cfg.ImpactRadius, Cfg.bAirborneIsSafe);
			}
		}
		break;
	}
	}
}

void UGolemBossComponent::TickHazard(float DeltaTime, bool bForceFinal)
{
	if (!Attacks.IsValidIndex(CurrentAttackIndex)) return;
	const FGolemAttackConfig& Cfg = Attacks[CurrentAttackIndex];

	const float Dur = FMath::Max(ActiveDurationCache, KINDA_SMALL_NUMBER);
	const float Alpha = bForceFinal ? 1.f : FMath::Clamp(1.f - ActiveTimer / Dur, 0.f, 1.f);
	const float ElapsedActive = Dur - FMath::Max(ActiveTimer, 0.f);

	const float Interval = FMath::Max(0.02f, Cfg.DamageInterval);
	DamageTickAccum += DeltaTime;
	bool bDamageTick = false;
	if (DamageTickAccum >= Interval) { bDamageTick = true; DamageTickAccum -= Interval; }
	if (bForceFinal) bDamageTick = true;

	FGolemHazardState S;
	S.AttackId = Cfg.AttackId;
	S.Shape = Cfg.Shape;
	S.Alpha = Alpha;

	switch (Cfg.Shape)
	{
	case EGolemHazardShape::RadialSlam:
	{
		if (!Cfg.bEmitShockwave) break;
		const FVector C = CurrentTelegraph.ImpactPoints.Num() > 0 ? CurrentTelegraph.ImpactPoints[0] : ResolveArenaCentre();
		const float Outer = Alpha * Cfg.ShockwaveMaxRadius;
		const float Inner = FMath::Max(0.f, Outer - Cfg.ShockwaveThickness);
		if (bDamageTick)
			ApplyRingBurst(C, Inner, Outer, Cfg.ShockwaveDamage, true, Cfg.KnockbackForce, Cfg.AttackId);
		S.RingRadius = Outer;
#if ENABLE_DRAW_DEBUG
		if (bDrawDebugHazards) DrawFlatCircle(GetWorld(), C, Outer, FColor::Orange, -1.f, 10.f);
#endif
		break;
	}
	case EGolemHazardShape::GroundFissures:
	{
		if (bDamageTick)
			ApplyRadialBurst(ResolveArenaCentre(), GetArenaRadius(), Cfg.Damage, true, Cfg.KnockbackForce, Cfg.AttackId, Cfg.MinSafeAltitude);
#if ENABLE_DRAW_DEBUG
		if (bDrawDebugHazards) DrawFlatCircle(GetWorld(), ProjectToGround(ResolveArenaCentre()), GetArenaRadius(), FColor::Red, -1.f, 8.f);
#endif
		break;
	}
	case EGolemHazardShape::SweepLine:
	{
		const FVector Dir = CurrentTelegraph.LineDirection;
		const FVector Start = ResolveArenaCentre() - Dir * GetArenaRadius();

		const FVector Cur = ProjectToGround(Start + Dir * (Alpha * 2.f * GetArenaRadius()));
		const FVector Perp = YawRotate(Dir, 90.f);
		const FVector A = Cur + Perp * (Cfg.SweepLength * 0.5f);
		const FVector B = Cur - Perp * (Cfg.SweepLength * 0.5f);
		if (bDamageTick)

			ApplyLineDamageOncePerVictim(A, B, Cfg.SweepThickness, Cfg.Damage, true, false, Cfg.KnockbackForce, Cfg.AttackId, Cfg.MinSafeAltitude, Dir);
		S.LineCentre = Cur;
		S.LineDirection = Dir;
#if ENABLE_DRAW_DEBUG
		if (bDrawDebugHazards) DrawDebugLine(GetWorld(), A, B, FColor::Red, false, -1.f, 0, 12.f);
#endif
		break;
	}
	case EGolemHazardShape::BeamSweep:
	{
		if (Cfg.bBeamRiseFromGround)
		{

			const float Hold = FMath::Clamp(Cfg.BeamRiseHoldFraction, 0.f, 0.95f);
			const float RiseAlpha = (Alpha <= Hold) ? 0.f : (Alpha - Hold) / FMath::Max(0.01f, 1.f - Hold);
			BuildBeamsToPoint(Cfg, FMath::Lerp(BeamRiseGround, BeamRiseHigh, RiseAlpha), S.Beams);
		}
		else if (Cfg.BeamSweepAngle > KINDA_SMALL_NUMBER)
		{
			const float Rotate = FMath::Lerp(-Cfg.BeamSweepAngle * 0.5f, Cfg.BeamSweepAngle * 0.5f, Alpha);
			BuildBeams(Cfg, Rotate, S.Beams);
		}
		else
		{
			S.Beams = CurrentTelegraph.Beams;
		}
		UpdateBeamVFX(S.Beams);
		UpdateBeamMesh(S.Beams);
		if (bDamageTick)
			for (const FGolemBeamSegment& Beam : S.Beams)
				ApplyLineDamage(Beam.Origin, Beam.End, Cfg.BeamWidth, Cfg.Damage, false, true, Cfg.KnockbackForce, Cfg.AttackId);
#if ENABLE_DRAW_DEBUG
		if (bDrawDebugHazards)
			for (const FGolemBeamSegment& Beam : S.Beams)
				DrawDebugLine(GetWorld(), Beam.Origin, Beam.End, FColor::Purple, false, -1.f, 0, 8.f);
#endif
		break;
	}
	case EGolemHazardShape::Bombardment:
	{
		const int32 Num = CurrentTelegraph.ImpactPoints.Num();
		for (int32 i = 0; i < Num; ++i)
		{
			const float When = CurrentTelegraph.ImpactDelays.IsValidIndex(i) ? CurrentTelegraph.ImpactDelays[i] : 0.f;
			const float Lead = FMath::Clamp(Cfg.BombardmentWarnLead, 0.f, When);
			const FVector P = CurrentTelegraph.ImpactPoints[i];

			if (BombardLaunched.IsValidIndex(i) && !BombardLaunched[i] && ElapsedActive >= (When - Lead))
			{
				BombardLaunched[i] = true;
				FGolemProjectileLaunch L;
				L.AttackId = Cfg.AttackId; L.Shape = Cfg.Shape;
				L.Origin = P + FVector(0, 0, BombardmentSpawnHeight);
				L.Target = P;
				L.TravelTime = FMath::Max(Lead, 0.05f);
				L.Index = i; L.Count = Num;
				OnGolemProjectileLaunch.Broadcast(L);
				AGolemFallingRock* Rock = SpawnFallingRock(L.Origin, L.Target, L.TravelTime, 0.f);
				ApplyRockImpactDecal(Rock, P, Cfg.ImpactRadius, FMath::Max(Lead, 0.1f) + 0.3f);
			}

			if (BombardImpactFired.IsValidIndex(i) && !BombardImpactFired[i] && ElapsedActive >= When)
			{
				BombardImpactFired[i] = true;
				ApplyRadialBurst(P, Cfg.ImpactRadius, Cfg.Damage, Cfg.bAirborneIsSafe, Cfg.KnockbackForce, Cfg.AttackId, Cfg.MinSafeAltitude);

				FGolemStrikeEvent St;
				St.AttackId = Cfg.AttackId; St.Shape = Cfg.Shape; St.Location = P;
				St.Radius = Cfg.ImpactRadius; St.ImpactIndex = i; St.ImpactCount = Num;
				OnGolemStrike.Broadcast(St);
				if (bDrawDebugHazards) DrawImpactDebug(P, Cfg.ImpactRadius, Cfg.bAirborneIsSafe);
			}
		}
		break;
	}
	default: break;
	}

	OnGolemHazardTick.Broadcast(S);
}

void UGolemBossComponent::GetEyeOrigins(TArray<FVector>& Out) const
{
	Out.Reset();
	if (OwnerCharacter)
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
			for (const FName& SocketName : EyeSocketNames)
				if (SocketName != NAME_None && Mesh->DoesSocketExist(SocketName))
					Out.Add(Mesh->GetSocketLocation(SocketName));

	if (Out.Num() == 0 && OwnerCharacter)
		Out.Add(OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 80.f + FVector(0, 0, 200.f));
}

FVector UGolemBossComponent::GetThrowOrigin() const
{
	if (OwnerCharacter && ThrowSocketName != NAME_None)
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
			if (Mesh->DoesSocketExist(ThrowSocketName))
				return Mesh->GetSocketLocation(ThrowSocketName);
	if (OwnerCharacter)
		return OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 120.f + FVector(0, 0, 300.f);
	return ResolveArenaCentre();
}

UDecalComponent* UGolemBossComponent::SpawnTelegraphDecal(const FVector& Location, float RadiusXY, float LifeSpan) const
{
	if (!TelegraphDecalMaterial || RadiusXY <= 0.f) return nullptr;
	UWorld* W = GetWorld();
	if (!W) return nullptr;

	const FVector DecalSize(DecalProjectionDepth, RadiusXY, RadiusXY);
	return UGameplayStatics::SpawnDecalAtLocation(W, TelegraphDecalMaterial, DecalSize, Location, FRotator(-90.f, 0.f, 0.f), FMath::Max(LifeSpan, 0.1f));
}

AGolemFallingRock* UGolemBossComponent::SpawnFallingRock(const FVector& Origin, const FVector& Target, float TravelTime, float ArcHeight)
{
	if (!bSpawnRockMeshes) return nullptr;
	UWorld* W = GetWorld();
	if (!W) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = OwnerCharacter;

	if (RockActorClass)
	{
		if (AActor* Spawned = W->SpawnActor<AActor>(RockActorClass, Origin, (Target - Origin).Rotation(), SpawnParams))
			InFlightRocks.Add(Spawned);
		return nullptr;
	}

	if (RockMeshes.Num() == 0) return nullptr;
	UStaticMesh* Mesh = RockMeshes[FMath::RandRange(0, RockMeshes.Num() - 1)];
	if (!Mesh) return nullptr;

	AGolemFallingRock* Rock = W->SpawnActor<AGolemFallingRock>(AGolemFallingRock::StaticClass(), Origin, FRotator::ZeroRotator, SpawnParams);
	if (Rock)
	{
		Rock->Configure(Mesh, RockMeshScale, RockSpinSpeed);
		Rock->Launch(Origin, Target, TravelTime, ArcHeight);
		InFlightRocks.Add(Rock);
	}
	return Rock;
}

void UGolemBossComponent::ApplyRockImpactDecal(AGolemFallingRock* Rock, const FVector& Target, float Radius, float StaticLifeSpan)
{
	if (!bSpawnTelegraphDecals || !TelegraphDecalMaterial || Radius <= 0.f) return;
	if (Rock) Rock->SetImpactDecal(TelegraphDecalMaterial, Radius, DecalProjectionDepth);
	else SpawnTelegraphDecal(Target, Radius, StaticLifeSpan);
}

void UGolemBossComponent::SpawnHeldRock()
{
	HeldRock = nullptr;
	if (!bSpawnRockMeshes || RockActorClass) return;
	if (RockMeshes.Num() == 0 || !OwnerCharacter) return;

	USkeletalMeshComponent* OwnerMesh = OwnerCharacter->GetMesh();
	UWorld* W = GetWorld();
	if (!OwnerMesh || !W) return;

	UStaticMesh* Mesh = RockMeshes[FMath::RandRange(0, RockMeshes.Num() - 1)];
	if (!Mesh) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = OwnerCharacter;

	AGolemFallingRock* Rock = W->SpawnActor<AGolemFallingRock>(AGolemFallingRock::StaticClass(), GetThrowOrigin(), FRotator::ZeroRotator, SpawnParams);
	if (!Rock) return;

	Rock->Configure(Mesh, RockMeshScale, RockSpinSpeed);
	Rock->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, ThrowSocketName);
	HeldRock = Rock;
}

void UGolemBossComponent::BuildBeams(const FGolemAttackConfig& Cfg, float RotateDeg, TArray<FGolemBeamSegment>& Out) const
{
	Out.Reset();
	TArray<FVector> Eyes;
	GetEyeOrigins(Eyes);
	if (Eyes.Num() == 0) return;

	const int32 Count = FMath::Max(1, Cfg.BeamCount);
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Origin = Eyes[i % Eyes.Num()];
		FVector Base = ResolveTargetLocation() - Origin;
		if (Base.IsNearlyZero()) Base = OwnerCharacter ? OwnerCharacter->GetActorForwardVector() : FVector::ForwardVector;
		Base = Base.GetSafeNormal();

		const float Frac = (Count > 1) ? static_cast<float>(i) / static_cast<float>(Count - 1) : 0.5f;
		const float FanYaw = FMath::Lerp(-Cfg.BeamSpreadAngle * 0.5f, Cfg.BeamSpreadAngle * 0.5f, Frac);
		const FVector Dir = YawRotate(Base, FanYaw + RotateDeg).GetSafeNormal();

		FGolemBeamSegment Beam;
		Beam.Origin = Origin;
		Beam.Direction = Dir;
		Beam.Length = Cfg.BeamLength;
		Beam.End = Origin + Dir * Cfg.BeamLength;
		Beam.Width = Cfg.BeamWidth;
		Out.Add(Beam);
	}
}

void UGolemBossComponent::BuildBeamsToPoint(const FGolemAttackConfig& Cfg, const FVector& EndPoint, TArray<FGolemBeamSegment>& Out) const
{
	Out.Reset();
	TArray<FVector> Eyes;
	GetEyeOrigins(Eyes);
	if (Eyes.Num() == 0) return;

	const int32 Count = FMath::Max(1, Cfg.BeamCount);
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Origin = Eyes[i % Eyes.Num()];
		FVector Dir = (EndPoint - Origin).GetSafeNormal();
		if (Dir.IsNearlyZero()) Dir = OwnerCharacter ? OwnerCharacter->GetActorForwardVector() : FVector::ForwardVector;

		FGolemBeamSegment Beam;
		Beam.Origin = Origin;
		Beam.Direction = Dir;
		Beam.End = EndPoint;
		Beam.Length = (EndPoint - Origin).Size();
		Beam.Width = Cfg.BeamWidth;
		Out.Add(Beam);
	}
}

void UGolemBossComponent::SpawnBeamVFX(const FGolemAttackConfig& Cfg, const TArray<FGolemBeamSegment>& Beams)
{
	ClearBeamVFX();
	UWorld* const W = GetWorld();
	if (!Cfg.BeamVFX || !W) return;

	for (const FGolemBeamSegment& B : Beams)
	{
		UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			W, Cfg.BeamVFX, B.Origin, B.Direction.Rotation(), FVector(1.f), false, true);
		if (!Comp) continue;
		ApplyBeamParams(Comp, B);
		BeamVFXComps.Add(Comp);
	}
}

void UGolemBossComponent::UpdateBeamVFX(const TArray<FGolemBeamSegment>& Beams)
{
	const int32 N = FMath::Min(BeamVFXComps.Num(), Beams.Num());
	for (int32 i = 0; i < N; ++i)
	{
		UNiagaraComponent* Comp = BeamVFXComps[i];
		if (!IsValid(Comp)) continue;
		ApplyBeamParams(Comp, Beams[i]);
	}
}

void UGolemBossComponent::ClearBeamVFX()
{
	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(BeamFireVFXTimerHandle);
	for (UNiagaraComponent* Comp : BeamVFXComps)
		if (IsValid(Comp)) { Comp->Deactivate(); Comp->DestroyComponent(); }
	BeamVFXComps.Reset();
	ClearBeamMesh();
}

void UGolemBossComponent::SpawnBeamFireVFX(UNiagaraSystem* System)
{
	if (!System) return;

	UNiagaraComponent* FireFX = nullptr;
	if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
		if (UCameraComponent* CamComp = P->FindComponentByClass<UCameraComponent>())
			FireFX = UNiagaraFunctionLibrary::SpawnSystemAttached(System, CamComp, NAME_None,
				FVector(120.f, 0.f, 0.f), FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	if (!FireFX)
		if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), System,
				Cam->GetCameraLocation() + Cam->GetCameraRotation().Vector() * 120.f, Cam->GetCameraRotation(), FVector(1.f), true, true);
}

void UGolemBossComponent::SpawnBeamMesh(const FGolemAttackConfig& Cfg, const TArray<FGolemBeamSegment>& Beams)
{
	ClearBeamMesh();
	if (!Cfg.bUseMeshBeam || !OwnerCharacter) return;

	UStaticMesh* Mesh = Cfg.BeamMesh ? Cfg.BeamMesh.Get() : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Mesh) return;

	for (int32 i = 0; i < Beams.Num(); ++i)
	{
		UStaticMeshComponent* MC = NewObject<UStaticMeshComponent>(OwnerCharacter);
		if (!MC) continue;
		MC->SetStaticMesh(Mesh);
		if (Cfg.BeamMeshMaterial) MC->SetMaterial(0, Cfg.BeamMeshMaterial);
		MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MC->SetCastShadow(false);
		MC->RegisterComponent();
		BeamMeshComps.Add(MC);
	}
	UpdateBeamMesh(Beams);
}

void UGolemBossComponent::UpdateBeamMesh(const TArray<FGolemBeamSegment>& Beams)
{
	const int32 N = FMath::Min(BeamMeshComps.Num(), Beams.Num());
	for (int32 i = 0; i < N; ++i)
	{
		UStaticMeshComponent* MC = BeamMeshComps[i];
		if (!IsValid(MC)) continue;

		const FVector Origin = Beams[i].Origin;
		const FVector End = Beams[i].End;
		const float Length = (End - Origin).Size();

		const FVector MeshSize = MC->GetStaticMesh() ? MC->GetStaticMesh()->GetBoundingBox().GetSize() : FVector(100.f);
		const float SX = MeshSize.X > 1.f ? Length / MeshSize.X : Length;
		const float SYZ = MeshSize.Y > 1.f ? (Beams[i].Width * 2.f) / MeshSize.Y : (Beams[i].Width * 2.f);

		MC->SetWorldLocation((Origin + End) * 0.5f);
		MC->SetWorldRotation((End - Origin).Rotation());
		MC->SetWorldScale3D(FVector(SX, SYZ, SYZ));
	}
}

void UGolemBossComponent::ClearBeamMesh()
{
	for (UStaticMeshComponent* MC : BeamMeshComps)
		if (IsValid(MC)) MC->DestroyComponent();
	BeamMeshComps.Reset();
}

FVector UGolemBossComponent::GetArenaCentre_Implementation() const
{
	if (bArenaCentreRelativeToActor && OwnerCharacter)
		return OwnerCharacter->GetActorLocation() + ArenaCentre;
	return ArenaCentre;
}

float UGolemBossComponent::GetArenaRadius_Implementation() const
{
	return ArenaRadius;
}

FVector UGolemBossComponent::ResolveTargetLocation_Implementation() const
{
	if (OwnerCharacter)
		if (AActor* T = OwnerCharacter->GetCurrentTarget())
			return ClampToArena(T->GetActorLocation());
	return ResolveArenaCentre();
}

FVector UGolemBossComponent::ResolveArenaCentre() const
{
	return GetArenaCentre();
}

FVector UGolemBossComponent::ClampToArena(const FVector& P, float Margin) const
{
	const FVector C = GetArenaCentre();
	const float R = FMath::Max(0.f, GetArenaRadius() - Margin);
	FVector Off = P - C;
	Off.Z = 0.f;
	if (Off.SizeSquared() > R * R)
		Off = Off.GetSafeNormal() * R;
	return FVector(C.X + Off.X, C.Y + Off.Y, C.Z);
}

FVector UGolemBossComponent::RandomArenaPoint(float SpreadFraction) const
{
	const FVector C = GetArenaCentre();
	const float R = GetArenaRadius() * FMath::Clamp(SpreadFraction, 0.f, 1.f);
	const float Ang = FMath::FRandRange(0.f, 2.f * PI);
	const float Rad = R * FMath::Sqrt(FMath::FRand());
	return FVector(C.X + FMath::Cos(Ang) * Rad, C.Y + FMath::Sin(Ang) * Rad, C.Z);
}

FVector UGolemBossComponent::ProjectToGround(const FVector& P) const
{
	const UWorld* W = GetWorld();
	if (!W) return P;

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GolemGround), false, OwnerCharacter);

	auto TraceFloor = [&](const FVector& At, float& OutZ) -> bool
	{
		const FVector Start = At + FVector(0.f, 0.f, 20000.f);
		const FVector End   = At - FVector(0.f, 0.f, 20000.f);
		FHitResult Hit;
		if (W->LineTraceSingleByObjectType(Hit, Start, End, ObjParams, Params)) { OutZ = Hit.ImpactPoint.Z; return true; }
		if (W->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) { OutZ = Hit.ImpactPoint.Z; return true; }
		return false;
	};

	float Z = 0.f;
	if (TraceFloor(P, Z))                 return FVector(P.X, P.Y, Z);
	if (TraceFloor(GetArenaCentre(), Z))  return FVector(P.X, P.Y, Z);
	return P;
}

bool UGolemBossComponent::ResolveSocketPoints(const FGolemAttackConfig& Cfg, TArray<FVector>& Out) const
{
	Out.Reset();
	if (Cfg.ImpactSocketNames.Num() == 0 || !OwnerCharacter) return false;

	const USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh) return false;

	for (const FName& S : Cfg.ImpactSocketNames)
	{
		if (S.IsNone()) continue;
		if (!Mesh->DoesSocketExist(S) && Mesh->GetBoneIndex(S) == INDEX_NONE) continue;
		Out.Add(ProjectToGround(Mesh->GetSocketLocation(S)));
	}
	return Out.Num() > 0;
}

void UGolemBossComponent::GatherTargets(FVector Center, float Radius, TArray<AActor*>& Out) const
{
	Out.Reset();
	UWorld* W = GetWorld();
	if (!W || !OwnerCharacter) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GolemHazard), false, OwnerCharacter);
	W->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
		FCollisionShape::MakeSphere(FMath::Max(Radius, 1.f)), Params);

	for (const FOverlapResult& O : Overlaps)
	{
		AActor* A = O.GetActor();
		if (!A || Out.Contains(A)) continue;
		if (!OwnerCharacter->IsValidTargetCandidate(A)) continue;
		Out.Add(A);
	}
}

bool UGolemBossComponent::IsActorAirborne(const AActor* A) const
{
	if (const ACharacter* C = Cast<ACharacter>(A))
		if (const UCharacterMovementComponent* M = C->GetCharacterMovement())
			return !M->IsMovingOnGround();
	return false;
}

float UGolemBossComponent::CurrentDamageScale() const
{
	return Phases.IsValidIndex(CurrentPhase) ? Phases[CurrentPhase].DamageScale : 1.f;
}

void UGolemBossComponent::DealDamage(AActor* Victim, float Damage, FVector FromLocation, float Knockback, FName AttackId, FVector KnockbackDirOverride)
{
	if (!Victim || !OwnerCharacter || Damage <= 0.f) return;

	float Final = Damage * CurrentDamageScale();

	bool bNegated = false;
	if (UCombatComponent* VictimCombat = Victim->FindComponentByClass<UCombatComponent>())
		Final = VictimCombat->MitigateIncomingDamage(Final, OwnerCharacter, bNegated);
	if (bNegated) return;

	FVector Dir = KnockbackDirOverride.IsNearlyZero() ? (Victim->GetActorLocation() - FromLocation) : KnockbackDirOverride;
	Dir.Z = 0.f;
	Dir = Dir.GetSafeNormal();
	if (Dir.IsNearlyZero()) Dir = OwnerCharacter->GetActorForwardVector();

	UGameplayStatics::ApplyPointDamage(Victim, Final, Dir, FHitResult(),
		OwnerCharacter->GetController(), OwnerCharacter, UDamageType::StaticClass());

	if (Knockback > 0.f)
		if (ACharacter* HitChar = Cast<ACharacter>(Victim))
			HitChar->LaunchCharacter(Dir * Knockback + FVector(0, 0, Knockback * 0.2f), false, false);

	OnGolemDamageDealt.Broadcast(Victim, Final, AttackId, Victim->GetActorLocation());
}

void UGolemBossComponent::DealDamageToBoss(float Amount, AActor* Instigator)
{
	if (Amount <= 0.f || !OwnerHealth) return;

	if (State == EGolemAttackState::Recovery && RecoveryDamageTakenMultiplier > 1.f)
		Amount *= RecoveryDamageTakenMultiplier;

	const bool bWasInvuln = OwnerHealth->IsInvulnerable();
	OwnerHealth->SetInvulnerable(false);
	OwnerHealth->TakeDamage(Amount);
	OwnerHealth->SetInvulnerable(bWasInvuln);

	if (Instigator && OwnerCharacter) OwnerCharacter->AddThreat(Instigator, Amount);
}

int32 UGolemBossComponent::ApplyRadialBurst(FVector Center, float Radius, float Damage, bool bAirborneIsSafe, float Knockback, FName AttackId, float MinSafeAltitude)
{
	TArray<AActor*> Targets;
	GatherTargets(Center, Radius, Targets);
	const float FloorZ = ProjectToGround(GetArenaCentre()).Z;
	int32 Hits = 0;
	for (AActor* A : Targets)
	{
		if (FVector::DistSquared2D(A->GetActorLocation(), Center) > Radius * Radius) continue;
		const bool bSafe = (MinSafeAltitude > 0.f)
			? ((A->GetActorLocation().Z - FloorZ) >= MinSafeAltitude)
			: (bAirborneIsSafe && IsActorAirborne(A));
		if (bSafe) continue;
		DealDamage(A, Damage, Center, Knockback, AttackId);
		++Hits;
	}
	return Hits;
}

int32 UGolemBossComponent::ApplyRingBurst(FVector Center, float InnerRadius, float OuterRadius, float Damage, bool bAirborneIsSafe, float Knockback, FName AttackId)
{
	TArray<AActor*> Targets;
	GatherTargets(Center, OuterRadius, Targets);
	int32 Hits = 0;
	const float In2 = InnerRadius * InnerRadius;
	const float Out2 = OuterRadius * OuterRadius;
	for (AActor* A : Targets)
	{
		const float D2 = FVector::DistSquared2D(A->GetActorLocation(), Center);
		if (D2 < In2 || D2 > Out2) continue;
		if (bAirborneIsSafe && IsActorAirborne(A)) continue;
		DealDamage(A, Damage, Center, Knockback, AttackId);
		++Hits;
	}
	return Hits;
}

int32 UGolemBossComponent::ApplyLineDamage(FVector A, FVector B, float HalfWidth, float Damage, bool bAirborneIsSafe, bool b3D, float Knockback, FName AttackId, float MinSafeAltitude, FVector KnockbackDir)
{
	const FVector Mid = (A + B) * 0.5f;
	const float GatherRadius = (FVector::Dist(A, B) * 0.5f) + HalfWidth + 50.f;
	TArray<AActor*> Targets;
	GatherTargets(Mid, GatherRadius, Targets);

	const float FloorZ = ProjectToGround(GetArenaCentre()).Z;
	int32 Hits = 0;
	for (AActor* Actor : Targets)
	{
		const FVector Loc = Actor->GetActorLocation();
		const float D = b3D ? FMath::PointDistToSegment(Loc, A, B) : SegDist2D(Loc, A, B);
		if (D > HalfWidth) continue;

		const bool bSafe = (MinSafeAltitude > 0.f)
			? ((Loc.Z - FloorZ) >= MinSafeAltitude)
			: (bAirborneIsSafe && IsActorAirborne(Actor));
		if (bSafe) continue;
		DealDamage(Actor, Damage, Loc, Knockback, AttackId, KnockbackDir);
		++Hits;
	}
	return Hits;
}

int32 UGolemBossComponent::ApplyLineDamageOncePerVictim(FVector A, FVector B, float HalfWidth, float Damage, bool bAirborneIsSafe, bool b3D, float Knockback, FName AttackId, float MinSafeAltitude, FVector KnockbackDir)
{
	const FVector Mid = (A + B) * 0.5f;
	const float GatherRadius = (FVector::Dist(A, B) * 0.5f) + HalfWidth + 50.f;
	TArray<AActor*> Targets;
	GatherTargets(Mid, GatherRadius, Targets);

	const float FloorZ = ProjectToGround(GetArenaCentre()).Z;
	int32 Hits = 0;
	for (AActor* Actor : Targets)
	{
		if (HazardVictimsThisAttack.Contains(Actor)) continue;
		const FVector Loc = Actor->GetActorLocation();
		const float D = b3D ? FMath::PointDistToSegment(Loc, A, B) : SegDist2D(Loc, A, B);
		if (D > HalfWidth) continue;
		const bool bSafe = (MinSafeAltitude > 0.f)
			? ((Loc.Z - FloorZ) >= MinSafeAltitude)
			: (bAirborneIsSafe && IsActorAirborne(Actor));
		if (bSafe) continue;
		HazardVictimsThisAttack.Add(Actor);
		DealDamage(Actor, Damage, Loc, Knockback, AttackId, KnockbackDir);
		++Hits;
	}
	return Hits;
}

void UGolemBossComponent::OpenAirZone(FVector Location, float Radius, float Lifetime)
{
	if (Radius <= 0.f) return;

	FGolemAirZone Zone;
	Zone.ZoneId = NextAirZoneId++;
	Zone.Location = Location;
	Zone.Radius = Radius;
	Zone.TimeRemaining = Lifetime;

	if (AirZoneActorClass && GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = OwnerCharacter;
		Zone.SpawnedActor = GetWorld()->SpawnActor<AActor>(AirZoneActorClass, Location, FRotator::ZeroRotator, SpawnParams);
	}

	ActiveAirZones.Add(Zone);
	OnGolemAirZoneOpened.Broadcast(Zone.ZoneId, Location, Radius, Lifetime);
}

void UGolemBossComponent::ClearAirZones()
{

	TArray<TPair<int32, FVector>> Closed;
	for (int32 i = ActiveAirZones.Num() - 1; i >= 0; --i)
	{
		Closed.Emplace(ActiveAirZones[i].ZoneId, ActiveAirZones[i].Location);
		if (IsValid(ActiveAirZones[i].SpawnedActor)) ActiveAirZones[i].SpawnedActor->Destroy();
		ActiveAirZones.RemoveAt(i);
	}
	for (const TPair<int32, FVector>& C : Closed)
		OnGolemAirZoneClosed.Broadcast(C.Key, C.Value);
}

void UGolemBossComponent::TickAirZones(float DeltaTime)
{
	TArray<TPair<int32, FVector>> Closed;
	for (int32 i = ActiveAirZones.Num() - 1; i >= 0; --i)
	{
		ActiveAirZones[i].TimeRemaining -= DeltaTime;
		if (ActiveAirZones[i].TimeRemaining <= 0.f)
		{
			Closed.Emplace(ActiveAirZones[i].ZoneId, ActiveAirZones[i].Location);
			if (IsValid(ActiveAirZones[i].SpawnedActor)) ActiveAirZones[i].SpawnedActor->Destroy();
			ActiveAirZones.RemoveAt(i);
		}
	}
	for (const TPair<int32, FVector>& C : Closed)
		OnGolemAirZoneClosed.Broadcast(C.Key, C.Value);
}

bool UGolemBossComponent::IsLocationInAirZone(FVector Location) const
{
	for (const FGolemAirZone& Z : ActiveAirZones)
		if (FVector::DistSquared2D(Location, Z.Location) <= Z.Radius * Z.Radius)
			return true;
	return false;
}

bool UGolemBossComponent::HasAirZoneWithTime(float RequiredSeconds) const
{
	for (const FGolemAirZone& Z : ActiveAirZones)
		if (Z.TimeRemaining >= RequiredSeconds)
			return true;
	return false;
}

void UGolemBossComponent::UpdatePhaseFromHealth()
{
	if (Phases.Num() == 0) return;

	if (bToppled) return;

	float Frac = 1.f;
	if (OwnerHealth)
	{
		const float Max = OwnerHealth->GetMaxHealth();
		if (Max > 0.f) Frac = FMath::Clamp(OwnerHealth->GetHealth() / Max, 0.f, 1.f);
	}

	int32 Best = CurrentPhase;
	for (int32 i = 0; i < Phases.Num(); ++i)
		if (Frac <= Phases[i].HealthThreshold && i > Best)
			Best = i;

	if (Best != CurrentPhase)
	{
		const int32 Old = CurrentPhase;
		CurrentPhase = Best;
		const FGolemPhaseConfig& Phase = Phases[CurrentPhase];

		if (State != EGolemAttackState::Idle)
		{
			if (OwnerCharacter)
				if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
					Anim->StopCurrentAction();
			FinishAttack(true);
		}

		float TransitionDelay = FMath::Max(Phase.TransitionPause, 0.f);
		if (Phase.TransitionMontage && OwnerCharacter)
		{
			if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
				if (Anim->PlayActionMontage(Phase.TransitionMontage))
					TransitionDelay += Phase.TransitionMontage->GetPlayLength();
		}

		const UWorld* W = GetWorld();
		const float Now = W ? W->GetTimeSeconds() : 0.f;
		NextAttackReadyTime = FMath::Max(NextAttackReadyTime, Now + TransitionDelay);

		if (Phase.ForcedOpenerAttackId != NAME_None)
			ForceAttack(Phase.ForcedOpenerAttackId);

		OnGolemPhaseChanged.Broadcast(Old, CurrentPhase, Phase.PhaseName);
	}
}

float UGolemBossComponent::GetPhaseCooldownScale() const
{
	return Phases.IsValidIndex(CurrentPhase) ? Phases[CurrentPhase].CooldownScale : 1.f;
}

FGolemWeakPoint* UGolemBossComponent::FindWeakPoint(FName Id)
{
	for (FGolemWeakPoint& WP : WeakPoints) if (WP.Id == Id) return &WP;
	return nullptr;
}

const FGolemWeakPoint* UGolemBossComponent::FindWeakPoint(FName Id) const
{
	for (const FGolemWeakPoint& WP : WeakPoints) if (WP.Id == Id) return &WP;
	return nullptr;
}

bool UGolemBossComponent::IsWeakPointBroken(FName Id) const
{
	const FGolemWeakPoint* WP = FindWeakPoint(Id);
	return WP && WP->bBroken;
}

bool UGolemBossComponent::IsWeakPointVulnerable(FName Id) const
{
	const FGolemWeakPoint* WP = FindWeakPoint(Id);
	return WP && WP->bVulnerable && !WP->bBroken;
}

FVector UGolemBossComponent::GetWeakPointLocation(FName Id) const
{
	const FGolemWeakPoint* WP = FindWeakPoint(Id);
	if (WP && OwnerCharacter)
	{
		if (WP->SocketName != NAME_None)
			if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
				if (Mesh->DoesSocketExist(WP->SocketName))
					return Mesh->GetSocketLocation(WP->SocketName);
		return OwnerCharacter->GetActorLocation();
	}
	return FVector::ZeroVector;
}

void UGolemBossComponent::SetWeakPointVulnerable(FName Id, bool bVulnerable)
{
	if (FGolemWeakPoint* WP = FindWeakPoint(Id))
		if (!WP->bBroken) WP->bVulnerable = bVulnerable;
}

bool UGolemBossComponent::HitWeakPoint(FName Id, float Damage, AActor* Instigator)
{
	FGolemWeakPoint* WP = FindWeakPoint(Id);
	if (!WP || WP->bBroken || !WP->bVulnerable || Damage <= 0.f) return false;

	if (WP->Kind == EGolemWeakPointKind::Head)
	{
		const float Crit = Damage * HeadCritMultiplier;
		DealDamageToBoss(Crit, Instigator);
		OnGolemWeakPointHit.Broadcast(Id, Crit, 0.f);
		OnGolemCriticalHit.Broadcast(Crit, Id);
		return true;
	}

	WP->CurrentHealth -= Damage;
	OnGolemWeakPointHit.Broadcast(Id, Damage, FMath::Max(WP->CurrentHealth, 0.f));
	if (WP->CurrentHealth <= 0.f)
	{
		WP->bBroken = true;
		WP->bVulnerable = false;
		OnGolemWeakPointBroken.Broadcast(Id);
		CheckTopple();
	}
	return true;
}

bool UGolemBossComponent::HitArm(FName ArmId, float Damage, AActor* Instigator)
{
	FGolemWeakPoint* WP = FindWeakPoint(ArmId);
	if (!WP || WP->Kind != EGolemWeakPointKind::Arm || Damage <= 0.f) return false;

	if (!bArmsAlwaysHittable && !WP->bVulnerable) return false;

	float Mult = WP->bBroken ? FMath::Max(1.f, BrokenArmDamageMultiplier) : 1.f;
	if (!WP->bVulnerable && !WP->bBroken) Mult *= FMath::Clamp(UnexposedArmDamageMultiplier, 0.f, 1.f);
	const float Dealt = Damage * Mult;
	DealDamageToBoss(Dealt, Instigator);

	const float Remaining = WP->bBroken ? 0.f : (float)FMath::Max(0, ArmCrystalHitsToBreak - WP->CrystalHits);
	OnGolemWeakPointHit.Broadcast(ArmId, Dealt, Remaining);

	const bool bMayChip = WP->bVulnerable || !bArmCrystalsChipOnlyWhenExposed;
	if (!WP->bBroken && bMayChip && ++WP->CrystalHits >= ArmCrystalHitsToBreak)
	{
		WP->bBroken = true;
		WP->bVulnerable = false;
		OnGolemWeakPointBroken.Broadcast(ArmId);
		CheckTopple();
	}
	return true;
}

bool UGolemBossComponent::RouteBodyHit(float Damage, AActor* Instigator)
{
	if (Damage <= 0.f) return false;

	if (Instigator && OwnerCharacter && !OwnerCharacter->GetCurrentTarget())
		OwnerCharacter->OnPerceiveTarget(Instigator);

	if (bToppled)
	{
		for (const FGolemWeakPoint& WP : WeakPoints)
			if (WP.Kind == EGolemWeakPointKind::Head && WP.bVulnerable && !WP.bBroken)
				if (HitWeakPoint(WP.Id, Damage, Instigator)) return true;
	}

	const FVector From = Instigator ? Instigator->GetActorLocation()
		: (OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector);
	FName BestArm = NAME_None;
	float BestDistSq = TNumericLimits<float>::Max();
	for (const FGolemWeakPoint& WP : WeakPoints)
	{
		if (WP.Kind != EGolemWeakPointKind::Arm) continue;
		const float DistSq = FVector::DistSquared(GetWeakPointLocation(WP.Id), From);
		if (DistSq < BestDistSq) { BestDistSq = DistSq; BestArm = WP.Id; }
	}

	if (BestArm == NAME_None)
	{
		DealDamageToBoss(Damage * UnexposedArmDamageMultiplier, Instigator);
		if (!bActivated && bAutoActivateOnTarget) ActivateBoss();
		return true;
	}

	const bool bApplied = HitArm(BestArm, Damage, Instigator);
	if (bApplied && !bActivated && bAutoActivateOnTarget) ActivateBoss();
	return bApplied;
}

void UGolemBossComponent::ExposeArmWeakPoints(bool bExpose, FName AttackId)
{
	ExposingAttackId = bExpose ? AttackId : NAME_None;
	for (FGolemWeakPoint& WP : WeakPoints)
		if (WP.Kind == EGolemWeakPointKind::Arm && !WP.bBroken)
			WP.bVulnerable = bExpose;
	OnGolemWeakPointsExposed.Broadcast(bExpose, AttackId);
}

void UGolemBossComponent::OnExposeLingerElapsed()
{
	if (!bToppled) ExposeArmWeakPoints(false, ExposingAttackId);
}

void UGolemBossComponent::CheckTopple()
{
	bool bAnyArm = false, bAllArmsBroken = true;
	for (const FGolemWeakPoint& WP : WeakPoints)
		if (WP.Kind == EGolemWeakPointKind::Arm)
		{
			bAnyArm = true;
			if (!WP.bBroken) bAllArmsBroken = false;
		}
	if (bAnyArm && bAllArmsBroken) Topple();
}

void UGolemBossComponent::ForceTopple() { Topple(); }

void UGolemBossComponent::Topple()
{
	if (bToppled) return;
	bToppled = true;

	if (OwnerCharacter)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->StopCurrentAction();
	if (State != EGolemAttackState::Idle) FinishAttack(true);
	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(ExposeLingerTimerHandle);

	for (FGolemWeakPoint& WP : WeakPoints)
		if (WP.Kind == EGolemWeakPointKind::Head) WP.bVulnerable = true;

	if (UWorld* W = GetWorld())
		W->GetTimerManager().SetTimer(ToppleTimerHandle, this, &UGolemBossComponent::OnToppleElapsed, ToppleDuration, false);

	OnGolemToppled.Broadcast(ToppleDuration);

	if (bSlamPlantsArenaCrystals) SpawnBigCrystal();
}

void UGolemBossComponent::OnToppleElapsed() { EndTopple(); }

void UGolemBossComponent::EndTopple()
{
	if (!bToppled) return;
	bToppled = false;
	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(ToppleTimerHandle);

	EndBigCrystal();
	ClearArenaCrystals();

	for (FGolemWeakPoint& WP : WeakPoints)
	{
		if (WP.Kind == EGolemWeakPointKind::Head) WP.bVulnerable = false;
		if (WP.Kind == EGolemWeakPointKind::Arm && bResetArmsOnRecover)
		{
			WP.CurrentHealth = WP.Health;
			WP.bBroken = false;
			WP.bVulnerable = false;
			WP.CrystalHits = 0;
		}
	}

	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	NextAttackReadyTime = Now + GlobalCooldown;

	OnGolemRecoverFromTopple.Broadcast();
}

void UGolemBossComponent::PlantArenaCrystals()
{
	ClearArenaCrystals();
	UWorld* const W = GetWorld();
	for (const FVector& P : CurrentTelegraph.ImpactPoints)
	{
		FGolemArenaCrystal C;
		C.Id = NextArenaCrystalId++;
		C.Location = P;
		C.HitsRemaining = FMath::Max(1, ArenaCrystalHitsToBreak);
		C.bDestroyed = false;

		if (W && *ArenaCrystalActorClass)
		{
			FActorSpawnParameters SP;
			SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SP.Owner = OwnerCharacter;
			if (AGolemCrystal* Actor = W->SpawnActor<AGolemCrystal>(ArenaCrystalActorClass, P, FRotator::ZeroRotator, SP))
			{
				Actor->InitAsArenaCrystal(this, C.Id);
				C.SpawnedActor = Actor;
			}
		}

		ArenaCrystals.Add(C);
		OnGolemArenaCrystalSpawned.Broadcast(C.Id, C.Location);
	}
}

void UGolemBossComponent::ClearArenaCrystals()
{
	if (ArenaCrystals.Num() == 0) return;
	for (const FGolemArenaCrystal& C : ArenaCrystals)
		ShatterCrystalActor(C.SpawnedActor);
	ArenaCrystals.Reset();
	OnGolemArenaCrystalsCleared.Broadcast();
}

bool UGolemBossComponent::AllArenaCrystalsDestroyed() const
{
	if (ArenaCrystals.Num() == 0) return false;
	for (const FGolemArenaCrystal& C : ArenaCrystals)
		if (!C.bDestroyed) return false;
	return true;
}

bool UGolemBossComponent::HitArenaCrystal(int32 CrystalId, float PlayerDamage, AActor* Instigator)
{
	FGolemArenaCrystal* C = ArenaCrystals.FindByPredicate([CrystalId](const FGolemArenaCrystal& X){ return X.Id == CrystalId; });
	if (!C || C->bDestroyed) return false;

	C->HitsRemaining = FMath::Max(0, C->HitsRemaining - 1);
	OnGolemArenaCrystalHit.Broadcast(C->Id, C->HitsRemaining);

	if (C->HitsRemaining <= 0)
	{

		const int32 Id = C->Id;
		const FVector Loc = C->Location;
		C->bDestroyed = true;
		ShatterCrystalActor(C->SpawnedActor);
		C->SpawnedActor = nullptr;

		DealDamageToBoss(FMath::Max(0.f, PlayerDamage) * ArenaCrystalBreakDamageMult, Instigator);
		OnGolemArenaCrystalDestroyed.Broadcast(Id, Loc);

		if (OwnerCharacter && !OwnerCharacter->IsDead() && AllArenaCrystalsDestroyed())
		{
			ArenaCrystals.Reset();
			Topple();
		}
	}
	else if (AGolemCrystal* Cr = Cast<AGolemCrystal>(C->SpawnedActor))
	{
		Cr->NotifyDamaged(C->HitsRemaining, FMath::Max(1, ArenaCrystalHitsToBreak));
	}
	return true;
}

void UGolemBossComponent::SpawnBigCrystal()
{
	bBigCrystalActive = true;
	BigCrystalHitsRemaining = FMath::Max(1, BigCrystalHitsToBreak);

	const FVector Loc = ProjectToGround(ResolveArenaCentre());

	if (UWorld* const W = GetWorld())
	{
		if (IsValid(BigCrystalActor)) BigCrystalActor->Destroy();
		BigCrystalActor = nullptr;
		if (*BigCrystalActorClass)
		{
			FActorSpawnParameters SP;
			SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SP.Owner = OwnerCharacter;
			if (AGolemCrystal* Actor = W->SpawnActor<AGolemCrystal>(BigCrystalActorClass, Loc, FRotator::ZeroRotator, SP))
			{
				Actor->InitAsBigCrystal(this);
				BigCrystalActor = Actor;
			}
		}
	}

	OnGolemBigCrystalSpawned.Broadcast(Loc);
}

void UGolemBossComponent::EndBigCrystal()
{
	if (!bBigCrystalActive) return;
	bBigCrystalActive = false;
	BigCrystalHitsRemaining = 0;
	ShatterCrystalActor(BigCrystalActor);
	BigCrystalActor = nullptr;
}

bool UGolemBossComponent::HitBigCrystal(float PlayerDamage, AActor* Instigator)
{
	if (!bBigCrystalActive) return false;

	BigCrystalHitsRemaining = FMath::Max(0, BigCrystalHitsRemaining - 1);
	OnGolemBigCrystalHit.Broadcast(BigCrystalHitsRemaining);

	if (BigCrystalHitsRemaining <= 0)
	{
		const float Chunk = OwnerHealth ? OwnerHealth->GetMaxHealth() * FMath::Clamp(BigCrystalHealthFraction, 0.f, 1.f) : 0.f;
		DealDamageToBoss(Chunk, Instigator);
		OnGolemBigCrystalBroken.Broadcast(Chunk);
		EndBigCrystal();

		if (UWorld* W = GetWorld())
			W->GetTimerManager().SetTimer(ToppleTimerHandle, this, &UGolemBossComponent::OnToppleElapsed,
				FMath::Max(0.5f, BigCrystalStunDuration), false);
	}
	else if (AGolemCrystal* Cr = Cast<AGolemCrystal>(BigCrystalActor))
	{
		Cr->NotifyDamaged(BigCrystalHitsRemaining, FMath::Max(1, BigCrystalHitsToBreak));
	}
	return true;
}

void UGolemBossComponent::ShowBossUIAndMusic()
{
	UWorld* const W = GetWorld();
	if (!W) return;

	if (!BossBar && *BossBarWidgetClass)
	{
		if (APlayerController* PC = W->GetFirstPlayerController())
		{
			BossBar = CreateWidget<UGolemBossBarWidget>(PC, BossBarWidgetClass);
			if (BossBar)
			{
				BossBar->AddToViewport(BossBarZOrder);
				BossBar->SetBossName(BossDisplayName);
				if (OwnerHealth) BossBar->SetBossHealth(OwnerHealth->GetHealth(), OwnerHealth->GetMaxHealth());
			}
		}
	}

	if (CombatMusic)
	{
		if (CombatMusicComp) { CombatMusicComp->Stop(); CombatMusicComp = nullptr; }
		CombatMusicComp = UGameplayStatics::CreateSound2D(this, CombatMusic, 1.f, 1.f, 0.f, nullptr, false, false);
		if (CombatMusicComp)
		{
			if (CombatMusicFadeIn > 0.f) CombatMusicComp->FadeIn(CombatMusicFadeIn);
			else                         CombatMusicComp->Play();
		}
	}
}

void UGolemBossComponent::HideBossUIAndMusic()
{
	if (BossBar)
	{
		BossBar->RemoveFromParent();
		BossBar = nullptr;
	}
	if (CombatMusicComp)
	{
		if (CombatMusicFadeOut > 0.f) CombatMusicComp->FadeOut(CombatMusicFadeOut, 0.f);
		else                          CombatMusicComp->Stop();
		CombatMusicComp = nullptr;
	}
}

void UGolemBossComponent::HandleBossHealthChanged(float NewHealth, float MaxHealth)
{
	if (BossBar) BossBar->SetBossHealth(NewHealth, MaxHealth);
}

void UGolemBossComponent::DrawTelegraphDebug(const FGolemTelegraph& T, float Lifetime) const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* W = GetWorld();
	if (!W) return;
	const FColor Warn = FColor::Yellow;

	switch (T.Shape)
	{
	case EGolemHazardShape::RadialSlam:
	case EGolemHazardShape::ProjectileImpact:
	case EGolemHazardShape::Bombardment:
		for (int32 i = 0; i < T.ImpactPoints.Num(); ++i)
			DrawFlatCircle(W, T.ImpactPoints[i], T.ImpactRadii.IsValidIndex(i) ? T.ImpactRadii[i] : 200.f, Warn, Lifetime, 6.f);
		break;
	case EGolemHazardShape::GroundFissures:
	{
		const FVector GroundC = ProjectToGround(ResolveArenaCentre());
		DrawFlatCircle(W, GroundC, GetArenaRadius(), FColor::Red, Lifetime, 8.f);
		for (const FVector& P : T.ImpactPoints)
			DrawDebugSphere(W, P, 70.f, 10, Warn, false, Lifetime);
		DrawDebugString(W, GroundC + FVector(0, 0, 180.f), TEXT("FISSURES - FLY!"), nullptr, FColor::Red, Lifetime);
		break;
	}
	case EGolemHazardShape::SweepLine:
	{
		const FVector Perp = YawRotate(T.LineDirection, 90.f);
		const FVector PA = T.LineCentre + Perp * (T.LineLength * 0.5f);
		const FVector PB = T.LineCentre - Perp * (T.LineLength * 0.5f);
		DrawDebugLine(W, PA, PB, Warn, false, Lifetime, 0, 8.f);
		DrawDebugDirectionalArrow(W, T.LineCentre, T.LineCentre + T.LineDirection * (GetArenaRadius() * 2.f), 140.f, Warn, false, Lifetime, 0, 6.f);
		break;
	}
	case EGolemHazardShape::BeamSweep:
		for (const FGolemBeamSegment& Beam : T.Beams)
			DrawDebugLine(W, Beam.Origin, Beam.End, Warn, false, Lifetime, 0, 5.f);
		break;
	}

	if (T.AirZoneRadius > 0.f)
		DrawFlatCircle(W, T.AirZoneLocation, T.AirZoneRadius, FColor::Cyan, Lifetime, 6.f);
#endif
}

void UGolemBossComponent::DrawImpactDebug(const FVector& Center, float Radius, bool bAirborneIsSafe) const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* W = GetWorld();
	if (!W) return;
	const FColor Col = bAirborneIsSafe ? FColor(0, 160, 255) : FColor::Orange;
	DrawFlatCircle(W, Center, Radius, Col, DebugImpactLinger, 7.f);
	DrawDebugSphere(W, Center, FMath::Min(Radius, 120.f), 12, Col, false, DebugImpactLinger);
#endif
}

void UGolemBossComponent::DrawLiveDebug(float Lifetime) const
{
#if ENABLE_DRAW_DEBUG
	const UWorld* W = GetWorld();
	if (!W || !OwnerCharacter) return;

	DrawFlatCircle(W, ResolveArenaCentre(), GetArenaRadius(), FColor::Green, Lifetime, 3.f);

	for (const FGolemAirZone& Z : ActiveAirZones)
	{
		DrawFlatCircle(W, Z.Location, Z.Radius, FColor::Cyan, Lifetime, 5.f);
		DrawDebugCylinder(W, Z.Location, Z.Location + FVector(0, 0, 1500.f), Z.Radius, 16, FColor::Cyan, false, Lifetime, 0, 2.f);
	}

	if (bDrawDebugText)
	{
		const TCHAR* StateStr =
			State == EGolemAttackState::Windup ? TEXT("Windup") :
			State == EGolemAttackState::Active ? TEXT("Active") :
			State == EGolemAttackState::Recovery ? TEXT("Recovery") : TEXT("Idle");
		const FString Txt = FString::Printf(TEXT("Golem  state=%s  attack=%s  phase=%d  airzones=%d"),
			StateStr, *GetCurrentAttackId().ToString(), CurrentPhase, ActiveAirZones.Num());
		DrawDebugString(W, OwnerCharacter->GetActorLocation() + FVector(0, 0, 320.f), Txt, nullptr, FColor::White, Lifetime);
	}
#endif
}

FName UGolemBossComponent::GetCurrentAttackId() const
{
	return Attacks.IsValidIndex(CurrentAttackIndex) ? Attacks[CurrentAttackIndex].AttackId : NAME_None;
}

FName UGolemBossComponent::GetPendingForcedAttackId() const
{
	return Attacks.IsValidIndex(PendingForcedAttack) ? Attacks[PendingForcedAttack].AttackId : NAME_None;
}

void UGolemBossComponent::HandleOwnerDamaged(AActor* )
{
	if (!bActivated && bAutoActivateOnTarget) ActivateBoss();
}

void UGolemBossComponent::HandleOwnerDied()
{
	if (State != EGolemAttackState::Idle) FinishAttack(true);
	ClearAirZones();
	EndBigCrystal();
	ClearArenaCrystals();
	bActivated = false;
	bToppled = false;
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(ToppleTimerHandle);
		W->GetTimerManager().ClearTimer(ExposeLingerTimerHandle);
	}
	OnGolemDefeated.Broadcast();
}
