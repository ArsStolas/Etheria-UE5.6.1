/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "BaseAICharacter - Header"
 * Notes: Pack, respawn, dormancy (timer-based), detection decal, hit reactions,
 *        death VFX + dissolve fade, flee/fight-back.
 */

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Characters/AI/AI_Types.h"
#include "Interfaces/Interaction.h"
#include "BaseAICharacter.generated.h"

class UAIMovementComponent;
class UAIAnimationComponent;
class UAICombatComponent;
class ABaseAIController;
class USplineComponent;
class UDecalComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class UNiagaraComponent;
class UHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIStateChanged, EAIState, OldState, EAIState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetAcquired, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetLost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIDamaged, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAwarenessChanged, EAIAwarenessLevel, Old, EAIAwarenessLevel, New);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPackAlerted, ABaseAICharacter*, Alerter, AActor*, Threat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIRespawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIDormancyChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIDeathFadeStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIDeathFadeCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIDamageBlocked, AActor*, Instigator, float, AttemptedDamage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAIInteractionStarted, AActor*, Interactor, EAINPCRole, Role, FName, DialogueID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIInteractionEnded);

UCLASS()
class ETHERIA_API ABaseAICharacter : public ABaseCharacter, public IInteraction
{
	GENERATED_BODY()

public:
	ABaseAICharacter();

	/* ═══════════ Component Getters ═══════════ */
	UFUNCTION(BlueprintPure, Category="AI") UAIMovementComponent* GetAIMovement() const { return AIMovementComponent; }
	UFUNCTION(BlueprintPure, Category="AI") UAIAnimationComponent* GetAIAnimation() const { return AIAnimationComponent; }
	UFUNCTION(BlueprintPure, Category="AI") UAICombatComponent* GetAICombat() const { return AICombatComponent; }
	UFUNCTION(BlueprintPure, Category="AI") USplineComponent* GetPatrolSpline() const { return PatrolSpline; }

	/* ═══════════ State Getters ═══════════ */
	UFUNCTION(BlueprintPure, Category="AI") EAIHostilityType GetHostilityType() const { return HostilityType; }
	UFUNCTION(BlueprintPure, Category="AI") EAIRank GetRank() const { return Rank; }
	UFUNCTION(BlueprintPure, Category="AI") EAIState GetCurrentAIState() const { return CurrentState; }
	UFUNCTION(BlueprintPure, Category="AI") AActor* GetCurrentTarget() const { return CurrentTarget; }
	UFUNCTION(BlueprintPure, Category="AI") FVector GetLastHitDirection() const { return LastHitDirection; }
	UFUNCTION(BlueprintPure, Category="AI") float GetLeashRange() const { return LeashRange; }
	UFUNCTION(BlueprintPure, Category="AI") bool CanFlee() const { return bCanFlee; }
	UFUNCTION(BlueprintPure, Category="AI") bool ShouldShowDebugPerception() const { return bShowDebugPerception; }
	UFUNCTION(BlueprintPure, Category="AI") FVector GetSpawnLocation() const { return SpawnLocation; }
	UFUNCTION(BlueprintPure, Category="AI") EAIAwarenessLevel GetAwarenessLevel() const { return AwarenessLevel; }
	UFUNCTION(BlueprintPure, Category="AI") FName GetPackID() const { return PackID; }
	UFUNCTION(BlueprintPure, Category="AI") bool IsDead() const { return CurrentState == EAIState::Dead; }
	UFUNCTION(BlueprintPure, Category="AI") bool IsDormant() const { return bIsDormant; }
	UFUNCTION(BlueprintPure, Category="AI") bool OnlyDetectsPlayers() const { return bOnlyDetectPlayers; }

	UFUNCTION(BlueprintPure, Category="AI") bool ShouldEngageTargets() const;

	/** True if this AI can ever act on a PERCEIVED actor (engage on sight, or flee from it). Inert NPCs
	 *  (passive/neutral that can't flee) return false — used to skip perception/proximity scans entirely. */
	UFUNCTION(BlueprintPure, Category="AI") bool CanReactToPerception() const;

	/** Shared target-acquisition filter: rejects self, other AI, and (when bOnlyDetectPlayers) non-players —
	 *  but always allows actors carrying a FleeFromTags tag (predators), so prey can detect them. */
	UFUNCTION(BlueprintPure, Category="AI") bool IsValidTargetCandidate(AActor* Candidate) const;

