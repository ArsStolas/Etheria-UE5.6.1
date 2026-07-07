/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemBoss_Types - Header"
 * Notes: Data model for the arena Golem boss. The Golem stands OUTSIDE the arena and strikes
 *        across the WHOLE arena with telegraphed AoE attacks. All geometry is resolved in C++
 *        (authoritative damage); every phase of every attack fires a Blueprint dispatcher so
 *        designers wire anims / VFX / SFX / camera-shake from the Event Graph. No C++ override
 *        of the component is needed to skin the fight.
 */

#pragma once

#include "CoreMinimal.h"
#include "GolemBoss_Types.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class UStaticMesh;
class UMaterialInterface;
class AActor;

/** Behavioural archetype of a Golem attack — drives C++ damage geometry AND the data handed to BP. */
UENUM(BlueprintType)
enum class EGolemHazardShape : uint8
{
	RadialSlam       UMETA(DisplayName = "Radial Slam",       ToolTip = "Hammer punch: instant burst at a point, plus an optional expanding shockwave RING the player jumps over."),
	ProjectileImpact UMETA(DisplayName = "Projectile Impact", ToolTip = "Rock throw: a single impact at the target. Can OPEN AN AIR ZONE (a flight hole) where it lands."),
	SweepLine        UMETA(DisplayName = "Sweep Line",        ToolTip = "Arena sweep: a wall travels across the arena. Jump the line to clear it (airborne = safe)."),
	GroundFissures   UMETA(DisplayName = "Ground Fissures",   ToolTip = "Two-hand slam: cracks span the whole floor. Everyone TOUCHING THE GROUND is hit — only flying (air zone) is safe."),
	BeamSweep        UMETA(DisplayName = "Beam Sweep",        ToolTip = "Eye laser: beams emit from the eye sockets and sweep in an X / V pattern."),
	Bombardment      UMETA(DisplayName = "Bombardment",       ToolTip = "Rock avalanche: many random impacts to dodge over a duration. Does NOT open air zones.")
};

/** Where an attack's impact geometry is anchored. */
UENUM(BlueprintType)
enum class EGolemTargetMode : uint8
{
	AtTarget          UMETA(DisplayName = "At Target",          ToolTip = "Aim the impact at the current target (player). Falls back to the arena centre if there is no target."),
	ArenaCentre       UMETA(DisplayName = "Arena Centre",       ToolTip = "Anchor at the arena centre."),
	ArenaExtremities  UMETA(DisplayName = "Arena Extremities",  ToolTip = "Two impacts at opposite arena edges (two-hand slam)."),
	ArenaRandom       UMETA(DisplayName = "Arena Random",       ToolTip = "Random point(s) inside the arena (avalanche)."),
	EyeSockets        UMETA(DisplayName = "Eye Sockets",        ToolTip = "Originate at the Golem's eye sockets (laser).")
};

/** Lifecycle stage of the Golem's CURRENT attack. */
UENUM(BlueprintType)
enum class EGolemAttackState : uint8
{
	Idle      UMETA(ToolTip = "Not attacking — picking the next move / cooling down."),
	Windup    UMETA(ToolTip = "Telegraph: warning decals/VFX up, wind-up anim playing, no damage yet."),
	Active    UMETA(ToolTip = "Strike(s) and live hazard — damage is being applied."),
	Recovery  UMETA(ToolTip = "Rooted, vulnerable recovery window after the strike.")
};

/** Kind of destructible weak point (crystal) on the Golem. */
UENUM(BlueprintType)
enum class EGolemWeakPointKind : uint8
{
	Arm     UMETA(ToolTip = "Arm crystal — exposed while the two-hand slam keeps the arms down. Break EVERY Arm to topple the boss."),
	Head    UMETA(ToolTip = "Head crystal — only hittable while the boss is toppled; deals the critical multiplier."),
	Generic UMETA(ToolTip = "Any other designer-defined weak point.")
};

