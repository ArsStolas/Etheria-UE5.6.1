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
#include "Components/Characters/HealthComponent.h"
#include "Engine/StaticMesh.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Components/SkeletalMeshComponent.h"
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
	PrimaryComponentTick.bStartWithTickEnabled = false; // tick is turned on only during a live moving hazard
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
	}

	AttackReadyTimes.Init(0.f, Attacks.Num());

	for (FGolemWeakPoint& WP : WeakPoints)
	{
		WP.CurrentHealth = WP.Health;
		WP.bBroken = false;
		WP.bVulnerable = false;
	}

	if (UWorld* W = GetWorld())
		W->GetTimerManager().SetTimer(BrainTimerHandle, this, &UGolemBossComponent::BrainTick,
			FMath::Max(0.02f, BrainInterval), /*bLoop=*/true, /*FirstDelay=*/0.1f);
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
	ClearAirZones();
	if (HeldRock) { HeldRock->Destroy(); HeldRock = nullptr; }
	Super::EndPlay(EndPlayReason);
}

void UGolemBossComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Only ever enabled during a live hazard — drive the moving hazard frame-by-frame, nothing else.
	if (OwnerCharacter && State == EGolemAttackState::Active)
		TickActive(DeltaTime);
}

/* ═══════════ Brain (timer heartbeat) ═══════════ */

void UGolemBossComponent::BrainTick()
{
	if (!OwnerCharacter) return;
	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;

	TickAirZones(BrainInterval);
	TickContactRepulsion();

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
		UpdatePhaseFromHealth();
		if (State == EGolemAttackState::Idle && !bToppled) // a toppled boss neither faces nor attacks
		{
			TickFacing(BrainInterval);
			if (Now >= NextAttackReadyTime)
			{
				AActor* T = OwnerCharacter->GetCurrentTarget();
				if ((T && !OwnerCharacter->IsTargetDeadOrInvalid(T)) || PendingForcedAttack >= 0)
				{
					const int32 Idx = SelectNextAttack();
					if (Idx >= 0) BeginAttack(Idx);
				}
			}
		}
	}

	if (bDrawDebugHazards) DrawLiveDebug(BrainInterval);
}

/* ═══════════ Activation ═══════════ */

void UGolemBossComponent::ActivateBoss()
{
	if (bActivated || !OwnerCharacter) return;
	bActivated = true;
	CurrentPhase = 0;
	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	NextAttackReadyTime = Now + FMath::FRandRange(0.4f, 0.4f + GlobalCooldownRandom);

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
	bActivated = false;
}