	/** True if Other carries one of this AI's FleeFromTags (i.e. it's a predator to flee). */
	UFUNCTION(BlueprintPure, Category="AI") bool MatchesFleeTag(const AActor* Other) const;

	/** True if a target is gone or dead (invalid, AI IsDead, or has a dead HealthComponent) — stop attacking it. */
	UFUNCTION(BlueprintPure, Category="AI") bool IsTargetDeadOrInvalid(const AActor* Target) const;

	UFUNCTION(BlueprintPure, Category="AI") bool ShouldShowDebugPatrol() const { return bShowDebugPatrol; }
	UFUNCTION(BlueprintPure, Category="AI|Damage") bool GetCanReceiveDamage() const { return bCanReceiveDamage; }

	/* ═══════════ Setters ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI") void SetAIState(EAIState NewState);
	UFUNCTION(BlueprintCallable, Category="AI") void SetTarget(AActor* NewTarget);
	UFUNCTION(BlueprintCallable, Category="AI") void ClearTarget();
	UFUNCTION(BlueprintCallable, Category="AI") void SetAwarenessLevel(EAIAwarenessLevel NewLevel);
	UFUNCTION(BlueprintCallable, Category="AI") void SetHostilityType(EAIHostilityType NewType);

	/** Add threat for an actor (damage auto-adds it; call directly for taunts/abilities). The highest-threat
	 *  actor steals aggro once it beats the current target by ThreatSwitchMargin. */
	UFUNCTION(BlueprintCallable, Category="AI|Threat") void AddThreat(AActor* Source, float Amount);

	/** Override in BP to report whether a target is actively defending (blocking / parrying / i-frames). The AI
	 *  uses this to BAIT — holding its attack instead of feeding a guard. Default: false (always attack). */
	UFUNCTION(BlueprintNativeEvent, Category="AI|Combat") bool IsTargetDefending(AActor* Target) const;
	virtual bool IsTargetDefending_Implementation(AActor* Target) const { return false; }

	/** Override in BP to report whether the target is currently winding up an attack. Lets Elite/Boss AI
	 *  sidestep/back-hop to dodge it. Default: false (never dodges until the player-combat side reports wind-ups). */
	UFUNCTION(BlueprintNativeEvent, Category="AI|Combat") bool IsTargetWindingUpAttack(AActor* Target) const;
	virtual bool IsTargetWindingUpAttack_Implementation(AActor* Target) const { return false; }

	/** Toggle damage immunity at runtime. Useful for cutscenes, scripted moments, or boss invulnerability phases. */
	UFUNCTION(BlueprintCallable, Category="AI|Damage") void SetCanReceiveDamage(bool bNewCanReceiveDamage);

	/* ═══════════ Perception / Damage ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI") void OnPerceiveTarget(AActor* PerceivedActor);
	UFUNCTION(BlueprintCallable, Category="AI") void OnReceiveDamage(AActor* DamageInstigator, float DamageAmount);

	/* ═══════════ Interaction (IInteraction) ═══════════ */
	virtual void Interact_Implementation(AActor* Target) override;
	virtual void CanReceiveTrace_Implementation() override;

	/** Start an interaction: stops, faces the interactor, plays the interaction montage, broadcasts OnInteractionStarted. */
	UFUNCTION(BlueprintCallable, Category="AI|Interaction") void BeginInteraction(AActor* Interactor);
	/** End the interaction and resume Idle/Patrol. */
	UFUNCTION(BlueprintCallable, Category="AI|Interaction") void EndInteraction();
	UFUNCTION(BlueprintPure, Category="AI|Interaction") AActor* GetInteractionPartner() const { return InteractionPartner; }
	UFUNCTION(BlueprintPure, Category="AI|Interaction") bool IsInteractable() const { return bIsInteractable; }

	UPROPERTY(BlueprintAssignable, Category="AI|Interaction") FOnAIInteractionStarted OnInteractionStarted;
	UPROPERTY(BlueprintAssignable, Category="AI|Interaction") FOnAIInteractionEnded OnInteractionEnded;

