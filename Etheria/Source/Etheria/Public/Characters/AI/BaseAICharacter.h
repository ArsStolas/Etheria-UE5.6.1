/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAICharacter - Header"
 * Notes: Pack, respawn, dormancy (timer-based), detection decal, hit reactions,
 *        death VFX + dissolve fade, flee/fight-back.
 */

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Characters/AI/AI_Types.h"
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

UCLASS()
class ETHERIA_API ABaseAICharacter : public ABaseCharacter
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
	UFUNCTION(BlueprintPure, Category="AI") float GetLeashRange() const { return LeashRange; }
	UFUNCTION(BlueprintPure, Category="AI") bool CanFlee() const { return bCanFlee; }
	UFUNCTION(BlueprintPure, Category="AI") bool ShouldShowDebugPerception() const { return bShowDebugPerception; }
	UFUNCTION(BlueprintPure, Category="AI") FVector GetSpawnLocation() const { return SpawnLocation; }
	UFUNCTION(BlueprintPure, Category="AI") EAIAwarenessLevel GetAwarenessLevel() const { return AwarenessLevel; }
	UFUNCTION(BlueprintPure, Category="AI") FName GetPackID() const { return PackID; }
	UFUNCTION(BlueprintPure, Category="AI") bool IsDead() const { return CurrentState == EAIState::Dead; }
	UFUNCTION(BlueprintPure, Category="AI") bool IsDormant() const { return bIsDormant; }
	UFUNCTION(BlueprintPure, Category="AI") bool OnlyDetectsPlayers() const { return bOnlyDetectPlayers; }
	UFUNCTION(BlueprintPure, Category="AI") bool ShouldShowDebugPatrol() const { return bShowDebugPatrol; }
	UFUNCTION(BlueprintPure, Category="AI|Damage") bool GetCanReceiveDamage() const { return bCanReceiveDamage; }

	/* ═══════════ Setters ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI") void SetAIState(EAIState NewState);
	UFUNCTION(BlueprintCallable, Category="AI") void SetTarget(AActor* NewTarget);
	UFUNCTION(BlueprintCallable, Category="AI") void ClearTarget();
	UFUNCTION(BlueprintCallable, Category="AI") void SetAwarenessLevel(EAIAwarenessLevel NewLevel);
	UFUNCTION(BlueprintCallable, Category="AI") void SetHostilityType(EAIHostilityType NewType) { HostilityType = NewType; }

	/** Toggle damage immunity at runtime. Useful for cutscenes, scripted moments, or boss invulnerability phases. */
	UFUNCTION(BlueprintCallable, Category="AI|Damage") void SetCanReceiveDamage(bool bNewCanReceiveDamage) { bCanReceiveDamage = bNewCanReceiveDamage; }

	/* ═══════════ Perception / Damage ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI") void OnPerceiveTarget(AActor* PerceivedActor);
	UFUNCTION(BlueprintCallable, Category="AI") void OnReceiveDamage(AActor* DamageInstigator, float DamageAmount);

	/* ═══════════ Pack ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI|Pack") void AlertPack(AActor* Threat);
	UFUNCTION(BlueprintCallable, Category="AI|Pack") void OnPackAlert(ABaseAICharacter* Alerter, AActor* Threat);
	UFUNCTION(BlueprintCallable, Category="AI|Pack") TArray<ABaseAICharacter*> GetPackMembers() const;
	UFUNCTION(BlueprintPure, Category="AI|Pack") ABaseAICharacter* GetPackLeader() const;
	UFUNCTION(BlueprintPure, Category="AI|Pack") bool IsPackLeader() const;

	/* ═══════════ Leash / Respawn ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI") void TeleportToSpawn();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void Die();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void Respawn();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void NotifyPlayerSaved();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void NotifyDayCycleComplete();

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

	/** Maximum distance from spawn point. AI teleports back if it exceeds this while chasing or fleeing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0",
		ToolTip="How far this AI can go from its spawn before it teleports back."))
	float LeashRange = 2000.f;

	/** If true, the player can interact with this AI (talk, quest, trade). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior",
		meta=(ToolTip="Enable interaction. When true, the player can talk to this AI."))
	bool bIsInteractable = false;

	/* ── Damage ── */

	/** If true, this AI can take damage normally (HP loss, hit reaction, death). If false, all incoming
	 *  damage is rejected — no HP change, no animation, no death — and OnAIDamageBlocked fires instead.
	 *  Use this for invulnerable NPCs (merchants, story characters) or scripted invulnerability phases. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Damage",
		meta=(ToolTip="When OFF, the player (and anything else using ApplyDamage) cannot harm this AI. The AI plays no hit reaction, loses no HP, and OnAIDamageBlocked fires for feedback hooks."))
	bool bCanReceiveDamage = false;
	
	/* ── Component Movements ── */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UAIMovementComponent> AIMovementComponent;
	
	/* ── Detection ── */

	/** Tags that trigger flee. If empty, the AI flees from any detected actor (that passes the player filter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="Specific actor tags that make this AI flee. Leave empty to flee from any valid target."))
	TArray<FName> FleeFromTags;

	/** If true, this AI only detects player-controlled pawns. Ignores other AI and NPCs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="Only react to player-controlled characters. Other AI are ignored."))
	bool bOnlyDetectPlayers = true;
	
	/* ── Pack ── */

	/** Pack identifier. AI with the same PackID form a group and share alerts. Leave empty for solo AI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack",
		meta=(ToolTip="Set the same PackID on multiple AI to make them a pack. They will alert and follow each other."))
	FName PackID = NAME_None;

	/** Radius within which pack alerts are transmitted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(ClampMin="0",
		ToolTip="How far a pack alert travels. Only AI within this distance will be notified."))
	float PackAlertRadius = 3000.f;

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

	/* ── Respawn ── */
	void HandleRespawnTimer();

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
	UPROPERTY() TObjectPtr<UNiagaraComponent> SpawnedDeathVFX;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> CachedDynamicMaterials;
	UPROPERTY() TObjectPtr<UHealthComponent> CachedHealthComponent;

	FVector SpawnLocation;
	FRotator SpawnRotation;
	FVector SpawnScale = FVector::OneVector;
	FVector DeathStartLocation;
	bool bIsDormant = false;
	bool bIsDeathFading = false;
	bool bDissolveParamFound = false;
	float DeathFadeProgress = 0.f;

	FTimerHandle DormancyCheckTimerHandle;
	FTimerHandle RespawnTimerHandle;
	FTimerHandle DeathVFXTimerHandle;
};