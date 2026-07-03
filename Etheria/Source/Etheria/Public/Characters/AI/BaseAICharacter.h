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
class UCombatComponent;

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

	UFUNCTION(BlueprintPure, Category="AI") UAIMovementComponent* GetAIMovement() const { return AIMovementComponent; }
	UFUNCTION(BlueprintPure, Category="AI") UAIAnimationComponent* GetAIAnimation() const { return AIAnimationComponent; }
	UFUNCTION(BlueprintPure, Category="AI") UAICombatComponent* GetAICombat() const { return AICombatComponent; }
	UFUNCTION(BlueprintPure, Category="AI") USplineComponent* GetPatrolSpline() const { return PatrolSpline; }

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
	UFUNCTION(BlueprintPure, Category="AI") bool WantsInfiniteSightPursuit() const { return bInfiniteSightPursuit; }

	UFUNCTION(BlueprintPure, Category="AI") bool CanReactToPerception() const;

	UFUNCTION(BlueprintPure, Category="AI") bool IsValidTargetCandidate(AActor* Candidate) const;

	UFUNCTION(BlueprintPure, Category="AI") bool MatchesFleeTag(const AActor* Other) const;

	UFUNCTION(BlueprintPure, Category="AI") bool IsTargetDeadOrInvalid(const AActor* Target) const;

	UFUNCTION(BlueprintPure, Category="AI") bool ShouldShowDebugPatrol() const { return bShowDebugPatrol; }
	UFUNCTION(BlueprintPure, Category="AI|Damage") bool GetCanReceiveDamage() const { return bCanReceiveDamage; }

	UFUNCTION(BlueprintPure, Category="AI|Damage") bool IsKillable() const;

	UFUNCTION(BlueprintCallable, Category="AI") void SetAIState(EAIState NewState);
	UFUNCTION(BlueprintCallable, Category="AI") void SetTarget(AActor* NewTarget);
	UFUNCTION(BlueprintCallable, Category="AI") void ClearTarget();
	UFUNCTION(BlueprintCallable, Category="AI") void SetAwarenessLevel(EAIAwarenessLevel NewLevel);
	UFUNCTION(BlueprintCallable, Category="AI") void SetHostilityType(EAIHostilityType NewType);

	UFUNCTION(BlueprintCallable, Category="AI|Threat") void AddThreat(AActor* Source, float Amount);

	UFUNCTION(BlueprintNativeEvent, Category="AI|Combat") bool IsTargetDefending(AActor* Target) const;
	virtual bool IsTargetDefending_Implementation(AActor* Target) const;

	UFUNCTION(BlueprintNativeEvent, Category="AI|Combat") bool IsTargetWindingUpAttack(AActor* Target) const;
	virtual bool IsTargetWindingUpAttack_Implementation(AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category="AI|Damage") void SetCanReceiveDamage(bool bNewCanReceiveDamage);

	UFUNCTION(BlueprintCallable, Category="AI") void OnPerceiveTarget(AActor* PerceivedActor);
	UFUNCTION(BlueprintCallable, Category="AI") void OnReceiveDamage(AActor* DamageInstigator, float DamageAmount);

	virtual void Interact_Implementation(AActor* Target) override;
	virtual void CanReceiveTrace_Implementation() override;

	UFUNCTION(BlueprintCallable, Category="AI|Interaction") void BeginInteraction(AActor* Interactor);

	UFUNCTION(BlueprintCallable, Category="AI|Interaction") void EndInteraction();
	UFUNCTION(BlueprintPure, Category="AI|Interaction") AActor* GetInteractionPartner() const { return InteractionPartner; }
	UFUNCTION(BlueprintPure, Category="AI|Interaction") bool IsInteractable() const { return bIsInteractable; }

	UPROPERTY(BlueprintAssignable, Category="AI|Interaction") FOnAIInteractionStarted OnInteractionStarted;
	UPROPERTY(BlueprintAssignable, Category="AI|Interaction") FOnAIInteractionEnded OnInteractionEnded;

	UFUNCTION(BlueprintCallable, Category="AI|Pack") void AlertPack(AActor* Threat);
	UFUNCTION(BlueprintCallable, Category="AI|Pack") void OnPackAlert(ABaseAICharacter* Alerter, AActor* Threat);
	UFUNCTION(BlueprintCallable, Category="AI|Pack") TArray<ABaseAICharacter*> GetPackMembers() const;

	UFUNCTION(BlueprintCallable, Category="AI|Pack") void AlertPackSearch(const FVector& Location);
	UFUNCTION(BlueprintPure, Category="AI|Pack") ABaseAICharacter* GetPackLeader() const;
	UFUNCTION(BlueprintPure, Category="AI|Pack") bool IsPackLeader() const;

	UFUNCTION(BlueprintCallable, Category="AI") void TeleportToSpawn(FVector OverrideLocation = FVector::ZeroVector);
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void Die();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void Respawn();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void NotifyPlayerSaved();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void NotifyDayCycleComplete();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AI|Schedule") void OnTimeOfDayChanged(float NormalizedTime);
	virtual void OnTimeOfDayChanged_Implementation(float NormalizedTime) {}

	UFUNCTION(BlueprintCallable, Category="AI|Optimization") void SetDormant(bool bDormant);

	UFUNCTION(BlueprintCallable, Category="AI|Optimization") void ForceDormancyCheck() { UpdateDormancy(); }

	UFUNCTION(BlueprintCallable, Category="AI|Debug") void SetDetectionDecalVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category="AI|Debug") void SetDetectionDecalRadius(float Radius);

	UPROPERTY(BlueprintAssignable, Category="AI") FOnAIStateChanged OnAIStateChanged;
	UPROPERTY(BlueprintAssignable, Category="AI") FOnTargetAcquired OnTargetAcquired;
	UPROPERTY(BlueprintAssignable, Category="AI") FOnTargetLost OnTargetLost;
	UPROPERTY(BlueprintAssignable, Category="AI") FOnAIDamaged OnAIDamaged;
	UPROPERTY(BlueprintAssignable, Category="AI") FOnAwarenessChanged OnAwarenessChanged;
	UPROPERTY(BlueprintAssignable, Category="AI|Pack") FOnPackAlerted OnPackAlerted;
	UPROPERTY(BlueprintAssignable, Category="AI|Respawn") FOnAIDied OnAIDied;
	UPROPERTY(BlueprintAssignable, Category="AI|Respawn") FOnAIRespawned OnAIRespawned;
	UPROPERTY(BlueprintAssignable, Category="AI|Optimization") FOnAIDormancyChanged OnAIDormancyChanged;

	UPROPERTY(BlueprintAssignable, Category="AI|Death") FOnAIDeathFadeStarted OnAIDeathFadeStarted;

	UPROPERTY(BlueprintAssignable, Category="AI|Death") FOnAIDeathFadeCompleted OnAIDeathFadeCompleted;

	UPROPERTY(BlueprintAssignable, Category="AI|Damage") FOnAIDamageBlocked OnAIDamageBlocked;

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Identity")
	EAIHostilityType HostilityType = EAIHostilityType::Passive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Identity", meta=(EditCondition="HostilityType==EAIHostilityType::Aggressive"))
	EAIRank Rank = EAIRank::Basic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior",
		meta=(ToolTip="Enable flee behavior. When threatened, the AI will run away instead of fighting."))
	bool bCanFlee = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior",
		meta=(ToolTip="When a Neutral AI is fleeing and gets hit, it turns around and fights the attacker.", EditCondition="bCanFlee"))
	bool bFightBackWhenAttacked = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0", ClampMax="1", EditCondition="bCanFlee",
		ToolTip="HP fraction (0-1) under which the AI panics and flees mid-fight. 0 = off. e.g. 0.25 = flee under 25% HP."))
	float FleeHealthThreshold = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0", EditCondition="bCanFlee",
		ToolTip="Seconds after a morale-break flee during which the AI won't turn back to fight, even if hit."))
	float MoraleBreakCooldown = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Threat",
		meta=(ToolTip="Focus the highest-threat attacker instead of the nearest. Supports taunts (AddThreat) and co-op."))
	bool bUseThreatSystem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Threat", meta=(EditCondition="bUseThreatSystem", ClampMin="1.0",
		ToolTip="Threat factor needed to steal aggro. 1.5 = a challenger needs 50% more threat than the current target."))
	float ThreatSwitchMargin = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Threat", meta=(EditCondition="bUseThreatSystem", ClampMin="0"))
	float ThreatPerDamage = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Threat", meta=(EditCondition="bUseThreatSystem", ClampMin="0",
		ToolTip="Threat lost per second so old attackers eventually stop holding aggro. 0 = permanent."))
	float ThreatDecayPerSecond = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="While it can SEE its engaged target, this AI follows it ANYWHERE on the map: no distance cap on tracking and no leash give-up. Losing sight falls back to the normal memory/leash rules."))
	bool bInfiniteSightPursuit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior",
		meta=(ToolTip="Idle NPCs turn to look at the player when noticed. Pure ambient flavor."))
	bool bNoticeReactions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0",
		ToolTip="How far this AI can go from its spawn before it teleports back."))
	float LeashRange = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0.1",
		ToolTip="Duration (seconds) of the stagger triggered after taking StaggerThreshold hits."))
	float StaggerDuration = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior",
		meta=(ToolTip="Enable interaction. When true, the player can talk to this AI."))
	bool bIsInteractable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Interaction", meta=(EditCondition="bIsInteractable"))
	EAINPCRole NPCRole = EAINPCRole::Dialogue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Interaction", meta=(EditCondition="bIsInteractable"))
	FName DialogueID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Damage",
		meta=(ToolTip="When OFF, the player (and anything else using ApplyDamage) cannot harm this AI. The AI plays no hit reaction, loses no HP, and OnAIDamageBlocked fires for feedback hooks."))
	bool bCanReceiveDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Damage",
		meta=(ToolTip="Auto: only fighters can be hit/killed by the player — NPCs/animals are untouchable (attacks pass through). Override with Killable/Unkillable."))
	EAIKillability Killability = EAIKillability::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Damage", meta=(ClampMin="0", ClampMax="1",
		ToolTip="One hit >= this fraction of MaxHealth staggers immediately, regardless of the hit counter. 0 = off."))
	float HeavyHitStaggerFraction = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Damage", meta=(ClampMin="0", ClampMax="1",
		ToolTip="Hits under this fraction of MaxHealth skip the full-body hit reaction (still count toward stagger)."))
	float LightHitFlinchFraction = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Damage", meta=(ClampMin="0",
		ToolTip="Stagger hit-count resets after this long without damage. 0 = never decays."))
	float HitCountDecayTime = 4.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UAIMovementComponent> AIMovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="Actor tags that make this AI flee (e.g. 'Predator'). Tagged actors are seen even when 'Only Detect Players' is on."))
	TArray<FName> FleeFromTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="On flee, alert nearby NPCs of the same hostility within AlarmRadius so a herd panics together."))
	bool bAlarmsNearbyAllies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection", meta=(EditCondition="bAlarmsNearbyAllies", ClampMin="0",
		ToolTip="How far a panic spreads to same-type NPCs when this one flees."))
	float AlarmRadius = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="Only react to player-controlled characters. Other AI are ignored."))
	bool bOnlyDetectPlayers = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI|Pack",
		meta=(ToolTip="Set the same PackID on multiple AI to make them a pack. They will alert and follow each other. Set in the editor, not at runtime."))
	FName PackID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(ClampMin="0",
		ToolTip="How far a pack alert travels. Only AI within this distance will be notified."))
	float PackAlertRadius = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack",
		meta=(EditCondition="HostilityType==EAIHostilityType::Aggressive",
		ToolTip="When this AI engages, rally nearby aggressive allies onto the same target (independent of PackID). The reason packmates join in when you hit one of them."))
	bool bCallForHelpOnEngage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(EditCondition="bCallForHelpOnEngage", ClampMin="0",
		ToolTip="How far the 'call for help' reaches. Nearby aggressive allies within this distance join the fight."))
	float CombatAlertRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(EditCondition="bCallForHelpOnEngage",
		ToolTip="ON: allies aggro the target instantly (snappy pack). OFF: allies investigate the spot first and only fight once they see it (cautious, no aggro through walls)."))
	bool bRallyEngagesDirectly = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(ClampMin="50",
		ToolTip="Ideal distance from the pack leader during patrol."))
	float PackFollowDistance = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(ClampMin="50",
		ToolTip="Random offset applied so pack members don't overlap."))
	float PackSpreadRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Respawn",
		meta=(ToolTip="Choose the condition under which this AI will respawn after being killed."))
	EAIRespawnCondition RespawnCondition = EAIRespawnCondition::OnSaveOrDay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Respawn",
		meta=(EditCondition="RespawnCondition==EAIRespawnCondition::OnTimer", ClampMin="1",
		ToolTip="Time in seconds before this AI respawns (only for Timer mode)."))
	float RespawnTimerDuration = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Respawn",
		meta=(ToolTip="If this AI never respawns, destroy it after the fade so it doesn't linger as a hidden actor."))
	bool bDestroyCorpseIfNoRespawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|VFX",
		meta=(ToolTip="Niagara system spawned during the death sequence. Use to cover the fade-out with particles. Leave empty for no VFX."))
	TObjectPtr<UNiagaraSystem> DeathVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|VFX",
		meta=(ToolTip="Mesh socket or bone the VFX attaches to. Leave None to spawn at the actor's root location."))
	FName DeathVFXSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|VFX", meta=(ClampMin="0",
		ToolTip="VFX spawns this many seconds BEFORE the death montage ends. The fade-out starts at the same moment. e.g. 0.5 = halfway through the last second."))
	float DeathVFXTimeBeforeEnd = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade",
		meta=(ToolTip="Smoothly fade the AI out instead of leaving the corpse on the ground."))
	bool bUseDeathFade = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade", meta=(EditCondition="bUseDeathFade", ClampMin="0.05",
		ToolTip="Total duration of the dissolve/fade-out, starting from the VFX trigger moment."))
	float DeathFadeDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade", meta=(EditCondition="bUseDeathFade",
		ToolTip="Material scalar parameter name driven 0→1 during the fade. Requires a dissolve material on the mesh. Common names: 'DissolveAmount', 'Dissolve', 'Opacity'."))
	FName DeathDissolveParameterName = TEXT("DissolveAmount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade", meta=(EditCondition="bUseDeathFade", ClampMin="0",
		ToolTip="Distance the body sinks into the ground while fading. 0 = stays in place. ~50-100 cm gives a 'sinking corpse' effect."))
	float DeathSinkDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade", meta=(EditCondition="bUseDeathFade",
		ToolTip="If your material doesn't have the dissolve parameter, scale the actor down to 0 instead. Always works, but visually less elegant."))
	bool bUseScaleFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade",
		meta=(ToolTip="Missing death montage → physics ragdoll (pushed along the killing hit) instead of a frozen stand."))
	bool bRagdollFallbackDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Death|Fade", meta=(EditCondition="bRagdollFallbackDeath", ClampMin="0"))
	float DeathRagdollImpulse = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization", meta=(ClampMin="500",
		ToolTip="AI further than this from the player will be put to sleep to save performance. AI within DormantDistance * 0.8 wakes up."))
	float DormantDistance = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization", meta=(ClampMin="0.1",
		ToolTip="Interval between dormancy checks. Lower = more responsive but slightly more expensive. Runs on a timer, NOT on Tick (so it works even while dormant)."))
	float DormancyCheckInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization",
		meta=(ToolTip="If true, this AI starts dormant on BeginPlay. The dormancy timer will wake it up when the player gets close enough."))
	bool bStartDormant = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization",
		meta=(ToolTip="Animate the mesh less frequently when it's far/small on screen. Recommended ON for all AI."))
	bool bEnableAnimURO = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection|Decal",
		meta=(ToolTip="Project detection radius visuals onto the ground and walls."))
	bool bShowDetectionDecal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection|Decal", meta=(EditCondition="bShowDetectionDecal",
		ToolTip="Decal material for the circular proximity detection area."))
	TObjectPtr<UMaterialInterface> ProximityDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection|Decal", meta=(EditCondition="bShowDetectionDecal",
		ToolTip="Decal material for the directional sight cone area."))
	TObjectPtr<UMaterialInterface> SightDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug") bool bShowDebugPatrol = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug",
		meta=(ToolTip="Draw debug lines and shapes showing all perception radii."))
	bool bShowDebugPerception = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug",
		meta=(ToolTip="Draw debug sphere showing the pack alert radius."))
	bool bShowDebugPack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug",
		meta=(ToolTip="Draw a sphere showing the dormancy radius. Green = active, red = dormant."))
	bool bShowDebugDormancy = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UAIAnimationComponent> AIAnimationComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UAICombatComponent> AICombatComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<USplineComponent> PatrolSpline;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UDecalComponent> ProximityDecal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UDecalComponent> SightDecal;