	/* ═══════════ Pack ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI|Pack") void AlertPack(AActor* Threat);
	UFUNCTION(BlueprintCallable, Category="AI|Pack") void OnPackAlert(ABaseAICharacter* Alerter, AActor* Threat);
	UFUNCTION(BlueprintCallable, Category="AI|Pack") TArray<ABaseAICharacter*> GetPackMembers() const;

	/** Tell idle pack members to go search a location (e.g. where this AI last saw a now-lost target). */
	UFUNCTION(BlueprintCallable, Category="AI|Pack") void AlertPackSearch(const FVector& Location);
	UFUNCTION(BlueprintPure, Category="AI|Pack") ABaseAICharacter* GetPackLeader() const;
	UFUNCTION(BlueprintPure, Category="AI|Pack") bool IsPackLeader() const;

	/* ═══════════ Leash / Respawn ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI") void TeleportToSpawn(FVector OverrideLocation = FVector::ZeroVector);
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void Die();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void Respawn();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void NotifyPlayerSaved();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void NotifyDayCycleComplete();

	/** Called by a day/night manager when the time of day changes (0..1). Override in BP for roosting, going home, schedules. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AI|Schedule") void OnTimeOfDayChanged(float NormalizedTime);
	virtual void OnTimeOfDayChanged_Implementation(float NormalizedTime) {}

	/* ═══════════ Optimization ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI|Optimization") void SetDormant(bool bDormant);

	/** Manually force a dormancy re-evaluation (e.g. after teleporting the player). */
	UFUNCTION(BlueprintCallable, Category="AI|Optimization") void ForceDormancyCheck() { UpdateDormancy(); }

	/* ═══════════ Detection Decal ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI|Debug") void SetDetectionDecalVisible(bool bVisible);

	/* ═══════════ Dispatchers ═══════════ */
	UPROPERTY(BlueprintAssignable, Category="AI") FOnAIStateChanged OnAIStateChanged;
	UPROPERTY(BlueprintAssignable, Category="AI") FOnTargetAcquired OnTargetAcquired;
	UPROPERTY(BlueprintAssignable, Category="AI") FOnTargetLost OnTargetLost;
	UPROPERTY(BlueprintAssignable, Category="AI") FOnAIDamaged OnAIDamaged;
	UPROPERTY(BlueprintAssignable, Category="AI") FOnAwarenessChanged OnAwarenessChanged;
	UPROPERTY(BlueprintAssignable, Category="AI|Pack") FOnPackAlerted OnPackAlerted;
	UPROPERTY(BlueprintAssignable, Category="AI|Respawn") FOnAIDied OnAIDied;
	UPROPERTY(BlueprintAssignable, Category="AI|Respawn") FOnAIRespawned OnAIRespawned;
	UPROPERTY(BlueprintAssignable, Category="AI|Optimization") FOnAIDormancyChanged OnAIDormancyChanged;

	/** Fired when the death VFX spawns and the fade-out begins. */
	UPROPERTY(BlueprintAssignable, Category="AI|Death") FOnAIDeathFadeStarted OnAIDeathFadeStarted;

	/** Fired when the fade-out completes (actor is now hidden). */
	UPROPERTY(BlueprintAssignable, Category="AI|Death") FOnAIDeathFadeCompleted OnAIDeathFadeCompleted;

	/** Fires when damage was rejected because bCanReceiveDamage is false. Hook this in BP to play
	 *  a "ting" sound, sparks, or a "shielded" tooltip — anything that signals "this attack didn't connect". */
	UPROPERTY(BlueprintAssignable, Category="AI|Damage") FOnAIDamageBlocked OnAIDamageBlocked;

	/** Override of AActor::TakeDamage. Returns 0 (and broadcasts OnAIDamageBlocked) when invulnerable
	 *  or already dead, which short-circuits HP loss, hit reactions, and the OnTakeAnyDamage broadcast
	 *  in one place — no need to add invulnerability checks anywhere else. */
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	/* ── Identity ── */

	/** Determines how this AI reacts to the player: Passive (no combat), Neutral (fights back), Aggressive (attacks on sight). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Identity")
	EAIHostilityType HostilityType = EAIHostilityType::Passive;

	/** Enemy rank. Only visible when Aggressive. Affects phase/combat behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Identity", meta=(EditCondition="HostilityType==EAIHostilityType::Aggressive"))
	EAIRank Rank = EAIRank::Basic;

	/* ── Behavior ── */