/* ═══════════ Scripted control ═══════════ */

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
		if (Rem > 0.f) Travel = Rem; // land exactly when the strike fires
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
		Rock->Launch(Origin, Target, Travel, RockThrowArcHeight); // detach from the hand and throw it
	}
	else
	{
		Rock = SpawnFallingRock(Origin, Target, Travel, RockThrowArcHeight); // BP-rock path / no held mesh
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

/* ═══════════ Sequence (timer-driven) ═══════════ */

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

	ACharacter* Player = Cast<ACharacter>(OwnerCharacter->GetCurrentTarget());
	if (!Player) return;

	FVector Out = Player->GetActorLocation() - OwnerCharacter->GetActorLocation();
	Out.Z = 0.f;
	const float Dist = Out.Size();
	if (Dist <= 1.f || Dist >= RepulsionRadius) return; // not touching the body

	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	if (Now < NextRepulsionTime) return; // brief cooldown so the player isn't pinned
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

	BuildTelegraph(Cfg, CurrentTelegraph);
	BombardImpactFired.Init(false, CurrentTelegraph.ImpactPoints.Num());
	BombardLaunched.Init(false, CurrentTelegraph.ImpactPoints.Num());

	if (Cfg.Montage)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->PlayActionMontage(Cfg.Montage);

	OnGolemAttackBegin.Broadcast(Cfg.AttackId, Cfg.Shape);
	OnGolemTelegraph.Broadcast(CurrentTelegraph);

	// Arm the wind-up timer FIRST so the rock-release path can read the time remaining until the strike.
	if (Cfg.WindupDuration > 0.f)
		if (UWorld* W = GetWorld())
			W->GetTimerManager().SetTimer(WindupTimerHandle, this, &UGolemBossComponent::OnWindupElapsed, Cfg.WindupDuration, false);

	// Thrown rock leaves the HAND at the release moment (RockReleaseTime, or an AnimNotify calling LaunchRockNow),
	// then flies to the impact — landing exactly when the strike fires.
	bRockLaunched = false;
	if (Cfg.Shape == EGolemHazardShape::ProjectileImpact)
	{
		SpawnHeldRock(); // put the rock in the hand for the whole wind-up
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

	// Ground warning decals. The thrown rock & avalanche boulders grow their OWN decal as they approach
	// (ApplyRockImpactDecal); here we only handle the rock-less attacks.
	if (bSpawnTelegraphDecals)
	{
		if (Cfg.Shape == EGolemHazardShape::RadialSlam)
		{
			if (CurrentTelegraph.ImpactPoints.Num() > 0)
				SpawnTelegraphDecal(CurrentTelegraph.ImpactPoints[0], Cfg.ImpactRadius, Cfg.WindupDuration + 0.4f);
		}
		else if (Cfg.Shape == EGolemHazardShape::GroundFissures)
		{
			SpawnTelegraphDecal(ResolveArenaCentre(), GetArenaRadius(), Cfg.WindupDuration + Cfg.ActiveDuration + 0.4f);
		}
	}

	if (bDrawDebugHazards) DrawTelegraphDebug(CurrentTelegraph, FMath::Max(Cfg.WindupDuration, 0.05f));

	if (Cfg.WindupDuration <= 0.f) OnWindupElapsed(); // zero wind-up: strike now, after the rock was set up above
}

void UGolemBossComponent::OnWindupElapsed()
{
	EnterActive();
}

void UGolemBossComponent::EnterActive()
{
	if (!Attacks.IsValidIndex(CurrentAttackIndex)) { FinishAttack(false); return; }
	const FGolemAttackConfig& Cfg = Attacks[CurrentAttackIndex];

	// Safety: if the throw's release never fired (release time > wind-up, missing notify), launch it now.
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

	// Arms slam down -> expose the arm crystals so the player can climb and break them.
	if (Cfg.bExposesArmWeakPoints)
	{
		if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(ExposeLingerTimerHandle);
		ExposeArmWeakPoints(true, Cfg.AttackId);
	}

	if (Cfg.ActiveDuration > 0.f)
		SetComponentTickEnabled(true); // per-frame ONLY while the live hazard runs
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
	if (!Attacks.IsValidIndex(CurrentAttackIndex)) { FinishAttack(false); return; }
	const FGolemAttackConfig& Cfg = Attacks[CurrentAttackIndex];

	// Flush any bombardment boulders the per-frame scheduler didn't reach (last index / low-FPS frame skips).
	if (Cfg.Shape == EGolemHazardShape::Bombardment)
	{
		const int32 Num = CurrentTelegraph.ImpactPoints.Num();
		for (int32 i = 0; i < Num; ++i)
		{
			if (BombardImpactFired.IsValidIndex(i) && BombardImpactFired[i]) continue;
			const FVector P = CurrentTelegraph.ImpactPoints[i];
			ApplyRadialBurst(P, Cfg.ImpactRadius, Cfg.Damage, Cfg.bAirborneIsSafe, Cfg.KnockbackForce, Cfg.AttackId);
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

	if (HeldRock) { HeldRock->Destroy(); HeldRock = nullptr; } // unreleased rock (attack interrupted mid-wind-up)

	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;

	if (Attacks.IsValidIndex(CurrentAttackIndex))
	{
		const FGolemAttackConfig& Cfg = Attacks[CurrentAttackIndex];
		if (AttackReadyTimes.IsValidIndex(CurrentAttackIndex))
			AttackReadyTimes[CurrentAttackIndex] = Now + Cfg.Cooldown * GetPhaseCooldownScale();
		OnGolemAttackEnd.Broadcast(Cfg.AttackId, bInterrupted);

		// Keep the arm crystals reachable a little longer after the slam ends, then retract them.
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

/* ═══════════ Selection ═══════════ */

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
		if (Attacks[i].bOpensAirZone) return i; // even on cooldown, so the unavoidable always gets its escape
	return -1;
}

int32 UGolemBossComponent::SelectNextAttack()
{
	auto MarkReady = [this](int32 i) { if (AttackReadyTimes.IsValidIndex(i)) AttackReadyTimes[i] = 0.f; };

	auto WeightedPick = [this](bool bExcludeRequiresAirZone) -> int32
	{
		float Total = 0.f;
		TArray<TPair<int32, float>> Candidates;
		for (int32 i = 0; i < Attacks.Num(); ++i)
		{
			if (!IsAttackUsable(i)) continue;
			if (bExcludeRequiresAirZone && Attacks[i].bRequiresAirZoneEscape) continue;
			// Floor the anti-repeat penalty so the just-used move can never be eliminated entirely (anti-softlock).
			float Wt = Attacks[i].SelectionWeight;
			if (i == LastAttackIndex) Wt = FMath::Max(Wt * RepeatPenalty, KINDA_SMALL_NUMBER);
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

	// 1) Forced attack (BP ForceAttack, or an unavoidable queued behind a rock-throw). Re-validate the air-zone guarantee.
	if (PendingForcedAttack >= 0)
	{
		const int32 Forced = PendingForcedAttack;
		PendingForcedAttack = -1;
		if (Attacks.IsValidIndex(Forced))
		{
			if (Attacks[Forced].bRequiresAirZoneEscape && !HasActiveAirZone())
			{
				const int32 Opener = FindAirZoneOpener();
				if (Opener >= 0)
				{
					MarkReady(Opener);
					PendingForcedAttack = Forced; // re-queue behind a fresh opener
					return Opener;
				}
				// No opener at all -> fall through to a zone-free pick.
			}
			else
			{
				MarkReady(Forced);
				return Forced;
			}
		}
	}

	// 2) Weighted-random pick among usable attacks.
	const int32 Chosen = WeightedPick(false);
	if (Chosen < 0) return -1;

	// 3) Air-zone gating: an unavoidable ground attack runs only while a flight zone is open.
	if (Attacks[Chosen].bRequiresAirZoneEscape && !HasActiveAirZone())
	{
		const int32 Opener = FindAirZoneOpener();
		if (Opener >= 0)
		{
			MarkReady(Opener);
			PendingForcedAttack = Chosen;
			return Opener;
		}
		const int32 ZoneFree = WeightedPick(true);
		return ZoneFree >= 0 ? ZoneFree : Chosen; // no opener & nothing zone-free -> fire the gated move (anti-softlock)
	}

	return Chosen;
}

/* ═══════════ Telegraph / geometry ═══════════ */

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
		// Sideways arm sweep only: travel along the Golem's RIGHT axis (left<->right), never behind its back
		// or straight front-to-back — the arms can't reach there.
		FVector Side = OwnerCharacter ? OwnerCharacter->GetActorRightVector() : FVector::RightVector;
		Side.Z = 0.f;
		Side = Side.GetSafeNormal();
		if (Side.IsNearlyZero()) Side = FVector::RightVector;
		if (FMath::RandBool()) Side = -Side; // left->right or right->left

		Out.LineDirection = Side;
		Out.LineCentre = Centre - Side * Radius;
		Out.LineLength = Cfg.SweepLength;
		Out.LineThickness = Cfg.SweepThickness;
		break;
	}
	case EGolemHazardShape::GroundFissures:
	{
		const FVector Axis = YawRotate(FVector::ForwardVector, FMath::FRandRange(0.f, 360.f));
		Out.ImpactPoints.Add(Centre + Axis * Radius);
		Out.ImpactPoints.Add(Centre - Axis * Radius);
		Out.ImpactRadii.Add(Cfg.ImpactRadius);
		Out.ImpactRadii.Add(Cfg.ImpactRadius);
		break;
	}
	case EGolemHazardShape::BeamSweep:
	{
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
			// Land time = Lead..(<Window); each boulder gets `Lead` of fall, none lands exactly at Window (never skipped).
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

	switch (Cfg.Shape)
	{
	case EGolemHazardShape::RadialSlam:
	case EGolemHazardShape::ProjectileImpact:
	{
		const FVector C = CurrentTelegraph.ImpactPoints.Num() > 0 ? CurrentTelegraph.ImpactPoints[0] : ResolveArenaCentre();
		ApplyRadialBurst(C, Cfg.ImpactRadius, Cfg.Damage, Cfg.bAirborneIsSafe, Cfg.KnockbackForce, Cfg.AttackId);
		Broadcast(C, Cfg.ImpactRadius, 0, 1);
		if (bDrawDebugHazards) DrawImpactDebug(C, Cfg.ImpactRadius, Cfg.bAirborneIsSafe);
		break;
	}
	case EGolemHazardShape::GroundFissures:
	{
		ApplyRadialBurst(ResolveArenaCentre(), GetArenaRadius(), Cfg.Damage, /*bAirborneIsSafe=*/true, Cfg.KnockbackForce, Cfg.AttackId, Cfg.MinSafeAltitude);
		for (int32 i = 0; i < CurrentTelegraph.ImpactPoints.Num(); ++i)
			Broadcast(CurrentTelegraph.ImpactPoints[i], Cfg.ImpactRadius, i, CurrentTelegraph.ImpactPoints.Num());
		break;
	}
	case EGolemHazardShape::SweepLine:
		Broadcast(CurrentTelegraph.LineCentre, CurrentTelegraph.LineThickness, 0, 1); // wall's initial slam edge
		break;
	case EGolemHazardShape::BeamSweep:
	{
		const FVector Origin = CurrentTelegraph.Beams.Num() > 0 ? CurrentTelegraph.Beams[0].Origin : ResolveArenaCentre();
		Broadcast(Origin, Cfg.BeamWidth, 0, 1); // laser fire moment
		break;
	}
	case EGolemHazardShape::Bombardment:
	{
		if (ActiveDurationCache <= KINDA_SMALL_NUMBER) // no active window to schedule across -> all at once
		{
			const int32 Num = CurrentTelegraph.ImpactPoints.Num();
			for (int32 i = 0; i < Num; ++i)
			{
				const FVector P = CurrentTelegraph.ImpactPoints[i];
				ApplyRadialBurst(P, Cfg.ImpactRadius, Cfg.Damage, Cfg.bAirborneIsSafe, Cfg.KnockbackForce, Cfg.AttackId);
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

	const float Interval = FMath::Max(0.02f, Cfg.DamageInterval); // use-site clamp (UPROPERTY ClampMin is UI-only)
	DamageTickAccum += DeltaTime;
	bool bDamageTick = false;
	if (DamageTickAccum >= Interval) { bDamageTick = true; DamageTickAccum -= Interval; }
	if (bForceFinal) bDamageTick = true; // always land a final pass at the end position

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
			ApplyRingBurst(C, Inner, Outer, Cfg.ShockwaveDamage, /*bAirborneIsSafe=*/true, Cfg.KnockbackForce, Cfg.AttackId);
		S.RingRadius = Outer;
#if ENABLE_DRAW_DEBUG
		if (bDrawDebugHazards) DrawFlatCircle(GetWorld(), C, Outer, FColor::Orange, -1.f, 10.f);
#endif
		break;
	}
	case EGolemHazardShape::GroundFissures:
	{
		if (bDamageTick)
			ApplyRadialBurst(ResolveArenaCentre(), GetArenaRadius(), Cfg.Damage, /*bAirborneIsSafe=*/true, Cfg.KnockbackForce, Cfg.AttackId, Cfg.MinSafeAltitude);
#if ENABLE_DRAW_DEBUG
		if (bDrawDebugHazards) DrawFlatCircle(GetWorld(), ResolveArenaCentre(), GetArenaRadius(), FColor::Red, -1.f, 8.f);
#endif
		break;
	}
	case EGolemHazardShape::SweepLine:
	{
		const FVector Dir = CurrentTelegraph.LineDirection;
		const FVector Start = ResolveArenaCentre() - Dir * GetArenaRadius();
		const FVector Cur = Start + Dir * (Alpha * 2.f * GetArenaRadius());
		const FVector Perp = YawRotate(Dir, 90.f);
		const FVector A = Cur + Perp * (Cfg.SweepLength * 0.5f);
		const FVector B = Cur - Perp * (Cfg.SweepLength * 0.5f);
		if (bDamageTick)
			ApplyLineDamage(A, B, Cfg.SweepThickness, Cfg.Damage, /*bAirborneIsSafe=*/true, /*b3D=*/false, Cfg.KnockbackForce, Cfg.AttackId, Cfg.MinSafeAltitude);
		S.LineCentre = Cur;
		S.LineDirection = Dir;
#if ENABLE_DRAW_DEBUG
		if (bDrawDebugHazards) DrawDebugLine(GetWorld(), A, B, FColor::Red, false, -1.f, 0, 12.f);
#endif
		break;
	}
	case EGolemHazardShape::BeamSweep:
	{
		const float Rotate = FMath::Lerp(-Cfg.BeamSweepAngle * 0.5f, Cfg.BeamSweepAngle * 0.5f, Alpha);
		BuildBeams(Cfg, Rotate, S.Beams);
		if (bDamageTick)
			for (const FGolemBeamSegment& Beam : S.Beams)
				ApplyLineDamage(Beam.Origin, Beam.End, Cfg.BeamWidth, Cfg.Damage, /*bAirborneIsSafe=*/false, /*b3D=*/true, Cfg.KnockbackForce, Cfg.AttackId);
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

			// Launch (fall start): spawn the boulder above its target so it lands on the strike.
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
				AGolemFallingRock* Rock = SpawnFallingRock(L.Origin, L.Target, L.TravelTime, 0.f); // straight fall
				ApplyRockImpactDecal(Rock, P, Cfg.ImpactRadius, FMath::Max(Lead, 0.1f) + 0.3f);
			}

			// Impact.
			if (BombardImpactFired.IsValidIndex(i) && !BombardImpactFired[i] && ElapsedActive >= When)
			{
				BombardImpactFired[i] = true;
				ApplyRadialBurst(P, Cfg.ImpactRadius, Cfg.Damage, Cfg.bAirborneIsSafe, Cfg.KnockbackForce, Cfg.AttackId);

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

void UGolemBossComponent::SpawnTelegraphDecal(const FVector& Location, float RadiusXY, float LifeSpan) const
{
	if (!TelegraphDecalMaterial || RadiusXY <= 0.f) return;
	UWorld* W = GetWorld();
	if (!W) return;

	// Box projecting straight down onto the floor (matches the project's ground-decal convention).
	const FVector DecalSize(DecalProjectionDepth, RadiusXY, RadiusXY);
	UGameplayStatics::SpawnDecalAtLocation(W, TelegraphDecalMaterial, DecalSize, Location, FRotator(-90.f, 0.f, 0.f), FMath::Max(LifeSpan, 0.1f));
}

AGolemFallingRock* UGolemBossComponent::SpawnFallingRock(const FVector& Origin, const FVector& Target, float TravelTime, float ArcHeight)
{
	if (!bSpawnRockMeshes) return nullptr;
	UWorld* W = GetWorld();
	if (!W) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = OwnerCharacter;

	if (RockActorClass) // advanced: designer's own rock actor handles its own movement
	{
		W->SpawnActor<AActor>(RockActorClass, Origin, (Target - Origin).Rotation(), SpawnParams);
		return nullptr; // not our built-in rock, so no growing-decal control
	}

	if (RockMeshes.Num() == 0) return nullptr;
	UStaticMesh* Mesh = RockMeshes[FMath::RandRange(0, RockMeshes.Num() - 1)]; // each rock picks one at random
	if (!Mesh) return nullptr;

	AGolemFallingRock* Rock = W->SpawnActor<AGolemFallingRock>(AGolemFallingRock::StaticClass(), Origin, FRotator::ZeroRotator, SpawnParams);
	if (Rock)
	{
		Rock->Configure(Mesh, RockMeshScale, RockSpinSpeed);
		Rock->Launch(Origin, Target, TravelTime, ArcHeight);
	}
	return Rock;
}

void UGolemBossComponent::ApplyRockImpactDecal(AGolemFallingRock* Rock, const FVector& Target, float Radius, float StaticLifeSpan)
{
	if (!bSpawnTelegraphDecals || !TelegraphDecalMaterial || Radius <= 0.f) return;
	if (Rock) Rock->SetImpactDecal(TelegraphDecalMaterial, Radius, DecalProjectionDepth); // grows as the rock approaches
	else SpawnTelegraphDecal(Target, Radius, StaticLifeSpan); // BP-rock / no-mesh fallback: a static warning
}

void UGolemBossComponent::SpawnHeldRock()
{
	HeldRock = nullptr;
	if (!bSpawnRockMeshes || RockActorClass) return; // BP-rock path or off: the rock is spawned at release instead
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

	// Held in the hand: follows the throw anim until LaunchRockNow detaches and throws it.
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
		FVector Base = ResolveArenaCentre() - Origin;
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
		Out.Add(Beam);
	}
}

/* ═══════════ Arena helpers ═══════════ */

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

/* ═══════════ Damage core ═══════════ */

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

void UGolemBossComponent::DealDamage(AActor* Victim, float Damage, FVector FromLocation, float Knockback, FName AttackId)
{
	if (!Victim || !OwnerCharacter || Damage <= 0.f) return;

	const float Final = Damage * CurrentDamageScale();
	FVector Dir = Victim->GetActorLocation() - FromLocation;
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

int32 UGolemBossComponent::ApplyRadialBurst(FVector Center, float Radius, float Damage, bool bAirborneIsSafe, float Knockback, FName AttackId, float MinSafeAltitude)
{
	TArray<AActor*> Targets;
	GatherTargets(Center, Radius, Targets);
	const float FloorZ = GetArenaCentre().Z;
	int32 Hits = 0;
	for (AActor* A : Targets)
	{
		if (FVector::DistSquared2D(A->GetActorLocation(), Center) > Radius * Radius) continue;
		const bool bSafe = (MinSafeAltitude > 0.f)
			? ((A->GetActorLocation().Z - FloorZ) >= MinSafeAltitude) // must FLY this high (air zone), not just jump
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

int32 UGolemBossComponent::ApplyLineDamage(FVector A, FVector B, float HalfWidth, float Damage, bool bAirborneIsSafe, bool b3D, float Knockback, FName AttackId, float MinSafeAltitude)
{
	const FVector Mid = (A + B) * 0.5f;
	const float GatherRadius = (FVector::Dist(A, B) * 0.5f) + HalfWidth + 50.f;
	TArray<AActor*> Targets;
	GatherTargets(Mid, GatherRadius, Targets);

	const float FloorZ = GetArenaCentre().Z;
	int32 Hits = 0;
	for (AActor* Actor : Targets)
	{
		const FVector Loc = Actor->GetActorLocation();
		const float D = b3D ? FMath::PointDistToSegment(Loc, A, B) : SegDist2D(Loc, A, B);
		if (D > HalfWidth) continue;
		// MinSafeAltitude > 0: only flying THAT high (an air zone) is safe — a plain jump can't clear it.
		const bool bSafe = (MinSafeAltitude > 0.f)
			? ((Loc.Z - FloorZ) >= MinSafeAltitude)
			: (bAirborneIsSafe && IsActorAirborne(Actor));
		if (bSafe) continue;
		DealDamage(Actor, Damage, Loc, Knockback, AttackId);
		++Hits;
	}
	return Hits;
}

/* ═══════════ Air zones ═══════════ */

void UGolemBossComponent::OpenAirZone(FVector Location, float Radius, float Lifetime)
{
	if (Radius <= 0.f) return;

	FGolemAirZone Zone;
	Zone.ZoneId = NextAirZoneId++;
	Zone.Location = Location;
	Zone.Radius = Radius;
	Zone.TimeRemaining = Lifetime;

	// Reuse the project's existing glider air-zone (AWindColumn) — spawn it, don't reinvent it.
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
	// Drain first, broadcast after — a bound BP handler could re-enter and mutate ActiveAirZones mid-loop.
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

/* ═══════════ Phases ═══════════ */

void UGolemBossComponent::UpdatePhaseFromHealth()
{
	if (Phases.Num() == 0) return;

	float Frac = 1.f;
	if (OwnerHealth)
	{
		const float Max = OwnerHealth->GetMaxHealth();
		if (Max > 0.f) Frac = FMath::Clamp(OwnerHealth->GetHealth() / Max, 0.f, 1.f);
	}

	int32 Best = CurrentPhase; // forward-only (never revert on heal)
	for (int32 i = 0; i < Phases.Num(); ++i)
		if (Frac <= Phases[i].HealthThreshold && i > Best)
			Best = i;

	if (Best != CurrentPhase)
	{
		const int32 Old = CurrentPhase;
		CurrentPhase = Best;
		OnGolemPhaseChanged.Broadcast(Old, CurrentPhase, Phases[CurrentPhase].PhaseName);
	}
}

float UGolemBossComponent::GetPhaseCooldownScale() const
{
	return Phases.IsValidIndex(CurrentPhase) ? Phases[CurrentPhase].CooldownScale : 1.f;
}

/* ═══════════ Weak points / topple ═══════════ */

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
		if (OwnerCharacter)
		{
			AController* Inst = Instigator ? Instigator->GetInstigatorController() : nullptr;
			UGameplayStatics::ApplyDamage(OwnerCharacter, Crit, Inst, Instigator, UDamageType::StaticClass());
		}
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
	bToppled = true; // set BEFORE FinishAttack so it won't re-arm the arm-expose linger

	if (OwnerCharacter)
		if (UAIAnimationComponent* Anim = OwnerCharacter->GetAIAnimation())
			Anim->StopCurrentAction();
	if (State != EGolemAttackState::Idle) FinishAttack(true);
	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(ExposeLingerTimerHandle);

	// Arms are spent; open the head crystal for the crit window.
	for (FGolemWeakPoint& WP : WeakPoints)
		if (WP.Kind == EGolemWeakPointKind::Head) WP.bVulnerable = true;

	if (UWorld* W = GetWorld())
		W->GetTimerManager().SetTimer(ToppleTimerHandle, this, &UGolemBossComponent::OnToppleElapsed, ToppleDuration, false);

	OnGolemToppled.Broadcast(ToppleDuration);
}

void UGolemBossComponent::OnToppleElapsed() { EndTopple(); }

void UGolemBossComponent::EndTopple()
{
	if (!bToppled) return;
	bToppled = false;
	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(ToppleTimerHandle);

	for (FGolemWeakPoint& WP : WeakPoints)
	{
		if (WP.Kind == EGolemWeakPointKind::Head) WP.bVulnerable = false;
		if (WP.Kind == EGolemWeakPointKind::Arm && bResetArmsOnRecover)
		{
			WP.CurrentHealth = WP.Health;
			WP.bBroken = false;
			WP.bVulnerable = false;
		}
	}

	const UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;
	NextAttackReadyTime = Now + GlobalCooldown; // a beat before resuming attacks

	OnGolemRecoverFromTopple.Broadcast();
}

/* ═══════════ Debug ═══════════ */

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
		DrawFlatCircle(W, ResolveArenaCentre(), GetArenaRadius(), FColor::Red, Lifetime, 8.f);
		for (const FVector& P : T.ImpactPoints)
			DrawDebugSphere(W, P, 70.f, 10, Warn, false, Lifetime);
		DrawDebugString(W, ResolveArenaCentre() + FVector(0, 0, 180.f), TEXT("FISSURES - FLY!"), nullptr, FColor::Red, Lifetime);
		break;
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
	const FColor Col = bAirborneIsSafe ? FColor(0, 160, 255) : FColor::Orange; // blue = get airborne, orange = direct hit
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

/* ═══════════ Misc ═══════════ */

FName UGolemBossComponent::GetCurrentAttackId() const
{
	return Attacks.IsValidIndex(CurrentAttackIndex) ? Attacks[CurrentAttackIndex].AttackId : NAME_None;
}

FName UGolemBossComponent::GetPendingForcedAttackId() const
{
	return Attacks.IsValidIndex(PendingForcedAttack) ? Attacks[PendingForcedAttack].AttackId : NAME_None;
}

void UGolemBossComponent::HandleOwnerDamaged(AActor* /*Instigator*/)
{
	if (!bActivated && bAutoActivateOnTarget) ActivateBoss();
}

void UGolemBossComponent::HandleOwnerDied()
{
	if (State != EGolemAttackState::Idle) FinishAttack(true);
	ClearAirZones();
	bActivated = false;
	bToppled = false; // no recover-from-topple should fire on the corpse
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(ToppleTimerHandle);
		W->GetTimerManager().ClearTimer(ExposeLingerTimerHandle);
	}
	OnGolemDefeated.Broadcast();
}
