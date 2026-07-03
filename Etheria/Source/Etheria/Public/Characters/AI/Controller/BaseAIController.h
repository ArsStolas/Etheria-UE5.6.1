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

	UFUNCTION(BlueprintCallable, Category="AI|Perception") void InvestigateThreat(AActor* Threat);

	UFUNCTION(BlueprintCallable, Category="AI|Perception") void Investigate(const FVector& Location);

	UFUNCTION(BlueprintCallable, Category="AI|Perception") void NoticeActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="AI") void ReconcileHostility();

	UFUNCTION(BlueprintPure, Category="AI") bool IsThreatInTerritory(const AActor* Threat) const;

	bool IsSelfOutsideLeash() const;

	bool ReacquireOnReturn();

	void NotifyTargetConfirmedByDamage(AActor* InstigatorActor);

	void NotifyPackMateDied(bool bAlphaDied, bool bLastSurvivor);

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
	void FaceTargetYawOnly(AActor* Target, float DeltaTime, float RotationSpeedOverride = -1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Hearing", meta=(ToolTip="360-degree hearing radius.")) float HearingRange = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection", meta=(ClampMin="100",
		ToolTip="Single detection radius: within this (360°) + line of sight, the AI detects a valid target. The decal shows this.")) float DetectionRadius = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection", meta=(ClampMin="0",
		ToolTip="Reaction delay before locking on at the edge of DetectionRadius (closer targets acquire faster). 0 = instant.")) float DetectionReactionTime = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection", meta=(ClampMin="100",
		ToolTip="Engaged tracking radius (should be > DetectionRadius). A chased target is only 'lost' beyond this."))
	float LoseSightRadius = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection", meta=(ClampMin="0.05", ClampMax="1.0",
		ToolTip="Detection fraction at which the AI becomes Suspicious (turn-to-look / bark via OnAwarenessChanged)."))
	float SuspiciousThreshold = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection", meta=(ClampMin="0.05", ClampMax="1.0",
		ToolTip="Detection fraction at which the AI escalates to Alert (growl/stance beat) just before engaging."))
	float AlertThreshold = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception", meta=(ClampMin="0",
		ToolTip="Seconds an engaged AI keeps chasing/attacking after losing line of sight before giving up. 0 = never give up (relentless; only the leash brings it home). Set 0 deliberately for bosses."))
	float TargetMemoryDuration = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception", meta=(ClampMin="0", ClampMax="2",
		ToolTip="Seconds of the target's last velocity extrapolated past the last-seen point during a lost-sight pursuit."))
	float LostSightPredictTime = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception", meta=(ClampMin="0",
		ToolTip="Scent tracking: after the sight memory expires, this AI follows the target's TRAIL (its position from a few seconds ago) for this many seconds before giving up. Softer than infinite sight, very predator-like. 0 = off."))
	float ScentTrackDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception", meta=(ClampMin="0.5", ClampMax="5",
		ToolTip="How far BEHIND the target the scent trail is (seconds). Higher = the trail is colder, easier to escape."))
	float ScentTrailDelay = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate",
		meta=(ToolTip="When the AI hears a noise (and isn't already chasing), it walks to investigate the source instead of instantly locking on through walls."))
	bool bInvestigateNoises = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate", meta=(EditCondition="bInvestigateNoises", ClampMin="0.5",
		ToolTip="Seconds to search around a heard noise before giving up and returning."))
	float InvestigateDuration = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate", meta=(EditCondition="bInvestigateNoises", ClampMin="50",
		ToolTip="How far around the noise the AI wanders while searching."))
	float InvestigateSearchRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate", meta=(ClampMin="0.1",
		ToolTip="Seconds an idle NPC keeps facing a noticed actor before losing interest."))
	float NoticeDuration = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate", meta=(ClampMin="50",
		ToolTip="Idle NPCs only turn to look at someone INSIDE this range (~1.8m). Farther away they keep their own facing (no across-the-plaza staring). They release the look when the actor walks back out."))
	float NoticeDistance = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Perception|Investigate", meta=(ClampMin="0.1",
		ToolTip="How long an NPC stays edgy after hearing a noise, so a bark/posture (OnAwarenessChanged) can actually play."))
	float SuspicionDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Interaction", meta=(ClampMin="50",
		ToolTip="If the player walks farther than this during dialogue, the NPC ends the interaction."))
	float MaxInteractDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="100", ToolTip="Distance from threat at which AI considers itself safe.")) float FleeSafeDistance = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="50", ToolTip="Panic radius. AI recalculates flee direction every tick when threat is this close."))
	float FleePanicRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="0.1", ClampMax="3.0", ToolTip="Normal interval between flee direction recalculations.")) float FleeReevalInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="0.05", ClampMax="0.5",
		ToolTip="How often a panicking AI recomputes its flee path. ~0.12s = responsive without a per-frame nav storm."))
	float FleePanicReevalInterval = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="0",
		ToolTip="Seconds a fleeing AI keeps running after losing sight of the threat before calming and returning home. 0 = calm instantly."))
	float FleeMemoryDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Flee", meta=(ClampMin="0", ClampMax="3",
		ToolTip="After reaching safe distance, seconds the prey pauses and looks back before calm-returning home. 0 = return immediately."))
	float FleeCalmDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="1.0",
		ToolTip="Max seconds spent returning before a hard teleport home. Prevents getting stuck on an unreachable spawn point."))
	float ReturnTimeout = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat", meta=(ClampMin="0",
		ToolTip="Fallback attack range if there's no CombatComponent with attacks. Normally the attack's own Range drives combat.")) float AttackRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat", meta=(ClampMin="0.3", ClampMax="0.95",
		ToolTip="Fraction of its attack range the AI closes to before striking (0.75 = stops at 75% of range, safely inside so hits always land).")) float ApproachPercent = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat", meta=(ClampMin="20", ClampMax="500",
		ToolTip="Max body-gap a melee AI keeps to strike, regardless of attack Range (raising Range must not push it away). Ranged style ignores this."))
	float MaxMeleeStandoff = 110.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ToolTip="Rotation interpolation speed toward target. Lower = smoother.")) float FaceTargetRotationSpeed = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="1",
		ToolTip="Yaw turn speed while engaged (between swings). Higher squares up faster to a moving target.")) float CombatFaceRotationSpeed = 11.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="0",
		ToolTip="Slow yaw tracking during the wind-up (before the aim lock). 0 = the whole swing is aim-frozen."))
	float WindupTrackRotationSpeed = 4.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="0", ClampMax="1",
		ToolTip="Aim freezes this long before the hit frame — the guaranteed dodge window."))
	float AttackAimLockTime = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="0", ClampMax="1.5",
		ToolTip="Orbit speed around the target while waiting between attacks (rad/s). 0 = stand still."))
	float CombatOrbitSpeed = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat", meta=(ClampMin="0", ClampMax="2",
		ToolTip="Max first-contact size-up delay before the first attack on a new target. 0 = none."))
	float EngageReactionTime = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Custom",
		meta=(ToolTip="Disable the built-in melee brain and script attacks from BP via TickCustomAttackLogic. For boss/scripted AI."))
	bool bUseCustomAttackLogic = false;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director",
		meta=(ToolTip="Limit how many AI may attack the SAME target at once. Others circle and wait their turn."))
	bool bUseAttackTokens = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bUseAttackTokens", ClampMin="1",
		ToolTip="Max number of AI that may be ASSIGNED an attack slot on the same target at once. Any beyond this hold their encirclement spot, face the target and menace (MenaceMontage), waiting a slot to free — they do NOT back away. Raise for swarms, lower (2-3) only if big bodies crowd."))
	int32 MaxSimultaneousAttackers = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bUseAttackTokens", ClampMin="0.5",
		ToolTip="Safety auto-release for an attack turn if the AI never releases it (death, dormancy, etc.)."))
	float AttackTokenLeaseDuration = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bUseAttackTokens", ClampMin="0.0",
		ToolTip="Minimum delay between attack STARTS among ALL enemies on the same target. Staggers the group so they don't swing in unison. LOWER = more bites land per second (aggressive pack). 0 = no pacing (everyone swings freely)."))
	float AttackInterval = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bUseAttackTokens", ClampMin="0",
		ToolTip="Group-wide breather after a CONNECTED hit before anyone swings again. Stops chain-hit stunlocks. 0 = off."))
	float AttackHitGrace = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director",
		meta=(ToolTip="Coordinate WHERE each attacker stands: each reserves a distinct angular lane around the target (via the combat director) so they surround it instead of stacking on one side."))
	bool bCoordinateAttackSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(EditCondition="bCoordinateAttackSlots", ClampMin="10", ClampMax="180",
		ToolTip="Minimum angular spacing (degrees) the director keeps between two attackers around the same target. ~360/this = how many fit around it (e.g. 60 = up to 6)."))
	float SlotSeparationDegrees = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(ClampMin="0.5",
		ToolTip="Seconds between menace/howl montages while an AI holds back waiting its turn to attack (needs MenaceMontage set)."))
	float CombatWaitMenaceInterval = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(ClampMin="0", ClampMax="1",
		ToolTip="When the target is blocking/parrying, chance to wait instead of feeding the attack. Requires IsTargetDefending implemented in BP."))
	float DefenseBaitChance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Director", meta=(ClampMin="0.05", ClampMax="2.0",
		ToolTip="Duration of a committed bait hold against a defending target before re-evaluating."))
	float BaitHoldDuration = 0.45f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="50",
		ToolTip="Speed while repositioning around the target between attacks. Keep UNDER the animation RunSpeedThreshold so orbiting plays the strafe/walk clips (a deliberate stalk), not a sideways run.")) float StrafeSpeed = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="0",
		ToolTip="Minimum comfortable distance to the target. The AI kites away if the target gets closer. 0 = no minimum."))
	float MinComfortRange = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="0.1", ToolTip="How often the AI re-picks its free attack position (slot) around the target.")) float SlotUpdateInterval = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Combat|Movement", meta=(ClampMin="5", ToolTip="How close the AI must get to its chosen attack position before it stops. Tighter = more precise surrounding.")) float CombatPositionAcceptance = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Init", meta=(ClampMin="0.0", ClampMax="2.0",
		ToolTip="Delay before initial patrol kicks off after the controller possesses the pawn. Required for Path mode."))
	float InitialPatrolDelay = 0.3f;