	/** If true, this AI can flee from threats (Passive/Neutral). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior",
		meta=(ToolTip="Enable flee behavior. When threatened, the AI will run away instead of fighting."))
	bool bCanFlee = false;

	/** If true, a fleeing Neutral AI will stop fleeing and fight back when it takes damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior",
		meta=(ToolTip="When a Neutral AI is fleeing and gets hit, it turns around and fights the attacker.", EditCondition="bCanFlee"))
	bool bFightBackWhenAttacked = true;

	/** Below this HP fraction, a flee-capable AI breaks off and runs even mid-fight (morale). 0 = never flee from low HP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0", ClampMax="1", EditCondition="bCanFlee",
		ToolTip="HP fraction (0-1) under which the AI panics and flees mid-fight. 0 = off. e.g. 0.25 = flee under 25% HP."))
	float FleeHealthThreshold = 0.f;

	/** After a morale break, suppress re-engaging for this long so a hit doesn't instantly cancel the flee (anti-flicker). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0", EditCondition="bCanFlee",
		ToolTip="Seconds after a morale-break flee during which the AI won't turn back to fight, even if hit."))
	float MoraleBreakCooldown = 4.f;

	/* ── Threat / aggro ── */

	/** Enable a threat table so the AI focuses whoever dealt the most damage / was taunted, instead of always
	 *  the nearest. Lets a tank hold aggro and lets co-op damage redirect the AI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Threat",
		meta=(ToolTip="Focus the highest-threat attacker instead of the nearest. Supports taunts (AddThreat) and co-op."))
	bool bUseThreatSystem = true;

	/** A challenger must exceed the current target's threat by this factor to steal aggro. Higher = stickier target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Threat", meta=(EditCondition="bUseThreatSystem", ClampMin="1.0",
		ToolTip="Threat factor needed to steal aggro. 1.5 = a challenger needs 50% more threat than the current target."))
	float ThreatSwitchMargin = 1.5f;

	/** Threat generated per point of damage taken. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Threat", meta=(EditCondition="bUseThreatSystem", ClampMin="0"))
	float ThreatPerDamage = 1.f;

	/** Threat decayed per second while engaged. 0 = threat never fades. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Threat", meta=(EditCondition="bUseThreatSystem", ClampMin="0",
		ToolTip="Threat lost per second so old attackers eventually stop holding aggro. 0 = permanent."))
	float ThreatDecayPerSecond = 2.f;

	/** If true, a passive/neutral NPC that notices the player (but won't fight or flee) turns to look at them briefly (curiosity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior",
		meta=(ToolTip="Idle NPCs turn to look at the player when noticed. Pure ambient flavor."))
	bool bNoticeReactions = true;

	/** Maximum distance from spawn point. AI teleports back if it exceeds this while chasing or fleeing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0",
		ToolTip="How far this AI can go from its spawn before it teleports back."))
	float LeashRange = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0.1",
		ToolTip="Duration (seconds) of the stagger triggered after taking StaggerThreshold hits."))
	float StaggerDuration = 1.f;

	/** If true, the player can interact with this AI (talk, quest, trade). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior",
		meta=(ToolTip="Enable interaction. When true, the player can talk to this AI."))
	bool bIsInteractable = false;

	/** What interacting with this NPC does (drives BP: dialogue UI / shop / quest). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Interaction", meta=(EditCondition="bIsInteractable"))
	EAINPCRole NPCRole = EAINPCRole::Dialogue;

	/** Dialogue/data id passed to BP on interaction (look up the conversation / shop inventory). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Interaction", meta=(EditCondition="bIsInteractable"))
	FName DialogueID = NAME_None;

	/* ── Damage ── */

	/** If true, this AI can take damage normally (HP loss, hit reaction, death). If false, all incoming
	 *  damage is rejected — no HP change, no animation, no death — and OnAIDamageBlocked fires instead.
	 *  Defaults to true so enemies are killable out of the box. Set to false for invulnerable NPCs
	 *  (merchants, story characters) or scripted invulnerability phases. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Damage",
		meta=(ToolTip="When OFF, the player (and anything else using ApplyDamage) cannot harm this AI. The AI plays no hit reaction, loses no HP, and OnAIDamageBlocked fires for feedback hooks."))
	bool bCanReceiveDamage = true;
	
	/* ── Component Movements ── */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UAIMovementComponent> AIMovementComponent;
	
	/* ── Detection ── */

