/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemBossComponent - Header"
 * Notes: The arena Golem's attack brain. Add it to a BaseAICharacter (or use AGolemBossCharacter).
 *        It runs a self-contained Windup→Active→Recovery state machine, selects attacks (enforcing
 *        the "rock-throw before the unavoidable ground attack" rule), resolves all AoE geometry and
 *        applies damage in C++, and broadcasts a Blueprint dispatcher at every step so you wire
 *        anims / VFX / SFX / camera-shake / the air-zone flight volume from the Event Graph.
 *
 *        Pair the owner with a controller that has bUseCustomAttackLogic = ON (AGolemBossController
 *        does this) so the default chase/melee brain stays out of the way.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Templates/SubclassOf.h"
#include "Engine/TimerHandle.h"
#include "Characters/AI/Boss/GolemBoss_Types.h"
#include "GolemBossComponent.generated.h"

class ABaseAICharacter;
class UHealthComponent;
class UAnimMontage;
class UMaterialInterface;
class UStaticMesh;
class AGolemFallingRock;
class AActor;

/* ── Dispatchers — BIND THESE IN BP ── */

/** The fight begins (target acquired or ActivateBoss called). Start music, raise the arena gate, play a roar. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGolemActivated);

/** An attack was chosen and is about to wind up. Shape + id let you branch your reactions. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemAttackBegin, FName, AttackId, EGolemHazardShape, Shape);

/** Telegraph frame: everything you need to draw the warning (impact points, sweep line, beams, air-zone preview). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemTelegraph, const FGolemTelegraph&, Telegraph);

/** A discrete impact moment (per slam / per rock / per boulder). Spawn impact VFX/SFX + camera-shake at Strike.Location. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemStrike, const FGolemStrikeEvent&, Strike);

/** A rock launches ahead of its impact (thrown rock / each falling boulder). Spawn the projectile at Origin and fly it
 *  to Target over TravelTime — the matching OnGolemStrike fires when it lands. Makes thrown/falling rocks read smoothly. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemProjectileLaunch, const FGolemProjectileLaunch&, Launch);

/** Per-frame state of a live moving hazard (sweep wall, shockwave ring, laser fan). Drive your moving VFX from this. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemHazardTick, const FGolemHazardState&, State);

/** C++ applied AoE damage to a victim. Spawn the victim-side hit VFX / reaction here. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnGolemDamageDealt, AActor*, Victim, float, Damage, FName, AttackId, FVector, HitLocation);

/** The attack entered its rooted, vulnerable recovery window. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemAttackRecovery, FName, AttackId);

/** The attack fully ended (completed or interrupted). Clean up lingering VFX. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemAttackEnd, FName, AttackId, bool, bInterrupted);

/** A rock throw opened an air zone. Spawn the ground hole + the flight volume the player flies in. ZoneId maps it to its Closed event. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnGolemAirZoneOpened, int32, ZoneId, FVector, Location, float, Radius, float, Lifetime);

/** An air zone's hole closed (timed out / cleared). Remove the flight volume keyed by ZoneId. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemAirZoneClosed, int32, ZoneId, FVector, Location);

/** The boss crossed into a new phase. Swap music intensity, change lighting, etc. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGolemPhaseChanged, int32, OldPhase, int32, NewPhase, FName, PhaseName);

/** The boss was defeated (owner died). Play the cinematic, open the arena, drop loot. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGolemDefeated);

/** Arm crystals became reachable/unreachable (e.g. during the two-hand slam). bExposed + the attack id. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemWeakPointsExposed, bool, bExposed, FName, AttackId);
/** A weak-point crystal took a hit. Id + damage + remaining health (0 for a head crit). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGolemWeakPointHit, FName, Id, float, Damage, float, HealthRemaining);
/** A weak-point crystal shattered — shatter the BP mesh/VFX. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemWeakPointBroken, FName, Id);
/** Both arm crystals broke — the boss topples (head to the ground). Play the topple anim; the head crystal is now hittable. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemToppled, float, Duration);
/** The boss gets back up from a topple. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGolemRecoverFromTopple);
/** A head-crystal critical landed (player Damage * HeadCritMultiplier applied to the boss). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemCriticalHit, float, Damage, FName, Id);

/** A montage hit a designer-placed cue marker (anim start, anticipation, enrage roar, footstep…). Tag says which one. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemAnimCue, FName, Tag);

UCLASS(ClassGroup = (AI), Blueprintable, meta = (BlueprintSpawnableComponent))
class ETHERIA_API UGolemBossComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGolemBossComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* ═══════════ Activation ═══════════ */

	/** Start the fight: begin selecting/executing attacks. Auto-called when a target is acquired if bAutoActivateOnTarget. */
	UFUNCTION(BlueprintCallable, Category = "Golem") void ActivateBoss();

	/** Stop the fight: interrupt any current attack and go dormant (the boss stops attacking). */
	UFUNCTION(BlueprintCallable, Category = "Golem") void DeactivateBoss();

	UFUNCTION(BlueprintPure, Category = "Golem") bool IsBossActive() const { return bActivated; }

	/* ═══════════ Scripted control ═══════════ */

	/** Force a specific attack by id on the next opportunity (bypasses weighted selection). Returns false if unknown/unusable now. */
	UFUNCTION(BlueprintCallable, Category = "Golem") bool ForceAttack(FName AttackId);

	/** Advance the CURRENT attack from its wind-up straight into the strike NOW (call from an AnimNotify at the impact frame
	 *  instead of relying on WindupDuration). No-op outside the wind-up. */
	UFUNCTION(BlueprintCallable, Category = "Golem") void TriggerStrikeNow();

	/** End the current attack immediately (e.g. from an AnimNotify at the montage's end). */
	UFUNCTION(BlueprintCallable, Category = "Golem") void EndAttackNow();

	/** Launch the thrown rock from the hand NOW — call from an AnimNotify at the throw-release frame. It leaves the
	 *  ThrowSocketName socket and flies to the impact, landing on the strike. No-op outside a rock-throw wind-up. */
	UFUNCTION(BlueprintCallable, Category = "Golem") void LaunchRockNow();

	/** Broadcast a named animation cue (used by the Golem Cue AnimNotify for anim-start/anticipation/enrage/etc markers). Bind OnGolemAnimCue in BP. */
	UFUNCTION(BlueprintCallable, Category = "Golem") void FireAnimCue(FName Tag) { OnGolemAnimCue.Broadcast(Tag); }

	/** Interrupt the current attack (stagger/break/scripted). Fires OnGolemAttackEnd(interrupted = true). */
	UFUNCTION(BlueprintCallable, Category = "Golem") void InterruptAttack();

	/* ═══════════ Air zones ═══════════ */

	/** Manually open an air zone (flight hole). Normally a rock-throw does this for you. */
	UFUNCTION(BlueprintCallable, Category = "Golem|AirZone") void OpenAirZone(FVector Location, float Radius, float Lifetime);

	/** Remove all active air zones now (fires OnGolemAirZoneClosed for each). */
	UFUNCTION(BlueprintCallable, Category = "Golem|AirZone") void ClearAirZones();

	UFUNCTION(BlueprintPure, Category = "Golem|AirZone") bool HasActiveAirZone() const { return ActiveAirZones.Num() > 0; }
	UFUNCTION(BlueprintPure, Category = "Golem|AirZone") bool IsLocationInAirZone(FVector Location) const;
	UFUNCTION(BlueprintPure, Category = "Golem|AirZone") const TArray<FGolemAirZone>& GetActiveAirZones() const { return ActiveAirZones; }

	/* ═══════════ Weak points / topple ═══════════ */

	/** The player's attack connected with a crystal. Returns true if it was vulnerable and registered.
	 *  Arm/Generic: chips its health, breaks at 0 (break ALL arms -> the boss topples). Head (only while toppled):
	 *  deals Damage * HeadCritMultiplier to the boss. Pass the player as Instigator so the damage is credited. */
	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") bool HitWeakPoint(FName Id, float Damage, AActor* Instigator);

	/** Manually set a crystal vulnerable (designers usually let the slam expose the arms automatically). */
	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") void SetWeakPointVulnerable(FName Id, bool bVulnerable);

	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") void ForceTopple();
	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") void EndTopple();

	UFUNCTION(BlueprintPure, Category = "Golem|WeakPoint") bool IsToppled() const { return bToppled; }
	UFUNCTION(BlueprintPure, Category = "Golem|WeakPoint") bool IsWeakPointBroken(FName Id) const;
	UFUNCTION(BlueprintPure, Category = "Golem|WeakPoint") bool IsWeakPointVulnerable(FName Id) const;
	UFUNCTION(BlueprintPure, Category = "Golem|WeakPoint") FVector GetWeakPointLocation(FName Id) const;

	/* ═══════════ Damage helpers (used internally; also reusable from BP) ═══════════ */

	/** Damage every valid target whose horizontal distance to Center is under Radius.
	 *  MinSafeAltitude > 0 makes only targets flying that high (an air zone) safe — a plain jump won't clear it. */
	UFUNCTION(BlueprintCallable, Category = "Golem|Damage")
	int32 ApplyRadialBurst(FVector Center, float Radius, float Damage, bool bAirborneIsSafe, float Knockback, FName AttackId, float MinSafeAltitude = 0.f);

	/** Damage every valid target whose horizontal distance to Center is within the annulus [InnerRadius, OuterRadius] (a shockwave wavefront). */
	UFUNCTION(BlueprintCallable, Category = "Golem|Damage")
	int32 ApplyRingBurst(FVector Center, float InnerRadius, float OuterRadius, float Damage, bool bAirborneIsSafe, float Knockback, FName AttackId);

	/** Damage every valid target within HalfWidth of the segment A→B. b3D = true measures full 3D distance (beams); false = horizontal only (sweeps).
	 *  MinSafeAltitude > 0 makes only targets flying that high (an air zone) safe — a plain jump won't clear it. */
	UFUNCTION(BlueprintCallable, Category = "Golem|Damage")
	int32 ApplyLineDamage(FVector A, FVector B, float HalfWidth, float Damage, bool bAirborneIsSafe, bool b3D, float Knockback, FName AttackId, float MinSafeAltitude = 0.f);

	/* ═══════════ Queries / overridable anchors ═══════════ */

	UFUNCTION(BlueprintPure, Category = "Golem") EGolemAttackState GetAttackState() const { return State; }
	UFUNCTION(BlueprintPure, Category = "Golem") FName GetCurrentAttackId() const;
	UFUNCTION(BlueprintPure, Category = "Golem") int32 GetCurrentPhase() const { return CurrentPhase; }

	/** AttackId queued to fire next (e.g. the unavoidable fissures behind a forced rock-throw), or None. Lets BP frame the setup. */
	UFUNCTION(BlueprintPure, Category = "Golem") FName GetPendingForcedAttackId() const;

	/** Arena centre. Defaults to ArenaCentre (a world-space point you set). Override in BP to read a trigger volume, etc. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golem|Arena") FVector GetArenaCentre() const;
	virtual FVector GetArenaCentre_Implementation() const;

	/** Arena radius. Defaults to ArenaRadius. Override in BP for non-circular arenas. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golem|Arena") float GetArenaRadius() const;
	virtual float GetArenaRadius_Implementation() const;

	/** Where targeted attacks (hammer, rock) aim. Default: the current target's location, clamped into the arena. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golem|Arena") FVector ResolveTargetLocation() const;
	virtual FVector ResolveTargetLocation_Implementation() const;

	/* ═══════════ Dispatchers ═══════════ */
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemActivated OnGolemActivated;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemAttackBegin OnGolemAttackBegin;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemTelegraph OnGolemTelegraph;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemStrike OnGolemStrike;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemProjectileLaunch OnGolemProjectileLaunch;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemHazardTick OnGolemHazardTick;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemDamageDealt OnGolemDamageDealt;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemAttackRecovery OnGolemAttackRecovery;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemAttackEnd OnGolemAttackEnd;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemAirZoneOpened OnGolemAirZoneOpened;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemAirZoneClosed OnGolemAirZoneClosed;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemPhaseChanged OnGolemPhaseChanged;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemDefeated OnGolemDefeated;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemWeakPointsExposed OnGolemWeakPointsExposed;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemWeakPointHit OnGolemWeakPointHit;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemWeakPointBroken OnGolemWeakPointBroken;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemToppled OnGolemToppled;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemRecoverFromTopple OnGolemRecoverFromTopple;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemCriticalHit OnGolemCriticalHit;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemAnimCue OnGolemAnimCue;

	/* ═══════════ Config ═══════════ */

	/** The Golem's full attack repertoire. Populate this in the editor / a data-only BP child. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Attacks") TArray<FGolemAttackConfig> Attacks;

	/** Optional boss phases (HP-gated). Order them high→low HealthThreshold. Leave empty for a single-phase boss. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Phases") TArray<FGolemPhaseConfig> Phases;

	/** Breathing room between attacks: seconds the boss waits (idle, hittable) after one attack ends before the next starts.
	 *  Raise it so the player isn't endlessly chained and can punish the boss. ~2.5 = a calm, fair rhythm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing", meta = (ClampMin = "0",
		ToolTip = "Wait (s) between attacks so the player can breathe and hit back. 2-3 is a fair demo pace.")) float GlobalCooldown = 2.5f;

	/** Random extra delay added on top of GlobalCooldown so the rhythm doesn't feel metronomic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing", meta = (ClampMin = "0")) float GlobalCooldownRandom = 0.7f;

	/** Discourages (does not forbid) repeating the just-used attack. 1 = no anti-repeat, 0.3 = 70% less likely. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing", meta = (ClampMin = "0", ClampMax = "1")) float RepeatPenalty = 0.35f;

	/** Begin the fight automatically the first time the owner acquires a target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing") bool bAutoActivateOnTarget = true;

	/** Interval (s) of the cheap "brain" timer: activation / phase / air-zone / facing / idle housekeeping.
	 *  The boss does NOT tick per-frame except during a live moving hazard, so this is its main heartbeat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing", meta = (ClampMin = "0.02")) float BrainInterval = 0.05f;

	/* ── Arena ── */

	/** World-space arena centre (the floor the player fights on). Used to scatter avalanche, place fissures/sweeps/extremities. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Arena") FVector ArenaCentre = FVector::ZeroVector;

	/** Arena radius (cm). Attacks span this area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Arena", meta = (ClampMin = "100")) float ArenaRadius = 2000.f;

	/** If true, ArenaCentre is treated as an OFFSET from the Golem's actor location instead of an absolute world point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Arena") bool bArenaCentreRelativeToActor = false;

	/* ── Air Zone ── */

	/** Actor spawned at the rock-throw impact so the player can FLY out — set this to your existing AWindColumn (glider air-zone) BP.
	 *  The Golem reuses it as-is and destroys it when the hole closes. Leave empty to spawn nothing and handle it from OnGolemAirZoneOpened. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Air Zone",
		meta = (ToolTip = "Set to BP_WindColumn (your glider air-zone). The rock-throw spawns it at the hole; it is destroyed when the zone closes.")) TSubclassOf<AActor> AirZoneActorClass;

	/* ── Weak points / topple ── */

	/** The Golem's crystals. Add ArmL/ArmR (Kind=Arm) and Head (Kind=Head). Break both arms during the slam to topple it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint") TArray<FGolemWeakPoint> WeakPoints;

	/** How long the boss stays toppled (head on the ground) after both arms break — the head-crit window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "0.5")) float ToppleDuration = 8.f;

	/** Head-crystal critical multiplier: a head hit while toppled deals (player Damage * this) to the boss. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "1")) float HeadCritMultiplier = 5.f;

	/** Re-form the arm crystals when the boss gets back up, so the topple loop is repeatable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint") bool bResetArmsOnRecover = true;

	/** Extra seconds the arm crystals stay reachable after the exposing attack ends, to give the player time to climb and hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "0")) float WeakPointExposeLinger = 3.f;

	/* ── Facing ── */

	/** Slowly yaw the Golem to face the target between attacks so targeted strikes (hammer/rock) aim believably. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Facing") bool bFaceTarget = true;

	/** Yaw turn speed (deg/sec) when facing the target. Giants should turn slowly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Facing", meta = (ClampMin = "1", EditCondition = "bFaceTarget")) float FaceTurnSpeed = 60.f;

	/* ── Sockets ── */

	/** Eye sockets/bones the laser originates from. If unset/missing, the laser falls back to the actor location + a forward offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Sockets") TArray<FName> EyeSocketNames;

	/** Socket/bone the thrown rock launches from (rock-throw projectile origin). Fallback: in front of / above the actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Sockets") FName ThrowSocketName = NAME_None;

	/** Height above each avalanche impact a falling boulder spawns from — its OnGolemProjectileLaunch Origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Sockets", meta = (ClampMin = "0")) float BombardmentSpawnHeight = 2000.f;

	/* ── Decals (attack ground warnings) ── */

	/** Project ground decals showing where attacks land, so players can read incoming hits even before the final VFX exist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Decals") bool bSpawnTelegraphDecals = true;

	/** Decal material projected on the floor at impact / danger spots. Assign a circular "danger" decal — warnings then appear automatically. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Decals",
		meta = (ToolTip = "Assign a ground decal material (e.g. a red circle). Decals spawn at rock/impact/fissure spots on their own once this is set.")) TObjectPtr<UMaterialInterface> TelegraphDecalMaterial;

	/** How far the decal projects vertically onto the ground (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Decals", meta = (ClampMin = "10")) float DecalProjectionDepth = 400.f;

	/* ── Falling rocks (mesh) ── */

	/** Spawn a visible rock mesh for the thrown rock and each avalanche boulder (it flies/falls to the impact then vanishes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks") bool bSpawnRockMeshes = true;

	/** Rock meshes to choose from — every rock picks ONE of these at random. Assign 3 (or any number) for variety. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks") TArray<TObjectPtr<UStaticMesh>> RockMeshes;

	/** Uniform scale applied to each spawned rock mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0.01")) float RockMeshScale = 1.f;

	/** Arc height (cm) of the THROWN rock's flight. Avalanche boulders always fall straight. 0 = straight line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockThrowArcHeight = 500.f;

	/** When (seconds into the throw wind-up) the rock leaves the hand. Match your throw anim's release frame.
	 *  0 = at wind-up start. For frame-perfect timing, call LaunchRockNow() from an AnimNotify instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockReleaseTime = 0.8f;

	/** Tumble speed (deg/s) of the rock — a steady spin for a heavy, powerful feel. Applies while held AND in flight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockSpinSpeed = 180.f;

	/** Advanced: spawn THIS actor for rocks instead of the built-in mesh flyer (e.g. your own BP rock with physics). If set, overrides RockMeshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks") TSubclassOf<AActor> RockActorClass;

	/* ── Debug ── */

	/** Draw every attack's danger zone, telegraph, live hazard and air zone as debug shapes — lets you test the whole fight before any VFX/anim exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Debug") bool bDrawDebugHazards = true;

	/** Print the Golem's state / current attack / phase above its head. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Debug") bool bDrawDebugText = true;

	/** Seconds debug impact markers linger so you can see where bursts landed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Debug", meta = (ClampMin = "0")) float DebugImpactLinger = 0.6f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/* ── State machine (timer-driven; per-frame tick is enabled ONLY during a live moving hazard) ── */
	void BrainTick();        // BrainInterval heartbeat: activation, phases, air-zones, facing, idle -> begin
	void OnWindupElapsed();  // one-shot timer -> EnterActive
	void OnRecoveryElapsed();// one-shot timer -> FinishAttack
	void TickActive(float DeltaTime);
	void TickFacing(float DeltaTime);

	void BeginAttack(int32 Index);
	void EnterActive();
	void EnterRecovery();
	void FinishAttack(bool bInterrupted);
	void ClearSequenceTimers();

	/* ── Selection ── */
	int32 SelectNextAttack();
	bool IsAttackUsable(int32 Index) const;
	int32 FindAttackIndex(FName AttackId) const;
	int32 FindAirZoneOpener() const;

	/* ── Geometry ── */
	void BuildTelegraph(const FGolemAttackConfig& Cfg, FGolemTelegraph& Out);
	void DoStrike();                 // instant / first-frame strikes
	void TickHazard(float DeltaTime, bool bForceFinal = false); // live hazards (sweep/ring/beam/bombardment)
	void GetEyeOrigins(TArray<FVector>& Out) const;
	FVector GetThrowOrigin() const;
	void SpawnTelegraphDecal(const FVector& Location, float RadiusXY, float LifeSpan) const;
	void SpawnFallingRock(const FVector& Origin, const FVector& Target, float TravelTime, float ArcHeight);
	void SpawnHeldRock();
	void BuildBeams(const FGolemAttackConfig& Cfg, float RotateDeg, TArray<FGolemBeamSegment>& Out) const;
	FVector ResolveArenaCentre() const;
	FVector ClampToArena(const FVector& P, float Margin = 0.f) const;
	FVector RandomArenaPoint(float SpreadFraction) const;

	/* ── Damage core ── */
	void GatherTargets(FVector Center, float Radius, TArray<AActor*>& Out) const;
	bool IsActorAirborne(const AActor* A) const;
	float CurrentDamageScale() const;
	void DealDamage(AActor* Victim, float Damage, FVector FromLocation, float Knockback, FName AttackId);

	/* ── Air zones ── */
	void TickAirZones(float DeltaTime);

	/* ── Weak points / topple ── */
	void ExposeArmWeakPoints(bool bExpose, FName AttackId);
	void OnExposeLingerElapsed();
	void CheckTopple();
	void Topple();
	void OnToppleElapsed();
	FGolemWeakPoint* FindWeakPoint(FName Id);
	const FGolemWeakPoint* FindWeakPoint(FName Id) const;

	/* ── Debug ── */
	void DrawTelegraphDebug(const FGolemTelegraph& T, float Lifetime) const;
	void DrawImpactDebug(const FVector& Center, float Radius, bool bAirborneIsSafe) const;
	void DrawLiveDebug(float Lifetime) const;

	/* ── Phases ── */
	void UpdatePhaseFromHealth();
	float GetPhaseCooldownScale() const;

	UFUNCTION() void HandleOwnerDamaged(AActor* Instigator);
	UFUNCTION() void HandleOwnerDied();

	UPROPERTY() TObjectPtr<ABaseAICharacter> OwnerCharacter;
	UPROPERTY() TObjectPtr<UHealthComponent> OwnerHealth;

	EGolemAttackState State = EGolemAttackState::Idle;
	bool bActivated = false;

	int32 CurrentAttackIndex = -1;
	int32 LastAttackIndex = -1;
	int32 PendingForcedAttack = -1;   // an air-zone-requiring attack queued behind a forced rock-throw, or a BP ForceAttack
	int32 CurrentPhase = 0;

	float ActiveTimer = 0.f;
	float ActiveDurationCache = 0.f;  // the attack's full active duration (for Alpha)
	float DamageTickAccum = 0.f;
	float NextAttackReadyTime = 0.f;  // world time the global cooldown lets the next attack start
	TArray<float> AttackReadyTimes;   // per-attack world time it comes off cooldown (parallel to Attacks)

	// Resolved geometry for the in-flight attack (filled at BeginAttack, reused through Active).
	FGolemTelegraph CurrentTelegraph;
	TArray<bool> BombardImpactFired; // parallel to CurrentTelegraph.ImpactPoints — boulder impact already applied
	TArray<bool> BombardLaunched;    // parallel — boulder launch (fall start) already broadcast

	UPROPERTY() TArray<FGolemAirZone> ActiveAirZones;
	int32 NextAirZoneId = 1;

	UPROPERTY() TObjectPtr<AGolemFallingRock> HeldRock; // rock held in the hand during a throw wind-up, before release

	bool bToppled = false;
	bool bRockLaunched = false;          // the thrown rock already left the hand this attack
	FName ExposingAttackId = NAME_None;  // attack currently exposing the arm crystals

	FTimerHandle BrainTimerHandle;
	FTimerHandle WindupTimerHandle;
	FTimerHandle RecoveryTimerHandle;
	FTimerHandle ToppleTimerHandle;
	FTimerHandle ExposeLingerTimerHandle;
	FTimerHandle RockReleaseTimerHandle;
};
