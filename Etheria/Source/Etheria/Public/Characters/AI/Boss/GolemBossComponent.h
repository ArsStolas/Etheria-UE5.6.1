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
class AGolemCrystal;
class UGolemBossBarWidget;
class UAudioComponent;
class USoundBase;
class UNiagaraComponent;
class UNiagaraSystem;
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

/* ── Arena crystals (two-hand slam plants them) ── */
/** The slam planted an arena crystal — spawn the destructible crystal mesh/collider at Location. Id maps it to its hit/destroy events; the actor calls HitArenaCrystal(Id, ...) when the player connects. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemArenaCrystalSpawned, int32, Id, FVector, Location);
/** An arena crystal took a hit. Id + hits left. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemArenaCrystalHit, int32, Id, int32, HitsRemaining);
/** An arena crystal was destroyed (remove its actor, shatter VFX). It dealt the player's damage × mult to the boss. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemArenaCrystalDestroyed, int32, Id, FVector, Location);
/** Despawn any leftover arena crystals (a new slam replaced them, or the boss stunned/died). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGolemArenaCrystalsCleared);
/** The big crystal appeared at the ARENA CENTRE during the stun — spawn the destructible big-crystal actor at Location
 *  (BigCrystalHitsToBreak hits to break). The actor calls HitBigCrystal when the player connects. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemBigCrystalSpawned, FVector, Location);
/** The big crystal took a hit. Hits left. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemBigCrystalHit, int32, HitsRemaining);
/** The big crystal broke — it removed DamageDealt (≈ half max HP) from the boss. Shatter it / end the stun visuals. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemBigCrystalBroken, float, DamageDealt);

/** A montage hit a designer-placed cue marker (anim start, anticipation, enrage roar, footstep…). Tag says which one. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemAnimCue, FName, Tag);

/** The boss shoved the player back on contact. Bind for a "thud" SFX / knockback VFX / camera shake. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemRepelledPlayer, AActor*, Player, FVector, Direction);

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

	/** The player struck an ARM (the always-hittable arm structure — the boss BODY itself takes no normal damage).
	 *  Deals Damage straight to the boss HP (bypassing the body's invulnerability). Each hit also chips that arm's
	 *  crystal; after ArmCrystalHitsToBreak hits the crystal shatters (OnGolemWeakPointBroken) and every later hit on
	 *  that arm deals Damage * BrokenArmDamageMultiplier (the bonus). Breaking BOTH arm crystals still topples the boss.
	 *  Returns false if ArmId isn't an Arm weak point or the arm isn't currently hittable. Pass the player as Instigator. */
	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") bool HitArm(FName ArmId, float Damage, AActor* Instigator);

	/** Route a plain hit on the boss BODY to the correct weak point: the head crystal while toppled, otherwise the arm
	 *  nearest the attacker. The shared player melee only ever resolves "the boss actor" (a query trace fires no
	 *  per-collider events), so AGolemBossCharacter::TakeDamage funnels every hit through here — no per-part BP colliders
	 *  needed. Returns true if a weak point absorbed it; false (e.g. arms not currently hittable) lets the body shrug it off. */
	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") bool RouteBodyHit(float Damage, AActor* Instigator);

	/** Manually set a crystal vulnerable (designers usually let the slam expose the arms automatically). */
	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") void SetWeakPointVulnerable(FName Id, bool bVulnerable);

	/* ═══════════ Arena crystals (slam mechanic) ═══════════ */

	/** The player's attack connected with an arena crystal (call from the crystal actor's hit handler with the Id from
	 *  OnGolemArenaCrystalSpawned). Each call counts as ONE hit; on the final hit the crystal is destroyed, dealing
	 *  PlayerDamage * ArenaCrystalBreakDamageMult to the boss, and destroying ALL of the slam's crystals STUNS it.
	 *  Returns true if the hit registered. Pass the player as Instigator. */
	UFUNCTION(BlueprintCallable, Category = "Golem|Crystals") bool HitArenaCrystal(int32 CrystalId, float PlayerDamage, AActor* Instigator);

	/** The player struck the BIG crystal (it spawns at the arena centre during the stun — call this from that crystal
	 *  actor's hit handler). Each call = one hit; on the last hit it breaks, removing BigCrystalHealthFraction of the
	 *  boss MAX HP and ending the stun. Returns true while the big crystal is live. */
	UFUNCTION(BlueprintCallable, Category = "Golem|Crystals") bool HitBigCrystal(float PlayerDamage, AActor* Instigator);

	UFUNCTION(BlueprintPure, Category = "Golem|Crystals") bool IsBigCrystalActive() const { return bBigCrystalActive; }
	UFUNCTION(BlueprintPure, Category = "Golem|Crystals") const TArray<FGolemArenaCrystal>& GetArenaCrystals() const { return ArenaCrystals; }

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
	int32 ApplyLineDamage(FVector A, FVector B, float HalfWidth, float Damage, bool bAirborneIsSafe, bool b3D, float Knockback, FName AttackId, float MinSafeAltitude = 0.f, FVector KnockbackDir = FVector::ZeroVector);

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
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemArenaCrystalSpawned OnGolemArenaCrystalSpawned;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemArenaCrystalHit OnGolemArenaCrystalHit;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemArenaCrystalDestroyed OnGolemArenaCrystalDestroyed;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemArenaCrystalsCleared OnGolemArenaCrystalsCleared;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemBigCrystalSpawned OnGolemBigCrystalSpawned;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemBigCrystalHit OnGolemBigCrystalHit;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemBigCrystalBroken OnGolemBigCrystalBroken;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemAnimCue OnGolemAnimCue;
	UPROPERTY(BlueprintAssignable, Category = "Golem|Events") FOnGolemRepelledPlayer OnGolemRepelledPlayer;

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

	/** Begin the fight automatically the first time the owner sees the player. UNCHECK for a scripted encounter:
	 *  the boss then stays dormant (won't face/attack) until you call ActivateBoss() from a trigger / after an intro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing") bool bAutoActivateOnTarget = true;

	/* ── Encounter intro ── */

	/** Optional wake-up montage played when ActivateBoss() fires. The boss holds still (no facing/attacks) until it ends
	 *  (or for IntroDuration), then the fight begins. Leave empty for an instant start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Encounter") TObjectPtr<UAnimMontage> IntroMontage;

	/** Seconds the boss waits (idle, playing the intro) after activation before its first attack. 0 = use the IntroMontage's
	 *  length, or start instantly if there's no montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Encounter", meta = (ClampMin = "0")) float IntroDuration = 0.f;

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

	/** Arms can be hit at ALL times (the giant's arms are the player's damage outlet; the body itself stays invulnerable).
	 *  OFF = arms only register hits while a slam exposes them (bExposesArmWeakPoints / IsWeakPointVulnerable). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint") bool bArmsAlwaysHittable = true;

	/** Player hits an arm soaks before its crystal shatters. After it shatters, that arm takes the broken-crystal bonus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "1")) int32 ArmCrystalHitsToBreak = 3;

	/** Damage multiplier applied to arm hits once that arm's crystal is shattered (the "extra damage" bonus). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "1")) float BrokenArmDamageMultiplier = 2.f;

	/** How long the boss stays toppled (head on the ground) after both arms break — the head-crit window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "0.5")) float ToppleDuration = 8.f;

	/** Head-crystal critical multiplier: a head hit while toppled deals (player Damage * this) to the boss. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "1")) float HeadCritMultiplier = 5.f;

	/** Re-form the arm crystals when the boss gets back up, so the topple loop is repeatable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint") bool bResetArmsOnRecover = true;

	/** Extra seconds the arm crystals stay reachable after the exposing attack ends, to give the player time to climb and hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "0")) float WeakPointExposeLinger = 3.f;

	/* ── Arena crystals (the two-hand-slam stun mechanic) ── */

	/** The two-hand slam plants destructible crystals in the arena (at the fissure points). Destroy them all to STUN
	 *  the boss; while stunned a big crystal can be broken for a huge HP chunk. Turn OFF to use the old arm-crystal flow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals",
		meta = (ToolTip = "Two-hand slam plants arena crystals (the new stun mechanic). OFF = use the arm-crystal/topple flow instead.")) bool bSlamPlantsArenaCrystals = true;

	/** Hits to destroy one small arena crystal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "1")) int32 ArenaCrystalHitsToBreak = 3;

	/** Destroying a small arena crystal deals (the player's hit damage × this) to the boss — the main way to chip the giant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "0",
		ToolTip = "On destroy, a crystal deals player damage × this to the boss. 6 = the x6 burst.")) float ArenaCrystalBreakDamageMult = 6.f;

	/** Hits to break the BIG crystal that appears on the golem during the stun. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "1")) int32 BigCrystalHitsToBreak = 6;

	/** Breaking the big crystal removes this fraction of the boss MAX HP (0.5 = half). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "0", ClampMax = "1",
		ToolTip = "Break the big crystal → remove this fraction of the boss MAX HP. 0.5 = half its life.")) float BigCrystalHealthFraction = 0.5f;

	/** After the big crystal breaks, the golem stays DOWN and STUNNED (a free-hit window) for this long, then recovers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "0.5",
		ToolTip = "Seconds the golem stays stunned/down after you break the big crystal, before it gets back up.")) float BigCrystalStunDuration = 6.f;

	/** Keep planted crystals at least this far INSIDE the arena edge — the slam's hands can land at/over the rim (where
	 *  there may be no floor to stand on). Crystals are clamped into the arena, then snapped to the ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "0")) float ArenaCrystalEdgeMargin = 200.f;

	/** Actor spawned for each small arena crystal. Set this to your crystal BP (a child of AGolemCrystal carrying a
	 *  crystal mesh) and the slam plants/destroys it for you — no Event-Graph wiring. Defaults to the bare C++ crystal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals",
		ToolTip = "Crystal BP planted at each fissure point. Child of AGolemCrystal with a mesh. The slam spawns & destroys it automatically.")) TSubclassOf<AGolemCrystal> ArenaCrystalActorClass;

	/** Actor spawned for the BIG stun crystal at the arena centre. Child of AGolemCrystal with a (bigger) crystal mesh.
	 *  Defaults to the bare C++ crystal; leave empty to spawn nothing and handle it from OnGolemBigCrystalSpawned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals",
		ToolTip = "Big crystal BP opened at the arena centre during the stun. Child of AGolemCrystal with a mesh.")) TSubclassOf<AGolemCrystal> BigCrystalActorClass;

	/* ── Facing ── */

	/** Slowly yaw the Golem to face the target between attacks so targeted strikes (hammer/rock) aim believably. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Facing") bool bFaceTarget = true;

	/** Yaw turn speed (deg/sec) when facing the target. Giants should turn slowly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Facing", meta = (ClampMin = "1", EditCondition = "bFaceTarget")) float FaceTurnSpeed = 60.f;

	/* ── Contact repulsion ── */

	/** Shove the player back when they touch the Golem's body — you can't walk into a giant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Contact") bool bRepelOnContact = true;

	/** Distance (cm) from the Golem within which the player is pushed out. Set to roughly the Golem's body radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Contact", meta = (ClampMin = "0", EditCondition = "bRepelOnContact")) float RepulsionRadius = 500.f;

	/** Outward launch speed (cm/s) applied to the player on contact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Contact", meta = (ClampMin = "0", EditCondition = "bRepelOnContact")) float RepulsionForce = 1200.f;

	/** Minimum delay between two shoves so the player isn't pinned in place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Contact", meta = (ClampMin = "0", EditCondition = "bRepelOnContact")) float RepulsionInterval = 0.6f;

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

	/** Uniform world scale applied to each spawned rock mesh. Bump it up for a giant Golem (rocks should look heavy). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0.01")) float RockMeshScale = 3.f;

	/** Arc height (cm) of the THROWN rock's flight. Avalanche boulders always fall straight. 0 = straight line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockThrowArcHeight = 500.f;

	/** When (seconds into the throw wind-up) the rock leaves the hand. Match your throw anim's release frame.
	 *  0 = at wind-up start. For frame-perfect timing, call LaunchRockNow() from an AnimNotify instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockReleaseTime = 0.8f;

	/** Tumble speed (deg/s) of the rock — a steady spin for a heavy, powerful feel. Applies while held AND in flight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockSpinSpeed = 180.f;

	/** Advanced: spawn THIS actor for rocks instead of the built-in mesh flyer (e.g. your own BP rock with physics). If set, overrides RockMeshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks") TSubclassOf<AActor> RockActorClass;

	/** THROWN rock flight time (s): the rock flies over this long and the strike WAITS for it to land (stays synced), so a
	 *  bigger value = a slower, more readable throw. 0 = auto (lands exactly when the wind-up ends — the fastest). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockThrowTravelTime = 1.2f;

	/* ── Boss UI (health bar) ── */

	/** Widget shown while the fight is on (boss name + HP bar). Reparent a WBP to UGolemBossBarWidget and assign it here;
	 *  the component creates / shows / hides + feeds it automatically. Leave empty for no bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|UI") TSubclassOf<UGolemBossBarWidget> BossBarWidgetClass;

	/** Name shown on the boss bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|UI") FText BossDisplayName = FText::FromString(TEXT("Golem"));

	/** Draw order of the boss bar on the viewport (project convention = 100). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|UI") int32 BossBarZOrder = 100;

	/* ── Combat music ── */

	/** Looping track played while in combat with the boss — starts on activation, fades out on defeat. Empty = none
	 *  (or wire your own from the OnGolemActivated / OnGolemDefeated dispatchers). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Audio") TObjectPtr<USoundBase> CombatMusic;

	/** Fade-in time (s) when the combat music starts. 0 = instant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Audio", meta = (ClampMin = "0")) float CombatMusicFadeIn = 1.f;

	/** Fade-out time (s) when the fight ends. 0 = cut. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Audio", meta = (ClampMin = "0")) float CombatMusicFadeOut = 2.f;

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
	void TickContactRepulsion();

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
	AGolemFallingRock* SpawnFallingRock(const FVector& Origin, const FVector& Target, float TravelTime, float ArcHeight);
	void SpawnHeldRock();
	void ApplyRockImpactDecal(AGolemFallingRock* Rock, const FVector& Target, float Radius, float StaticLifeSpan);
	void BuildBeams(const FGolemAttackConfig& Cfg, float RotateDeg, TArray<FGolemBeamSegment>& Out) const;

	/* ── Beam VFX (auto-spawned + driven Niagara, one per beam) ── */
	void SpawnBeamVFX(const FGolemAttackConfig& Cfg, const TArray<FGolemBeamSegment>& Beams); // at the fire moment
	void UpdateBeamVFX(const TArray<FGolemBeamSegment>& Beams);                                // per frame: drive start/end/width
	void ClearBeamVFX();                                                                       // beam off (active ends / interrupt / endplay)
	void SpawnBeamFireVFX(UNiagaraSystem* System);                                             // one-shot full-screen impact at the camera
	FVector ResolveArenaCentre() const;
	FVector ClampToArena(const FVector& P, float Margin = 0.f) const;
	FVector RandomArenaPoint(float SpreadFraction) const;
	FVector ProjectToGround(const FVector& P) const; // trace down to the real floor Z (so crystals/debug aren't left floating)
	bool ResolveSocketPoints(const FGolemAttackConfig& Cfg, TArray<FVector>& Out) const; // strike-frame socket world positions, ground-projected (socket-driven impacts)

	/* ── Damage core ── */
	void GatherTargets(FVector Center, float Radius, TArray<AActor*>& Out) const;
	bool IsActorAirborne(const AActor* A) const;
	float CurrentDamageScale() const;
	void DealDamage(AActor* Victim, float Damage, FVector FromLocation, float Knockback, FName AttackId, FVector KnockbackDirOverride = FVector::ZeroVector);

	/** Apply damage to the BOSS itself, bypassing its body invulnerability (used by weak-point / arm hits).
	 *  Lifts the HealthComponent's invuln for the single hit so normal melee on the body stays harmless. */
	void DealDamageToBoss(float Amount, AActor* Instigator);

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

	/* ── Arena crystals ── */
	void PlantArenaCrystals();   // slam impact: drop the destructible crystals at the fissure points
	void ClearArenaCrystals();   // despawn any leftover crystals (new slam / stun / death)
	bool AllArenaCrystalsDestroyed() const;
	void SpawnBigCrystal();      // on stun: open the big crystal for the HP-chunk break
	void EndBigCrystal();        // stun ended (timed out or broken): retract the big crystal

	/* ── Debug ── */
	void DrawTelegraphDebug(const FGolemTelegraph& T, float Lifetime) const;
	void DrawImpactDebug(const FVector& Center, float Radius, bool bAirborneIsSafe) const;
	void DrawLiveDebug(float Lifetime) const;

	/* ── Phases ── */
	void UpdatePhaseFromHealth();
	float GetPhaseCooldownScale() const;

	UFUNCTION() void HandleOwnerDamaged(AActor* Instigator);
	UFUNCTION() void HandleOwnerDied();

	/* ── Boss UI / combat music ── */
	UFUNCTION() void ShowBossUIAndMusic();   // OnGolemActivated -> create the bar + start the combat music
	UFUNCTION() void HideBossUIAndMusic();   // OnGolemDefeated / deactivate / endplay -> remove the bar + fade the music
	UFUNCTION() void HandleBossHealthChanged(float NewHealth, float MaxHealth); // OwnerHealth->OnHealthChanged -> bar fill

	UPROPERTY() TObjectPtr<ABaseAICharacter> OwnerCharacter;
	UPROPERTY() TObjectPtr<UHealthComponent> OwnerHealth;
	UPROPERTY(Transient) TObjectPtr<UGolemBossBarWidget> BossBar;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> CombatMusicComp;
	UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraComponent>> BeamVFXComps; // live laser beam VFX, one per beam

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
	float IntroEndTime = 0.f;         // world time the intro anim ends and the boss may start facing/attacking
	float NextRepulsionTime = 0.f;    // world time the next contact shove is allowed
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

	UPROPERTY() TArray<FGolemArenaCrystal> ArenaCrystals; // crystals currently planted in the arena
	int32 NextArenaCrystalId = 1;
	bool bBigCrystalActive = false;
	int32 BigCrystalHitsRemaining = 0;
	UPROPERTY() TObjectPtr<AActor> BigCrystalActor; // the big stun crystal actor, while it is open

	FTimerHandle BrainTimerHandle;
	FTimerHandle WindupTimerHandle;
	FTimerHandle RecoveryTimerHandle;
	FTimerHandle ToppleTimerHandle;
	FTimerHandle ExposeLingerTimerHandle;
	FTimerHandle RockReleaseTimerHandle;
	FTimerHandle BeamFireVFXTimerHandle; // delayed BeamFireVFX spawn
};