	/** Tags that trigger flee (e.g. "Predator"). If empty, the AI flees from any detected actor (that passes the player filter).
	 *  Tagged actors are detectable even when bOnlyDetectPlayers is true — so prey can flee wolves while ignoring other NPCs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="Actor tags that make this AI flee (e.g. 'Predator'). Tagged actors are seen even when 'Only Detect Players' is on."))
	TArray<FName> FleeFromTags;

	/** When this AI starts fleeing, alarm nearby same-type NPCs so the herd scatters together. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="On flee, alert nearby NPCs of the same hostility within AlarmRadius so a herd panics together."))
	bool bAlarmsNearbyAllies = true;

	/** Radius for the flee alarm broadcast (independent of PackID). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection", meta=(EditCondition="bAlarmsNearbyAllies", ClampMin="0",
		ToolTip="How far a panic spreads to same-type NPCs when this one flees."))
	float AlarmRadius = 1000.f;

	/** If true, this AI only detects player-controlled pawns. Ignores other AI and NPCs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="Only react to player-controlled characters. Other AI are ignored."))
	bool bOnlyDetectPlayers = true;
	
	/* ── Pack ── */

	/** Pack identifier. AI with the same PackID form a group and share alerts. Leave empty for solo AI.
	 *  Read-only at runtime: it's captured into the pack registry at BeginPlay, so changing it live would desync. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Pack",
		meta=(ToolTip="Set the same PackID on multiple AI to make them a pack. They will alert and follow each other. Set in the editor, not at runtime."))
	FName PackID = NAME_None;

	/** Radius within which pack alerts are transmitted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(ClampMin="0",
		ToolTip="How far a pack alert travels. Only AI within this distance will be notified."))
	float PackAlertRadius = 3000.f;

	/** When this AI enters combat (gets hit OR starts attacking the player), pull nearby aggressive allies into the
	 *  fight too — no PackID needed. This is what makes a wolf pack react when you attack one of them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack",
		meta=(EditCondition="HostilityType==EAIHostilityType::Aggressive",
		ToolTip="When this AI engages, rally nearby aggressive allies onto the same target (independent of PackID). The reason packmates join in when you hit one of them."))
	bool bCallForHelpOnEngage = true;

	/** Radius for the combat rally (independent of PackID). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(EditCondition="bCallForHelpOnEngage", ClampMin="0",
		ToolTip="How far the 'call for help' reaches. Nearby aggressive allies within this distance join the fight."))
	float CombatAlertRadius = 1500.f;

	/** True = rallied allies engage immediately; false = they walk over to investigate and only commit once they see the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(EditCondition="bCallForHelpOnEngage",
		ToolTip="ON: allies aggro the target instantly (snappy pack). OFF: allies investigate the spot first and only fight once they see it (cautious, no aggro through walls)."))
	bool bRallyEngagesDirectly = true;

	/** Pack members try to stay this far from the leader when patrolling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(ClampMin="50",
		ToolTip="Ideal distance from the pack leader during patrol."))
	float PackFollowDistance = 400.f;

	/** Random spread around the leader to avoid stacking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(ClampMin="50",
		ToolTip="Random offset applied so pack members don't overlap."))
	float PackSpreadRadius = 300.f;

	/* ── Respawn ── */

	/** When and how this AI respawns after death. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Respawn",
		meta=(ToolTip="Choose the condition under which this AI will respawn after being killed."))
	EAIRespawnCondition RespawnCondition = EAIRespawnCondition::OnSaveOrDay;

	/** Respawn timer (seconds). Only used when RespawnCondition = OnTimer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Respawn",
		meta=(EditCondition="RespawnCondition==EAIRespawnCondition::OnTimer", ClampMin="1",
		ToolTip="Time in seconds before this AI respawns (only for Timer mode)."))
	float RespawnTimerDuration = 300.f;

	/** When RespawnCondition is Never, fully Destroy the actor after the death fade instead of leaving a hidden corpse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Respawn",
		meta=(ToolTip="If this AI never respawns, destroy it after the fade so it doesn't linger as a hidden actor."))
	bool bDestroyCorpseIfNoRespawn = true;

	/* ═══════════ Death VFX & Smooth Fade ═══════════ */

