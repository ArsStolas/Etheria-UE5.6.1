/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAICharacter - Header"
 * Notes: Pack, respawn, dormancy, detection decal, hit reactions, flee/fight-back.
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIStateChanged, EAIState, OldState, EAIState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetAcquired, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetLost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIDamaged, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAwarenessChanged, EAIAwarenessLevel, Old, EAIAwarenessLevel, New);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPackAlerted, ABaseAICharacter*, Alerter, AActor*, Threat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIRespawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIDormancyChanged);

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

	/* ═══════════ Setters ═══════════ */
	UFUNCTION(BlueprintCallable, Category="AI") void SetAIState(EAIState NewState);
	UFUNCTION(BlueprintCallable, Category="AI") void SetTarget(AActor* NewTarget);
	UFUNCTION(BlueprintCallable, Category="AI") void ClearTarget();
	UFUNCTION(BlueprintCallable, Category="AI") void SetAwarenessLevel(EAIAwarenessLevel NewLevel);
	UFUNCTION(BlueprintCallable, Category="AI") void SetHostilityType(EAIHostilityType NewType) { HostilityType = NewType; }

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

protected:
	virtual void BeginPlay() override;
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

	/* ── Detection ── */

	/** Tags that trigger flee. If empty, the AI flees from any detected actor (that passes the player filter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="Specific actor tags that make this AI flee. Leave empty to flee from any valid target."))
	TArray<FName> FleeFromTags;

	/** If true, this AI only detects player-controlled pawns. Ignores other AI and NPCs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection",
		meta=(ToolTip="Only react to player-controlled characters. Other AI are ignored."))
	bool bOnlyDetectPlayers = true;

	/* ── Rotation ── */

	/** How fast the AI rotates toward its movement direction (degrees/sec). Lower = smoother turns for humanoids. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Movement",
		meta=(ClampMin="50", ClampMax="1000", ToolTip="Rotation speed when moving. Lower values give smoother turns."))
	float MovementRotationRate = 400.f;

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

	/* ── Optimization ── */

	/** Distance from the player at which this AI goes dormant (hidden, stops ticking). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization", meta=(ClampMin="1000",
		ToolTip="AI further than this from the player will be put to sleep to save performance."))
	float DormantDistance = 8000.f;

	/** How often the dormancy distance is checked (seconds). Higher = less CPU but slower reaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization", meta=(ClampMin="0.5",
		ToolTip="Interval between dormancy checks. Lower = more responsive but slightly more expensive."))
	float DormancyCheckInterval = 2.f;

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

	/** Show debug perception shapes (sight cone, hearing, proximity, leash, attack range). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug",
		meta=(ToolTip="Draw debug lines and shapes showing all perception radii."))
	bool bShowDebugPerception = false;

	/** Show debug info for pack behavior (alert radius sphere). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug",
		meta=(ToolTip="Draw debug sphere showing the pack alert radius."))
	bool bShowDebugPack = false;

	/* ── Components ── */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UAIMovementComponent> AIMovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UAIAnimationComponent> AIAnimationComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UAICombatComponent> AICombatComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<USplineComponent> PatrolSpline;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UDecalComponent> ProximityDecal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UDecalComponent> SightDecal;

private:
	void UpdateDormancy();
	void UpdatePackFollow(float DeltaTime);
	void HandleRespawnTimer();

	/** Called by HealthComponent via OnTakeAnyDamage — routes to OnReceiveDamage. */
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY() EAIState CurrentState = EAIState::Idle;
	UPROPERTY() EAIAwarenessLevel AwarenessLevel = EAIAwarenessLevel::Unaware;
	UPROPERTY() TObjectPtr<AActor> CurrentTarget;

	FVector SpawnLocation;
	FRotator SpawnRotation;
	bool bIsDormant = false;
	float DormancyTimer = 0.f;
	FTimerHandle RespawnTimerHandle;
};