/** One laser/sweep ray, in world space. Handed to BP so the beam VFX can be placed/oriented exactly. */
USTRUCT(BlueprintType)
struct FGolemBeamSegment
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector Origin = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector Direction = FVector::ForwardVector;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float Length = 3000.f;

	/** Convenient end point (Origin + Direction * Length). */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector End = FVector::ZeroVector;

	/** Radius (half-width) of the damaging tube. Drive your beam VFX's thickness from this so it matches the hit radius. */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float Width = 160.f;
};

/**
 * One attack in the Golem's repertoire. Most fields are per-shape — only the ones relevant to the
 * chosen Shape are read at runtime, but all are exposed so designers can tune any attack in place.
 */
USTRUCT(BlueprintType)
struct FGolemAttackConfig
{
	GENERATED_BODY()

	/* ── Identity ── */

	/** Name used to identify this attack in BP events and in ForceAttack(). Make it unique. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity") FName AttackId = NAME_None;

	/** Behavioural archetype — decides the damage geometry and which fields below matter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity") EGolemHazardShape Shape = EGolemHazardShape::RadialSlam;

	/** Wind-up / cast montage played on the Golem when this attack starts. Optional (you can also drive anim purely from the dispatchers). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity") TObjectPtr<UAnimMontage> Montage = nullptr;

	/** Where the impact geometry is anchored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity") EGolemTargetMode TargetMode = EGolemTargetMode::AtTarget;

	/* ── Timing ── */

	/** Telegraph length: seconds of warning before the first hit lands. Bigger = more readable / dodgeable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0", ToolTip = "Seconds of telegraph before the strike. Match this to your wind-up anim.")) float WindupDuration = 1.4f;

	/** How long the hazard stays live (sweep travel time, ring expansion, laser sweep, avalanche window). 0 = instant one-shot strike. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0", ToolTip = "Duration of the live hazard. 0 = single instant hit (e.g. a plain slam).")) float ActiveDuration = 0.f;

	/** Rooted, vulnerable window after the strike — the player's punish opening. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0", ToolTip = "Vulnerable recovery after the attack. Bigger = more punishable.")) float RecoveryDuration = 1.0f;

	/** Cooldown on THIS attack before it can be chosen again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0")) float Cooldown = 7.f;

	/** For live hazards (sweep/ring/fissures/laser): how often damage is re-applied to whoever is caught. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.02", ToolTip = "Re-tick interval for continuous hazards. Lower = more punishing to stand in.")) float DamageInterval = 0.25f;

	/* ── Damage ── */

	/** Damage per hit applied by the primary impact / per damage tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0")) float Damage = 25.f;

	/** Launch impulse applied to a victim along the hit direction. 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0")) float KnockbackForce = 0.f;

	/** If true, an AIRBORNE target (jumping / falling / flying — not on the ground) takes NO damage from this attack.
	 *  This is the core "jump it" / "fly to escape" mechanic. Defaults are set per-shape but you can override here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ToolTip = "Airborne = safe. ON for sweeps/fissures/shockwaves (jump or fly to avoid). OFF for lasers / direct impacts.")) bool bAirborneIsSafe = false;

	/** Altitude above the arena floor (cm) you must be ABOVE to be safe. Set high (e.g. 800) so a normal jump can't
	 *  clear it — only FLYING up an air zone does. 0 = altitude doesn't matter (a plain jump/airborne avoids it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (ClampMin = "0", ToolTip = "Min height (cm) to clear this attack. High value = must FLY an air zone, a jump won't do. 0 = off.")) float MinSafeAltitude = 0.f;

	/* ── Geometry: Radial / Impact (RadialSlam, ProjectileImpact, Bombardment rock) ── */