private:
	void SetupPerception();

	void UpdateDetection(float DeltaTime);

	AActor* FindPerceptibleTarget(float Radius) const;

	bool IsTargetCurrentlySeen(AActor* Target);

	void StartSearchAtLastKnown();
	void BeginInvestigate(const FVector& Location, bool bUrgent = false);
	void DrawDebugPerception() const;
	float GetEffectiveAttackRange() const;

	UFUNCTION() void HandleOwnAttackResolved(const FAIAttackData& Attack, AActor* Target, bool bHitConnected);

	float GetChaseSpeed() const;
	void ApproachTarget(AActor* Target, float DesiredDistance);
	float GetCombatApproachDistance(float Range) const;

	float GetCapsuleScale() const;

	void SteerToPoint(const FVector& Point, float MaxSpeed, float SettleRadius);

	float GetCombatReach(const AActor* Target) const;

	FVector GetAttackSlotLocation(AActor* Target, float Radius, float DeltaTime);
	float ComputeFreeSlotAngle(AActor* Target, float Radius);

	void ConfigureCrowdAvoidance();

	UAICombatDirectorSubsystem* GetCombatDirector() const;
	bool TryTakeAttackTurn(AActor* Target);
	void ReleaseAttackTokenHeld();
	bool HasUsableAttack(const UAICombatComponent* Combat, float Distance) const;

	bool IsAttackWindowOpen(AActor* Target) const;

	void NotifyAttackStarted(AActor* Target);

	void TryStartInitialPatrol();

	bool IsNavmeshReadyNear(const FVector& Loc) const;

	UPROPERTY() TObjectPtr<ABaseAICharacter> AICharacter;
	UPROPERTY() TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	float IdleTimer = 0.f;
	float NextIdleAnimTime = 5.f;
	float FleeReevalTimer = 0.f;
	float LostSightTimer = -1.f;
	float ReturnTimer = 0.f;
	float FleeCalmTimer = 0.f;
	float FleeLostSightTimer = -1.f;
	bool bFleeCornered = false;
	bool bFleeingHome = false;
	FVector LastKnownLocation = FVector::ZeroVector;
	FVector LastKnownVelocity = FVector::ZeroVector;
	TWeakObjectPtr<AActor> LastKnownActor;
	float DetectionReactionScale = 1.f;

	TWeakObjectPtr<AActor> PendingDetectTarget;
	float DetectionProgress = 0.f;
	float DetectionScanTimer = 0.f;
	TWeakObjectPtr<AActor> CachedDetectionCandidate;
	FVector InvestigateLocation = FVector::ZeroVector;
	float InvestigateTimer = 0.f;
	bool bInvestigateUrgent = false;
	bool bInvestigatePausing = false;
	float InvestigateScanPause = 0.f;
	float InteractGestureTimer = 0.f;
	TWeakObjectPtr<AActor> NoticedActor;
	float NoticeTimer = 0.f;
	float IdleAnchorYaw = 0.f;
	bool bIdleAnchorSet = false;
	float BaitTimer = 0.f;
	bool bDefendObserved = false;
	bool bBaitCommitted = false;
	float ReacquireCooldown = 0.f;
	float SuspicionTimer = 0.f;
	TWeakObjectPtr<AActor> EngagedTarget;
	float EngageReactionTimer = 0.f;
	float ScanGoalYaw = 0.f;
	float ScanTimer = 0.f;
	bool bEvading = false;
	float EvadeTimer = 0.f;
	float EvadeCooldownTimer = 0.f;
	bool bWindupObserved = false;
	bool bEvadeCommitted = false;
	float EvadeReactDelay = 0.f;
	float MenaceTimer = 0.f;
	float PlannedAttackRange = -1.f;
	float PlannedRangeTimer = 0.f;
	float CombatStallTimer = 0.f;
	bool bSteerCombat = false;
	bool bSteerSettled = false;
	bool bStallRepath = false;
	float StallRepathTime = 0.f;
	FVector EvadeDir = FVector::ZeroVector;
	float PlannedMinRange = 0.f;
	float HarassRetreatTimer = 0.f;
	float ScentTimer = -1.f;
	TArray<TPair<float, FVector>> ScentTrail;
	TWeakObjectPtr<AActor> LastScentTarget;
	FVector LastCandidateLocation = FVector::ZeroVector;
	bool bPackEnraged = false;
	bool bPackDemoralized = false;
	float BasePackAttackInterval = -1.f;
	float BasePackStrafeSpeed = -1.f;
	float BasePackEvadeChance = -1.f;
	float BasePackChaseSpeed = -1.f;
	float OrbitDir = 1.f;
	float OrbitDirTimer = 0.f;

	float CachedSlotAngle = 0.f;
	float SlotTargetAngle = 0.f;
	float SlotTimer = 0.f;
	bool bHasCachedSlot = false;
	FVector SpawnOrigin = FVector::ZeroVector;

	bool bHoldingAttackToken = false;
	TWeakObjectPtr<AActor> TokenTarget;

	FTimerHandle InitialPatrolTimerHandle;
	int32 InitialPatrolAttempts = 0;
	static constexpr int32 MaxInitialPatrolAttempts = 40;

	bool bHomeAnchored = false;
};