private:

	void UpdateDormancy();

	void UpdatePackFollow(float DeltaTime);

	bool ReactToThreat(AActor* Threat, bool bFromDamage);
	void JoinHuntDelayed(AActor* Threat);
	void AlarmNearbyAllies(AActor* Threat);

	void RallyNearbyAllies(AActor* Threat);

	void EvaluateThreatSwitch();
	void TickThreatDecay(float DeltaTime);

	void HandleRespawnTimer();
	void DestroyCorpse();

	void OnDeathVFXAndFadeStart();
	void TickDeathFade(float DeltaTime);
	void FinishDeathFade();
	void ResetDeathVisuals();
	void CacheMeshMaterials();

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

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

	mutable TWeakObjectPtr<UCombatComponent> CachedTargetCombat;
	mutable TWeakObjectPtr<const AActor> CachedTargetCombatOwner;
	float LastDamageTime = -1000.f;

	mutable TWeakObjectPtr<ABaseAICharacter> CachedPackLeader;
	mutable float PackLeaderCacheStamp = -1000.f;
	static constexpr float PACK_LEADER_CACHE_TTL = 0.5f;
	FVector CachedFollowOffset = FVector::ZeroVector;
	float FollowReevalTimer = 0.f;
	bool bHasFollowOffset = false;
	UPROPERTY() TObjectPtr<UNiagaraComponent> SpawnedDeathVFX;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> CachedDynamicMaterials;
	UPROPERTY() TObjectPtr<UHealthComponent> CachedHealthComponent;

	FVector SpawnLocation;
	FRotator SpawnRotation;
	FVector SpawnScale = FVector::OneVector;
	FTransform MeshRelativeTransform;
	float LastHealthFraction = 1.f;
	float MoraleBreakUntil = -1000.f;
	FVector LastHitDirection = FVector::ZeroVector;
	bool bSuppressAlarmBroadcast = false;
	bool bSuppressRallyBroadcast = false;

	TMap<TWeakObjectPtr<AActor>, float> ThreatTable;
	FVector DeathStartLocation;
	bool bIsDormant = false;
	bool bIsDeathFading = false;
	bool bDissolveParamFound = false;
	float DeathFadeProgress = 0.f;

	FTimerHandle DormancyCheckTimerHandle;
	FTimerHandle RespawnTimerHandle;
	FTimerHandle DeathVFXTimerHandle;
	FTimerHandle PackAlertTimerHandle;
	TWeakObjectPtr<AActor> PendingPackThreat;
};