	/** Kill radius of the core impact (the fist, the rock, each falling boulder). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Impact", meta = (ClampMin = "0")) float ImpactRadius = 350.f;

	/** Optional: bone/socket name(s) on the Golem mesh whose ground-projected position(s) become this attack's impact
	 *  point(s) AT THE STRIKE FRAME — so the damage/fissures/crystals land exactly under the hand(s) the anim slams down.
	 *  One name = a single impact (e.g. a fist); two = the two-hand slam's two fissure points. Empty = default arena geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Impact",
		meta = (ToolTip = "Bone/socket name(s) the impact snaps under at the hit frame (e.g. hand_l, hand_r). Empty = default geometry.")) TArray<FName> ImpactSocketNames;

	/* ── Geometry: Shockwave ring (RadialSlam) ── */

	/** Hammer punch: emit an expanding shockwave ring around the impact (the part you jump over). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Shockwave") bool bEmitShockwave = true;

	/** Outer radius the ring grows to over ActiveDuration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Shockwave", meta = (ClampMin = "0", EditCondition = "bEmitShockwave")) float ShockwaveMaxRadius = 1400.f;

	/** Thickness of the damaging wavefront (the annulus that hurts as the ring passes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Shockwave", meta = (ClampMin = "10", EditCondition = "bEmitShockwave")) float ShockwaveThickness = 250.f;

	/** Damage dealt by the ring wavefront (often lower than the core impact). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Shockwave", meta = (ClampMin = "0", EditCondition = "bEmitShockwave")) float ShockwaveDamage = 18.f;

	/* ── Geometry: Sweep (SweepLine) ── */

	/** Half-width of the sweeping wall — how thick the damaging band is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Sweep", meta = (ClampMin = "10")) float SweepThickness = 300.f;

	/** Length of the wall (set ~arena diameter so it spans the whole arena). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Sweep", meta = (ClampMin = "100")) float SweepLength = 3000.f;

	/** Sweep anim played when the wall travels toward the Golem's RIGHT (starts left edge → ends right = "left to right").
	 *  Usually one arm. Optional — falls back to Montage if unset. The Golem faces the player, so "to the Golem's right"
	 *  is the player's left; if it looks mirrored in game, just swap this clip with SweepMontageRightToLeft. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Sweep") TObjectPtr<UAnimMontage> SweepMontageLeftToRight;

	/** Sweep anim played when the wall travels toward the Golem's LEFT ("right to left") — the OTHER arm. Optional, falls back to Montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Sweep") TObjectPtr<UAnimMontage> SweepMontageRightToLeft;

	/* ── Geometry: Beams (BeamSweep) ── */

	/** Number of beams. 2 gives a V (or, mirrored, an X). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (ClampMin = "1", ClampMax = "8")) int32 BeamCount = 2;

	/** Length of each beam (set long enough to cross the arena). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (ClampMin = "100")) float BeamLength = 3500.f;

	/** Half-width of the damaging beam tube. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (ClampMin = "10")) float BeamWidth = 160.f;

	/** Opening angle between the outermost beams (the V/X spread), in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (ClampMin = "0", ClampMax = "180")) float BeamSpreadAngle = 70.f;

	/** How far the whole beam fan rotates across ActiveDuration (the sweep), in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (ClampMin = "0", ClampMax = "180")) float BeamSweepAngle = 50.f;

	/** Beam VFX (Niagara) the component AUTO-spawns (one per beam) and drives every frame — no BP wiring. Expose Vector user
	 *  params "BeamStart" / "BeamEnd" and a float "BeamWidth" in your system; the component sets them (BeamWidth = the radius,
	 *  so the VFX scales with BeamWidth). Empty = no built-in beam VFX (wire it yourself from the dispatchers instead). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam") TObjectPtr<UNiagaraSystem> BeamVFX;

	/** One-shot impact / full-screen VFX played ONCE the instant the beam FIRES (spawned at the player camera). Empty = none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam") TObjectPtr<UNiagaraSystem> BeamFireVFX;

	/** Delay (s) after the beam fires before BeamFireVFX plays — use it to line the screen impact up with your montage. 0 = instant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (ClampMin = "0")) float BeamFireVFXDelay = 0.f;

	/** Eye-laser style: the beam scrapes FROM NEAR THE GOLEM along the ground OUT TO THE PLAYER (instead of a fixed beam) —
	 *  it aims low near the boss, holds, then sweeps out to catch the player. Overrides the yaw sweep when ON. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam") bool bBeamRiseFromGround = false;

	/** Rise-laser: where the sweep STARTS along the golem→player line (0 = at the golem, 1 = at the player). ~0.15 = just in front of the boss. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bBeamRiseFromGround")) float BeamSweepStartFraction = 0.15f;

	/** Rise-laser: optional extra height ABOVE the player the beam ends at. 0 = stop right at the player (don't go higher). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (ClampMin = "0", EditCondition = "bBeamRiseFromGround")) float BeamRiseExtraHeight = 0.f;

	/** Rise-laser: fraction of the active window the beam HOLDS at the start (near the golem) before it sweeps out to the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (ClampMin = "0", ClampMax = "0.95", EditCondition = "bBeamRiseFromGround")) float BeamRiseHoldFraction = 0.4f;

	/* ── Mesh beam (reliable, C++-driven; skin it with a material) ── */