	/** Niagara VFX spawned near the end of the death montage to mask the disappearance (dust, ash, light, smoke...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|VFX",
		meta=(ToolTip="Niagara system spawned during the death sequence. Use to cover the fade-out with particles. Leave empty for no VFX."))
	TObjectPtr<UNiagaraSystem> DeathVFX;

	/** Mesh socket/bone where the VFX is attached. None = actor root. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|VFX",
		meta=(ToolTip="Mesh socket or bone the VFX attaches to. Leave None to spawn at the actor's root location."))
	FName DeathVFXSocket = NAME_None;

	/** Time before the death montage ends at which the VFX spawns AND the fade-out starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|VFX", meta=(ClampMin="0",
		ToolTip="VFX spawns this many seconds BEFORE the death montage ends. The fade-out starts at the same moment. e.g. 0.5 = halfway through the last second."))
	float DeathVFXTimeBeforeEnd = 0.5f;

	/** If true, the AI doesn't stay frozen on the ground — it dissolves smoothly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade",
		meta=(ToolTip="Smoothly fade the AI out instead of leaving the corpse on the ground."))
	bool bUseDeathFade = true;

	/** How long the fade lasts. Starts at the same moment as the VFX. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade", meta=(EditCondition="bUseDeathFade", ClampMin="0.05",
		ToolTip="Total duration of the dissolve/fade-out, starting from the VFX trigger moment."))
	float DeathFadeDuration = 1.0f;

	/** Scalar parameter on the mesh material driven from 0→1 during the fade.
	 *  Requires a dissolve-capable material (with a parameter of this name). If absent, the fallback (scale-down) is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade", meta=(EditCondition="bUseDeathFade",
		ToolTip="Material scalar parameter name driven 0→1 during the fade. Requires a dissolve material on the mesh. Common names: 'DissolveAmount', 'Dissolve', 'Opacity'."))
	FName DeathDissolveParameterName = TEXT("DissolveAmount");

	/** Optional sink-into-ground distance during the fade (cm). 0 = no sink. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade", meta=(EditCondition="bUseDeathFade", ClampMin="0",
		ToolTip="Distance the body sinks into the ground while fading. 0 = stays in place. ~50-100 cm gives a 'sinking corpse' effect."))
	float DeathSinkDistance = 0.f;

	/** If true, when no dissolve parameter is found, the actor is scaled down as a fallback so it still disappears smoothly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade", meta=(EditCondition="bUseDeathFade",
		ToolTip="If your material doesn't have the dissolve parameter, scale the actor down to 0 instead. Always works, but visually less elegant."))
	bool bUseScaleFallback = true;

	/* ── Optimization ── */

	/** Distance from the player at which this AI goes dormant (hidden, stops ticking). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization", meta=(ClampMin="500",
		ToolTip="AI further than this from the player will be put to sleep to save performance. AI within DormantDistance * 0.8 wakes up."))
	float DormantDistance = 8000.f;

	/** How often the dormancy distance is checked (seconds). Higher = less CPU but slower reaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization", meta=(ClampMin="0.1",
		ToolTip="Interval between dormancy checks. Lower = more responsive but slightly more expensive. Runs on a timer, NOT on Tick (so it works even while dormant)."))
	float DormancyCheckInterval = 1.f;

	/** If true, the AI starts dormant. Useful for AI placed far from the player's spawn point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization",
		meta=(ToolTip="If true, this AI starts dormant on BeginPlay. The dormancy timer will wake it up when the player gets close enough."))
	bool bStartDormant = false;

	/** Enable skeletal-mesh Update Rate Optimization (animate less often when small on screen / far). Big CPU win for crowds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization",
		meta=(ToolTip="Animate the mesh less frequently when it's far/small on screen. Recommended ON for all AI."))
	bool bEnableAnimURO = true;

	/* ── Detection Decal ── */

	/** Show a decal on the ground representing the detection radii. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection|Decal",
		meta=(ToolTip="Project detection radius visuals onto the ground and walls."))
	bool bShowDetectionDecal = false;

	/** Material for the proximity (360°) radius decal. Should be a circular decal material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection|Decal", meta=(EditCondition="bShowDetectionDecal",
		ToolTip="Decal material for the circular proximity detection area."))
	TObjectPtr<UMaterialInterface> ProximityDecalMaterial;

	/** Material for the sight cone decal. Should be a fan/cone shaped decal material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection|Decal", meta=(EditCondition="bShowDetectionDecal",
		ToolTip="Decal material for the directional sight cone area."))
	TObjectPtr<UMaterialInterface> SightDecalMaterial;

	/* ── Debug ── */
	
