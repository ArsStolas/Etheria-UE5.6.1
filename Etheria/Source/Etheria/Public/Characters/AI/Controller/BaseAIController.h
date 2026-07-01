/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "BaseAIController - Header"
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Characters/AI/AI_Types.h"
#include "Perception/AIPerceptionTypes.h"
#include "BaseAIController.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class ABaseAICharacter;
class UAICombatComponent;
class UAICombatDirectorSubsystem;

UCLASS()
class ETHERIA_API ABaseAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABaseAIController(const FObjectInitializer& ObjectInitializer);

	/** Walk to investigate a threat's location (Suspicious search). Used by pack alerts and noise reactions.
	 *  BP-callable so designers can trigger an investigation (e.g. from a noise/alarm volume). */
	UFUNCTION(BlueprintCallable, Category="AI|Perception") void InvestigateThreat(AActor* Threat);

	/** Walk to investigate a world location (squad search, scripted "go check there"). */
	UFUNCTION(BlueprintCallable, Category="AI|Perception") void Investigate(const FVector& Location);

	/** Briefly turn to look at an actor (ambient curiosity / scripted glance) while idle/patrolling. */
	UFUNCTION(BlueprintCallable, Category="AI|Perception") void NoticeActor(AActor* Actor);

	/** Re-arm/disable perception and disengage to match a runtime hostility change (called by SetHostilityType). */
	UFUNCTION(BlueprintCallable, Category="AI") void ReconcileHostility();

	/** True if Threat is within this AI's leash territory. */
	UFUNCTION(BlueprintPure, Category="AI") bool IsThreatInTerritory(const AActor* Threat) const;

	/** True if this AI is itself beyond its leash tether (mirrors CheckLeashAndReturn's bSelfTooFar). Public so the
	 *  character can refuse a NEW acquisition while out of territory — it must finish returning home first. */
	bool IsSelfOutsideLeash() const;

	/** Look around once for a valid target (player-only fast path or sight overlap); engages if found, returns true.
	 *  Public so the character can re-scan the instant it teleports home — a player on the spawn re-aggros with no ramp wait. */
	bool ReacquireOnReturn();

	/** Drive your boss's attack pattern here. Fires every frame while the AI is engaged (in the Attacking state),
	 *  but ONLY when bUseCustomAttackLogic is ON — the built-in approach/slot/token/melee loop is then skipped, so you
	 *  have full control. Call GetAICombat()->ExecuteAttackByName / play montages / spawn arena hazards from BP.
	 *  Implement on a BP subclass of this controller. Target is the current foe; DistanceToTarget in cm. */
	UFUNCTION(BlueprintImplementableEvent, Category="AI|Combat|Custom")
	void TickCustomAttackLogic(AActor* Target, float DistanceToTarget, float DeltaTime);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION() void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION() void HandleAIStateChanged(EAIState OldState, EAIState NewState);
	UFUNCTION() void HandleDormancyChanged();

	void HandleIdleState(float DeltaTime);
	void HandlePatrolState(float DeltaTime);
	void HandleChaseState(float DeltaTime);
	void HandleAttackState(float DeltaTime);
	void HandleReturnState(float DeltaTime);
	void HandleFleeState(float DeltaTime);
	void HandleStaggerState(float DeltaTime);
	void HandleInvestigateState(float DeltaTime);
	void HandleInteractState(float DeltaTime);
	bool CheckLeashAndReturn();
	void FaceTargetYawOnly(AActor* Target, float DeltaTime);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight", meta=(ToolTip="How far this AI can see.")) float SightRadius = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight", meta=(ToolTip="Distance at which AI loses sight. Should be > SightRadius.")) float LoseSightRadius = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Sight", meta=(ToolTip="Half-angle of the sight cone in degrees.")) float SightFOVDegrees = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Hearing", meta=(ToolTip="360-degree hearing radius.")) float HearingRange = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception", meta=(ClampMin="0",
		ToolTip="Seconds an engaged AI keeps chasing/attacking after losing line of sight before giving up. 0 = never give up (relentless; only the leash brings it home). Set 0 deliberately for bosses."))
	float TargetMemoryDuration = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(ToolTip="Enable 360-degree close range detection.")) bool bUseProximityDetection = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(EditCondition="bUseProximityDetection", ClampMin="50")) float ProximityRadius = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Proximity", meta=(EditCondition="bUseProximityDetection", ClampMin="0.05", ClampMax="2.0")) float ProximityCheckInterval = 0.2f;

	/* ── Detection ramp (gradual sight acquisition) ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Detection",
		meta=(ToolTip="Gradual sight detection: the AI must keep the target in view for a moment before fully acquiring (gives a stealth approach window). OFF = instant aggro on sight."))
	bool bUseDetectionRamp = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Detection", meta=(EditCondition="bUseDetectionRamp", ClampMin="0.05",
		ToolTip="Seconds of continuous sight to fully detect a target at MAX sight range. Closer targets detect proportionally faster (point-blank is near-instant)."))
	float SightDetectionTime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Detection", meta=(EditCondition="bUseDetectionRamp", ClampMin="0.05", ClampMax="1.0",
		ToolTip="Detection fraction at which the AI becomes Suspicious (turn-to-look / alert bark via OnAwarenessChanged)."))
	float SuspiciousThreshold = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Detection", meta=(EditCondition="bUseDetectionRamp", ClampMin="0.1",
		ToolTip="How fast detection drains back down when the target leaves sight (multiplier of the fill rate)."))
	float DetectionDecayRate = 1.0f;

	/** Detection fill multiplier at the edge of the sight cone vs centre (1.0). <1 = slower to notice peripheral targets. 360° proximity is unaffected. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Detection", meta=(EditCondition="bUseDetectionRamp", ClampMin="0.1", ClampMax="1.0",
		ToolTip="How much slower a target at the FOV edge fills the detection meter vs dead-centre. Adds foveal vs peripheral realism."))
	float PeripheralDetectionScale = 0.4f;

	/* ── Investigate (hearing) ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate",
		meta=(ToolTip="When the AI hears a noise (and isn't already chasing), it walks to investigate the source instead of instantly locking on through walls."))
	bool bInvestigateNoises = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate", meta=(EditCondition="bInvestigateNoises", ClampMin="0.5",
		ToolTip="Seconds to search around a heard noise before giving up and returning."))
	float InvestigateDuration = 4.f;

	/** Radius around the noise the AI sweeps between search points while investigating (looks around, not one spot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate", meta=(EditCondition="bInvestigateNoises", ClampMin="50",
		ToolTip="How far around the noise the AI wanders while searching."))
	float InvestigateSearchRadius = 500.f;

	/** How long an idle NPC keeps turning to look at something it noticed (ambient curiosity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate", meta=(ClampMin="0.1",
		ToolTip="Seconds an idle NPC keeps facing a noticed actor before losing interest."))
	float NoticeDuration = 3.f;

	/** Lifetime of a heard-noise "Suspicious" reaction for prey/non-hunters before it fades back to Unaware. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate", meta=(ClampMin="0.1",
		ToolTip="How long an NPC stays edgy after hearing a noise, so a bark/posture (OnAwarenessChanged) can actually play."))
	float SuspicionDuration = 2.5f;

	/** Beyond this distance from the interaction partner, the NPC auto-ends the dialogue (safety net). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Interaction", meta=(ClampMin="50",
		ToolTip="If the player walks farther than this during dialogue, the NPC ends the interaction."))
	float MaxInteractDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="100", ToolTip="Distance from threat at which AI considers itself safe.")) float FleeSafeDistance = 2000.f;

	/** When the threat is closer than this, the AI re-evaluates its flee path every tick instead of waiting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="50", ToolTip="Panic radius. AI recalculates flee direction every tick when threat is this close."))
	float FleePanicRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="0.1", ClampMax="3.0", ToolTip="Normal interval between flee direction recalculations.")) float FleeReevalInterval = 0.5f;

	/** Recalc interval while in panic (threat very close). Fast but not every-frame, to avoid a NavMesh query storm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="0.05", ClampMax="0.5",
		ToolTip="How often a panicking AI recomputes its flee path. ~0.12s = responsive without a per-frame nav storm."))
	float FleePanicReevalInterval = 0.12f;

	/** If the AI can't reach its spawn within this many seconds while Returning, it teleports home (anti-soft-lock). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="1.0",
		ToolTip="Max seconds spent returning before a hard teleport home. Prevents getting stuck on an unreachable spawn point."))
	float ReturnTimeout = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat", meta=(ToolTip="Fallback attack range if no CombatComponent attacks.")) float AttackRange = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat") float AttackCooldown = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ToolTip="Rotation interpolation speed toward target. Lower = smoother.")) float FaceTargetRotationSpeed = 8.f;

	/** Brief size-up delay the first time the AI reaches a NEW target before its first attack (a random 0.15..this).
	 *  Stops the AI swinging the exact frame it arrives in range (reads as aimbot). 0 = attack immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat", meta=(ClampMin="0", ClampMax="2",
		ToolTip="Max first-contact size-up delay before the first attack on a new target. 0 = none."))
	float EngageReactionTime = 0.4f;

	/* ── Custom / scripted offense (exotic bosses) ── */

	/** Hand ALL offense to Blueprint. When ON, the built-in approach/slot/token/evade/kite/melee loop is DISABLED:
	 *  the AI locks into the Attacking state as soon as it has a target and calls TickCustomAttackLogic every frame,
	 *  so you script the whole fight from BP (attack sequences, arena-wide hazards, stationary bosses, etc.).
	 *  Tip: pair with a large SightRadius, TargetMemoryDuration=0 (relentless), and (for stationary bosses) a pawn
	 *  that never moves. The break/poise gauge, phases, and dispatchers all still work. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Custom",
		meta=(ToolTip="Disable the built-in melee brain and script attacks from BP via TickCustomAttackLogic. For boss/scripted AI."))
	bool bUseCustomAttackLogic = false;

	/** While bUseCustomAttackLogic is ON, let the engine yaw the body to face the target each frame.
	 *  Leave OFF if you control facing yourself in the animation/BP (e.g. a hand-animated giant boss). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Custom", meta=(EditCondition="bUseCustomAttackLogic",
		ToolTip="Auto-rotate to face the target while using custom attack logic. Off = you handle facing in anim/BP."))
	bool bCustomLogicFacesTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Range", meta=(ClampMin="0.3", ClampMax="1.0",
		ToolTip="How close the AI stands to attack, as a fraction of attack range. LOWER = gets right in the target's face (aggressive melee); higher = hangs back near max reach. 0.6 of a 300 range = stands ~180 out."))
	float CombatEngageRangeRatio = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Range", meta=(ClampMin="1.05", ClampMax="3.0",
		ToolTip="How far past attack range the AI keeps re-approaching before switching back to a full chase."))
	float CombatDisengageRangeRatio = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0.5", ClampMax="1.15",
		ToolTip="Inner leash ratio. A leashed/returning AI gives up at LeashRange*1.15 but only RE-ENGAGES once back within LeashRange*this. Keep below 1.15 to leave a no-flip dead band (stops boundary stutter)."))
	float LeashReengageRatio = 0.95f;

	/* ── Crowd avoidance (Detour) ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Avoidance",
		meta=(ToolTip="Enable Detour Crowd avoidance so groups of AI flow around each other instead of overlapping."))
	bool bUseCrowdAvoidance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Avoidance", meta=(EditCondition="bUseCrowdAvoidance", ClampMin="0",
		ToolTip="How strongly AI push apart from each other. Higher = more personal space."))
	float CrowdSeparationWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Avoidance", meta=(EditCondition="bUseCrowdAvoidance", ClampMin="0.1",
		ToolTip="Scales how far ahead the AI looks to avoid others."))
	float CrowdAvoidanceRangeMultiplier = 1.0f;

	/* ── Combat director (attack tokens) ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director",
		meta=(ToolTip="Limit how many AI may attack the SAME target at once. Others circle and wait their turn."))
	bool bUseAttackTokens = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bUseAttackTokens", ClampMin="1",
		ToolTip="Max number of AI that may be ASSIGNED an attack slot on the same target at once. Any beyond this hold their encirclement spot, face the target and menace (MenaceMontage), waiting a slot to free — they do NOT back away. Raise for swarms, lower (2-3) only if big bodies crowd."))
	int32 MaxSimultaneousAttackers = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bUseAttackTokens", ClampMin="0.5",
		ToolTip="Safety auto-release for an attack turn if the AI never releases it (death, dormancy, etc.)."))
	float AttackTokenLeaseDuration = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bUseAttackTokens", ClampMin="0.0",
		ToolTip="Minimum delay between attack STARTS among ALL enemies on the same target. Staggers the group so they don't swing in unison. LOWER = more bites land per second (aggressive pack). 0 = no pacing (everyone swings freely)."))
	float AttackInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bUseAttackTokens", ClampMin="1.0", ClampMax="3.0",
		ToolTip="Enemies waiting for an attack turn hold at this multiple of attack range instead of crowding the target."))
	float WaitRingRatio = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director",
		meta=(ToolTip="Coordinate WHERE each attacker stands: each reserves a distinct angular lane around the target (via the combat director) so they surround it instead of stacking on one side."))
	bool bCoordinateAttackSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bCoordinateAttackSlots", ClampMin="10", ClampMax="180",
		ToolTip="Minimum angular spacing (degrees) the director keeps between two attackers around the same target. ~360/this = how many fit around it (e.g. 60 = up to 6)."))
	float SlotSeparationDegrees = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(ClampMin="0.5",
		ToolTip="Seconds between menace/howl montages while an AI holds back waiting its turn to attack (needs MenaceMontage set)."))
	float CombatWaitMenaceInterval = 3.f;

	/** Chance to hold/bait instead of attacking when the target is defending (ABaseAICharacter::IsTargetDefending). 0 = always attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(ClampMin="0", ClampMax="1",
		ToolTip="When the target is blocking/parrying, chance to wait instead of feeding the attack. Requires IsTargetDefending implemented in BP."))
	float DefenseBaitChance = 0.5f;

	/** Once an AI decides to bait, it commits to holding for this long (no per-frame re-roll) — reads as deliberate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(ClampMin="0.05", ClampMax="2.0",
		ToolTip="Duration of a committed bait hold against a defending target before re-evaluating."))
	float BaitHoldDuration = 0.45f;

	/* ── Reactive evade (Elite/Boss; requires ABaseAICharacter::IsTargetWindingUpAttack implemented in BP) ── */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Evade",
		meta=(ToolTip="Let Elite/Boss AI sidestep/back-hop when the target winds up an attack. Needs IsTargetWindingUpAttack in BP."))
	bool bUseReactiveEvade = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Evade", meta=(EditCondition="bUseReactiveEvade", ClampMin="0", ClampMax="1",
		ToolTip="Chance to dodge when the target is winding up (rolled when off cooldown)."))
	float EvadeChance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Evade", meta=(EditCondition="bUseReactiveEvade", ClampMin="0.1",
		ToolTip="Minimum delay between dodges."))
	float EvadeCooldown = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Evade", meta=(EditCondition="bUseReactiveEvade", ClampMin="0.1", ClampMax="1.0",
		ToolTip="How long the dodge movement is committed."))
	float EvadeDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Evade", meta=(EditCondition="bUseReactiveEvade", ClampMin="50",
		ToolTip="Distance of the sidestep/back-hop."))
	float EvadeDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Evade", meta=(EditCondition="bUseReactiveEvade", ClampMin="100",
		ToolTip="Burst speed of the dodge."))
	float EvadeSpeed = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Idle") float IdleAnimInterval = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Idle", meta=(ClampMin="0")) float IdleAnimRandomDeviation = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="50", ToolTip="Speed while repositioning around the target between attacks.")) float StrafeSpeed = 320.f;

	/** If the target gets closer than this, the AI backs off to keep distance (kiting). 0 = glue to target (melee). Set for ranged enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="0",
		ToolTip="Minimum comfortable distance to the target. The AI kites away if the target gets closer. 0 = no minimum."))
	float MinComfortRange = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="0.1", ToolTip="How often the AI re-picks its free attack position (slot) around the target.")) float SlotUpdateInterval = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="5", ToolTip="How close the AI must get to its chosen attack position before it stops. Tighter = more precise surrounding.")) float CombatPositionAcceptance = 35.f;

	/** Delay (seconds) between OnPossess and the first patrol kickoff. Gives BeginPlay time to wire up the
	 *  patrol spline and gives the navmesh time to be ready. Lower = snappier; too low and Path mode breaks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Init", meta=(ClampMin="0.0", ClampMax="2.0",
		ToolTip="Delay before initial patrol kicks off after the controller possesses the pawn. Required for Path mode."))
	float InitialPatrolDelay = 0.3f;

private:
	void SetupPerception();
	void CheckProximityDetection();
	void TickDetection(float DeltaTime);
	/** True if the current target is genuinely sensed right now (sight active, or point-blank) — vs tracked through walls. */
	bool IsTargetCurrentlySeen(AActor* Target);
	void BeginInvestigate(const FVector& Location);
	void DrawDebugPerception() const;
	float GetEffectiveAttackRange() const;

	float GetChaseSpeed() const;
	void ApproachTarget(AActor* Target, float DesiredDistance);
	float GetCombatApproachDistance(float Range) const;

	/** Combined collision reach (this AI's capsule radius + the target's) so attack range is measured body-surface to
	 *  body-surface, not center to center — a large creature otherwise can't close inside its own capsule to attack. */
	float GetCombatReach(const AActor* Target) const;

	FVector GetAttackSlotLocation(AActor* Target, float Radius, float DeltaTime);
	float ComputeFreeSlotAngle(AActor* Target, float Radius);

	void ConfigureCrowdAvoidance();

	UAICombatDirectorSubsystem* GetCombatDirector() const;
	bool TryTakeAttackTurn(AActor* Target);
	void ReleaseAttackTokenHeld();
	bool HasUsableAttack(const UAICombatComponent* Combat, float Distance) const;

	/** Group attack pacing — read-only check (delegates to the director's IsAttackWindowOpen with AttackInterval). */
	bool IsAttackWindowOpen(AActor* Target) const;

	/** Stamp the group attack window — call only when an attack actually fires. */
	void NotifyAttackStarted(AActor* Target);

	/** Deferred patrol kickoff. Called via timer after OnPossess so the character has finished its own BeginPlay.
	 *  Re-arms itself (bounded) until a navmesh is ready near the pawn, so a cooked/streamed nav that arrives a beat
	 *  late doesn't leave the AI stuck idle forever. */
	void TryStartInitialPatrol();

	/** True if there is navigable navmesh near Loc right now (used to defer patrol until the nav data is ready). */
	bool IsNavmeshReadyNear(const FVector& Loc) const;

	UPROPERTY() TObjectPtr<ABaseAICharacter> AICharacter;
	UPROPERTY() TObjectPtr<UAISenseConfig_Sight> SightConfig;
	UPROPERTY() TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	float AttackTimer = 0.f;
	float IdleTimer = 0.f;
	float NextIdleAnimTime = 5.f;
	float ProximityTimer = 0.f;
	float FleeReevalTimer = 0.f;
	float LostSightTimer = -1.f;
	float ReturnTimer = 0.f;
	bool bFleeCornered = false; // last FleeFrom found no nav escape → hold and face the threat
	FVector LastKnownLocation = FVector::ZeroVector; // where we last actually sensed the target (anti wall-hack chase)
	TWeakObjectPtr<AActor> LastKnownActor;          // which target LastKnownLocation belongs to
	float DetectionReactionScale = 1.f; // per-attempt reaction-time variance for the detection ramp

	/* ── Detection ramp / investigate runtime ── */
	TWeakObjectPtr<AActor> PendingDetectTarget;
	float DetectionProgress = 0.f;
	TWeakObjectPtr<AActor> ProximitySeen;   // last actor the proximity poll flagged (feeds the detection ramp)
	float ProximitySeenTime = -1000.f;
	FVector InvestigateLocation = FVector::ZeroVector;
	float InvestigateTimer = 0.f;
	TWeakObjectPtr<AActor> NoticedActor;
	float NoticeTimer = 0.f;
	float BaitTimer = 0.f;          // committed bait hold remaining
	float ReacquireCooldown = 0.f;  // throttle for ReacquireOnReturn during investigate
	float SuspicionTimer = 0.f;     // lifetime of a heard-noise Suspicious reaction
	TWeakObjectPtr<AActor> EngagedTarget; // target we've already sized up (first-contact reaction)
	float EngageReactionTimer = 0.f;
	float ScanGoalYaw = 0.f;        // idle look-around target heading
	float ScanTimer = 0.f;
	bool bEvading = false;          // committed to a reactive dodge
	float EvadeTimer = 0.f;
	float EvadeCooldownTimer = 0.f;
	float MenaceTimer = 0.f;        // throttle for the wait-your-turn menace/howl montage

	float CachedSlotAngle = 0.f;
	float SlotTargetAngle = 0.f; // chosen slot angle the cached angle smoothly rotates toward (hysteresis)
	float SlotTimer = 0.f;
	bool bHasCachedSlot = false;
	FVector SpawnOrigin = FVector::ZeroVector;

	bool bHoldingAttackToken = false;
	TWeakObjectPtr<AActor> TokenTarget;

	FTimerHandle InitialPatrolTimerHandle;
	int32 InitialPatrolAttempts = 0; // bounded nav-ready retries before patrolling anyway
	static constexpr int32 MaxInitialPatrolAttempts = 40; // ~20s at 0.5s intervals
};