	/** Reliable C++ beam: a MESH stretched + oriented from the eye to the target EVERY FRAME (follows perfectly, no Niagara
	 *  authoring). The beam VFX above (BeamVFX) is the Niagara path; this mesh path is the bulletproof one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam") bool bUseMeshBeam = false;

	/** Mesh used for the beam (authored along +X). Leave empty to use a default box. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (EditCondition = "bUseMeshBeam")) TObjectPtr<UStaticMesh> BeamMesh;

	/** Material on the beam mesh — YOUR laser look (an emissive / additive material). Empty = the mesh's own material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Beam", meta = (EditCondition = "bUseMeshBeam")) TObjectPtr<UMaterialInterface> BeamMeshMaterial;

	/* ── Geometry: Bombardment (Bombardment) ── */

	/** Number of boulders that fall over ActiveDuration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Bombardment", meta = (ClampMin = "1")) int32 ImpactCount = 14;

	/** Per-boulder telegraph: seconds a warning marker shows before each boulder lands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Bombardment", meta = (ClampMin = "0")) float BombardmentWarnLead = 0.8f;

	/** Fraction of the arena radius the boulders scatter within (1 = whole arena). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Geometry|Bombardment", meta = (ClampMin = "0.1", ClampMax = "1")) float BombardmentSpread = 1.f;

	/* ── Air zone (the flight hole) ── */

	/** If true, this attack punches a hole that opens an AIR ZONE the player can fly in (rock throw). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Zone", meta = (ToolTip = "Rock throw: open a flight zone where the rock lands so the player can take to the air.")) bool bOpensAirZone = false;

	/** Radius of the opened air zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Zone", meta = (ClampMin = "0", EditCondition = "bOpensAirZone")) float AirZoneRadius = 700.f;

	/** How long the opened air zone stays usable before the hole closes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Zone", meta = (ClampMin = "0", EditCondition = "bOpensAirZone")) float AirZoneLifetime = 12.f;

	/** If true, this attack is UNAVOIDABLE on solid ground and may only be used while an air zone exists.
	 *  The director will FORCE an air-zone-opener (rock throw) the turn before, guaranteeing the player an escape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Zone", meta = (ToolTip = "Two-hand fissures: needs an open air zone to be fair. A rock throw is forced beforehand automatically.")) bool bRequiresAirZoneEscape = false;

	/* ── Weak points ── */

	/** While this attack runs (arms down at the extremities), expose the Arm crystals so the player can climb and break them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weak Point", meta = (ToolTip = "Two-hand slam: arms are down -> expose the arm crystals. Break both to topple the boss.")) bool bExposesArmWeakPoints = false;

	/* ── Selection / phases ── */