	/** Show debug lines for patrol path/zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug") bool bShowDebugPatrol = false;

	/** Show debug perception shapes (sight cone, hearing, proximity, leash, attack range). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug",
		meta=(ToolTip="Draw debug lines and shapes showing all perception radii."))
	bool bShowDebugPerception = false;

	/** Show debug info for pack behavior (alert radius sphere). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug",
		meta=(ToolTip="Draw debug sphere showing the pack alert radius."))
	bool bShowDebugPack = false;

	/** Show debug info for dormancy (sphere of DormantDistance, color-coded by state). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug",
		meta=(ToolTip="Draw a sphere showing the dormancy radius. Green = active, red = dormant."))
	bool bShowDebugDormancy = false;

	/* ── Components ── */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UAIAnimationComponent> AIAnimationComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UAICombatComponent> AICombatComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<USplineComponent> PatrolSpline;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UDecalComponent> ProximityDecal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UDecalComponent> SightDecal;

private:
	/* ── Dormancy ── */
	void UpdateDormancy();

	/* ── Pack ── */
	void UpdatePackFollow(float DeltaTime);

	bool ReactToThreat(AActor* Threat, bool bFromDamage);
	void AlarmNearbyAllies(AActor* Threat);

	/** Radius-based combat rally: when this AI engages, pull nearby aggressive allies onto the same target. */
	void RallyNearbyAllies(AActor* Threat);

	/* ── Threat ── */
	void EvaluateThreatSwitch();
	void TickThreatDecay(float DeltaTime);

	/* ── Respawn ── */
	void HandleRespawnTimer();
	void DestroyCorpse();

	/* ── Death sequence ── */
	void OnDeathVFXAndFadeStart();
	void TickDeathFade(float DeltaTime);
	void FinishDeathFade();
	void ResetDeathVisuals();
	void CacheMeshMaterials();

	/* ── Damage routing ── */
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	/** Bound to HealthComponent::OnHealthChanged. Triggers Die() when HP hits 0. */
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);

#if ENABLE_DRAW_DEBUG
	void DrawDebugDormancy() const;
#endif

	UPROPERTY() EAIState CurrentState = EAIState::Idle;
	UPROPERTY() EAIAwarenessLevel AwarenessLevel = EAIAwarenessLevel::Unaware;
	UPROPERTY() TObjectPtr<AActor> CurrentTarget;
	UPROPERTY() TObjectPtr<AActor> InteractionPartner;
	UPROPERTY() EAIState PreInteractionState = EAIState::Idle;

	mutable TWeakObjectPtr<ABaseAICharacter> CachedPackLeader;
	mutable float PackLeaderCacheStamp = -1000.f;
	static constexpr float PACK_LEADER_CACHE_TTL = 0.5f;
	FVector CachedFollowOffset = FVector::ZeroVector; // cached herd-follow spread (re-rolled on a timer, not per frame)
	float FollowReevalTimer = 0.f;
	bool bHasFollowOffset = false;
	UPROPERTY() TObjectPtr<UNiagaraComponent> SpawnedDeathVFX;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> CachedDynamicMaterials;
	UPROPERTY() TObjectPtr<UHealthComponent> CachedHealthComponent;

	FVector SpawnLocation;
	FRotator SpawnRotation;
	FVector SpawnScale = FVector::OneVector;
	float LastHealthFraction = 1.f;
	float MoraleBreakUntil = -1000.f; // world time until which a morale-broken AI refuses to re-engage
	FVector LastHitDirection = FVector::ZeroVector; // travel direction of the last hit (for directional reactions)
	bool bSuppressAlarmBroadcast = false; // set while reacting to a herd alarm so we don't re-broadcast (anti-cascade)
	bool bSuppressRallyBroadcast = false; // set while answering a combat rally so we don't re-rally (single-hop spread)

	/** Per-attacker threat (weak keys; not GC-tracked). Drives target focus when bUseThreatSystem. */
	TMap<TWeakObjectPtr<AActor>, float> ThreatTable;
	FVector DeathStartLocation;
	bool bIsDormant = false;
	bool bIsDeathFading = false;
	bool bDissolveParamFound = false;
	float DeathFadeProgress = 0.f;

	FTimerHandle DormancyCheckTimerHandle;
	FTimerHandle RespawnTimerHandle;
	FTimerHandle DeathVFXTimerHandle;
};