	/** Weighted-random selection weight. Higher = chosen more often. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0.01")) float SelectionWeight = 1.f;

	/** Earliest phase (0-based index into the Phases array) at which this attack unlocks. 0 = from the start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0")) int32 MinPhase = 0;
};

/** A boss phase, entered as HP drops past its threshold. Scales pacing/damage and unlocks attacks (FGolemAttackConfig::MinPhase). */
USTRUCT(BlueprintType)
struct FGolemPhaseConfig
{
	GENERATED_BODY()

	/** Friendly label for this phase (passed to BP / handy in the editor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase") FName PhaseName = NAME_None;

	/** Enter this phase when HP fraction drops to/under this value (1 = full HP). Order phases high→low. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase", meta = (ClampMin = "0", ClampMax = "1")) float HealthThreshold = 1.f;

	/** Multiplies every attack's cooldown in this phase (<1 = faster / more aggressive). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase", meta = (ClampMin = "0.05")) float CooldownScale = 1.f;

	/** Multiplies all damage dealt in this phase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase", meta = (ClampMin = "0.05")) float DamageScale = 1.f;

	/** Played (full-body) the moment this phase is entered — the enrage roar. Interrupts the current attack. Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase") TObjectPtr<UAnimMontage> TransitionMontage = nullptr;

	/** Extra beat (seconds) after the transition montage before the boss attacks again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase", meta = (ClampMin = "0", ClampMax = "10")) float TransitionPause = 1.f;

	/** First attack of this phase (AttackId) — the signature opener right after the roar. None = normal selection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phase") FName ForcedOpenerAttackId = NAME_None;
};

/** Telegraph payload — everything BP needs to draw the warning the moment an attack starts winding up. */
USTRUCT(BlueprintType)
struct FGolemTelegraph
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Golem") FName AttackId = NAME_None;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") EGolemHazardShape Shape = EGolemHazardShape::RadialSlam;

	/** Seconds until the strike lands (the wind-up length). */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float Duration = 0.f;

	/** Impact centres (one for a slam, two for extremities, N for a bombardment). Place a warning decal at each. */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") TArray<FVector> ImpactPoints;

	/** Per-impact land-radius, parallel to ImpactPoints (or a single entry shared by all). */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") TArray<float> ImpactRadii;

	/** Per-impact extra delay (used by bombardment to stagger boulders across the active window). Parallel to ImpactPoints. */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") TArray<float> ImpactDelays;

	/** Sweep wall at its START position: centre, the travel direction, length and thickness. */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector LineCentre = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector LineDirection = FVector::ForwardVector;   // direction the wall TRAVELS
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float LineLength = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float LineThickness = 0.f;

	/** Initial beam rays (laser), already oriented for the X/V. */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") TArray<FGolemBeamSegment> Beams;

	/** If this attack opens an air zone, where and how big (rock throw). Radius 0 = no air zone. */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector AirZoneLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float AirZoneRadius = 0.f;
};

/** One discrete impact moment — fired per slam, per rock, per boulder. BP spawns impact VFX/SFX/cam-shake at Location. */
USTRUCT(BlueprintType)
struct FGolemStrikeEvent
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FName AttackId = NAME_None;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") EGolemHazardShape Shape = EGolemHazardShape::RadialSlam;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float Radius = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") int32 ImpactIndex = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") int32 ImpactCount = 1;
};

/** Per-frame state of a LIVE hazard (sweep position, ring radius, beam orientation) so BP can drive a moving VFX. */
USTRUCT(BlueprintType)
struct FGolemHazardState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FName AttackId = NAME_None;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") EGolemHazardShape Shape = EGolemHazardShape::RadialSlam;

	/** Normalised progress through the active window, 0→1. */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float Alpha = 0.f;

	/** Sweep: current wall centre (moves across the arena). */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector LineCentre = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector LineDirection = FVector::ForwardVector;

	/** Shockwave: current ring outer radius. */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float RingRadius = 0.f;

	/** Laser: current beam rays. */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") TArray<FGolemBeamSegment> Beams;
};

/**
 * A rock leaving the Golem — the thrown rock, or each falling avalanche boulder. Spawn the projectile at Origin
 * and fly/fall it to Target over TravelTime; the matching OnGolemStrike fires the instant it lands (impact VFX/SFX/damage).
 * This is what makes thrown/falling rocks read smoothly instead of just popping at the impact frame.
 */
USTRUCT(BlueprintType)
struct FGolemProjectileLaunch
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FName AttackId = NAME_None;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") EGolemHazardShape Shape = EGolemHazardShape::ProjectileImpact;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector Origin = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector Target = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float TravelTime = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") int32 Index = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") int32 Count = 1;
};

/** A live air zone (flight hole). Exposed so BP/movement code can ask whether a point is inside one. */
USTRUCT(BlueprintType)
struct FGolemAirZone
{
	GENERATED_BODY()
	/** Stable id, unique per open. Use it to map an air-zone volume 1:1 to its Opened/Closed events. */
	UPROPERTY(BlueprintReadWrite, Category = "Golem") int32 ZoneId = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float Radius = 0.f;
	UPROPERTY(BlueprintReadWrite, Category = "Golem") float TimeRemaining = 0.f;

	/** The reused wind/air actor (e.g. AWindColumn) the Golem spawned for this zone, if any. Destroyed on close. */
	UPROPERTY(BlueprintReadOnly, Category = "Golem") TObjectPtr<AActor> SpawnedActor = nullptr;
};

/**
 * A destructible weak point (a crystal). Break every Arm crystal to topple the boss; while toppled, hitting the
 * Head crystal deals the critical multiplier. BP owns the crystal mesh/VFX and calls HitWeakPoint() when the player connects.
 */
USTRUCT(BlueprintType)
struct FGolemWeakPoint
{
	GENERATED_BODY()

	/** Unique id used by HitWeakPoint() / the dispatchers (e.g. "ArmL", "ArmR", "Head"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem") FName Id = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem") EGolemWeakPointKind Kind = EGolemWeakPointKind::Arm;

	/** Mesh socket/bone the crystal lives on (for GetWeakPointLocation / placing BP VFX). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem") FName SocketName = NAME_None;

	/** Hit points the crystal soaks before it breaks (Arm/Generic). The Head crystal doesn't break — it just takes crits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem", meta = (ClampMin = "1")) float Health = 100.f;

	/* ── Runtime ── */
	UPROPERTY(BlueprintReadOnly, Category = "Golem") float CurrentHealth = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Golem") bool bBroken = false;
	UPROPERTY(BlueprintReadOnly, Category = "Golem") bool bVulnerable = false;

	/** Arm only: player hits this arm's crystal has soaked so far. It shatters at the component's ArmCrystalHitsToBreak,
	 *  after which hits on the arm deal the broken-crystal bonus (see UGolemBossComponent::HitArm). */
	UPROPERTY(BlueprintReadOnly, Category = "Golem") int32 CrystalHits = 0;
};

/**
 * A crystal planted in the ARENA by the two-hand slam. Destroy it (ArenaCrystalHitsToBreak hits) to deal the player's
 * damage × ArenaCrystalBreakDamageMult to the boss; destroying ALL of a slam's crystals STUNS it. BP spawns the
 * destructible mesh at Location from OnGolemArenaCrystalSpawned and calls HitArenaCrystal(Id, ...) when the player connects.
 */
USTRUCT(BlueprintType)
struct FGolemArenaCrystal
{
	GENERATED_BODY()
	/** Unique id mapping this crystal to its spawn/hit/destroy events. */
	UPROPERTY(BlueprintReadOnly, Category = "Golem") int32 Id = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Golem") FVector Location = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Golem") int32 HitsRemaining = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Golem") bool bDestroyed = false;

	/** The crystal actor the component spawned for this entry (AGolemCrystal/child BP), if any. Destroyed on break/clear. */
	UPROPERTY(BlueprintReadOnly, Category = "Golem") TObjectPtr<AActor> SpawnedActor = nullptr;
};
