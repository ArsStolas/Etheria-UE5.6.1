/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAICharacter - Header"
 * Notes: No dissolve. Hide/teleport when too far. Pack system. Respawn. LOD optimization.
 *        Detection decal. Player-only / tag-based detection.
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

/* ── Dispatchers ── */

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
	UFUNCTION(BlueprintPure, Category="AI") const TArray<FName>& GetFleeFromTags() const { return FleeFromTags; }
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

	/** Get all alive pack members (excluding self). */
	UFUNCTION(BlueprintCallable, Category="AI|Pack") TArray<ABaseAICharacter*> GetPackMembers() const;

	/** Get the pack leader (first alive member, deterministic). */
	UFUNCTION(BlueprintPure, Category="AI|Pack") ABaseAICharacter* GetPackLeader() const;

	/** Am I the pack leader? */
	UFUNCTION(BlueprintPure, Category="AI|Pack") bool IsPackLeader() const;

	/* ═══════════ Leash / Teleport ═══════════ */

	/** Called when AI is too far from spawn. Hides, teleports, shows. */
	UFUNCTION(BlueprintCallable, Category="AI") void TeleportToSpawn();

	/* ═══════════ Respawn ═══════════ */

	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void Die();
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void Respawn();

	/** Call from your save system when the player saves. */
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void NotifyPlayerSaved();
	/** Call from your day/night system when a full day passes. */
	UFUNCTION(BlueprintCallable, Category="AI|Respawn") void NotifyDayCycleComplete();

	/* ═══════════ Optimization ═══════════ */

	/** Called by the controller to put AI to sleep/wake based on player distance. */
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Identity")
	EAIHostilityType HostilityType = EAIHostilityType::Passive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Identity", meta=(EditCondition="HostilityType==EAIHostilityType::Aggressive"))
	EAIRank Rank = EAIRank::Basic;

	/* ── Behavior ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior")
	bool bCanFlee = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior", meta=(ClampMin="0"))
	float LeashRange = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Behavior")
	bool bIsInteractable = false;

	/* ── Detection filter ── */

	/** Tags that trigger flee (for Passive/Neutral with bCanFlee). If empty, flees from players. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection")
	TArray<FName> FleeFromTags;

	/** If true, only detect actors with the "Player" tag (or custom tag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection")
	bool bOnlyDetectPlayers = true;

	/* ── Pack ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack")
	FName PackID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(ClampMin="0"))
	float PackAlertRadius = 3000.f;

	/** Distance pack members try to stay from their leader. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(ClampMin="50"))
	float PackFollowDistance = 400.f;

	/** How close pack members clump around the leader. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Pack", meta=(ClampMin="50"))
	float PackSpreadRadius = 300.f;

	/* ── Respawn ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Respawn")
	EAIRespawnCondition RespawnCondition = EAIRespawnCondition::OnSaveOrDay;

	/** Timer duration if RespawnCondition is OnTimer (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Respawn", meta=(EditCondition="RespawnCondition==EAIRespawnCondition::OnTimer", ClampMin="1"))
	float RespawnTimerDuration = 300.f;

	/* ── Optimization ── */

	/** Distance from the player at which AI goes dormant (stops ticking, hidden). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization", meta=(ClampMin="1000"))
	float DormantDistance = 8000.f;

	/** How often (seconds) to check dormancy distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Optimization", meta=(ClampMin="0.5"))
	float DormancyCheckInterval = 2.f;

	/* ── Detection Decal ── */

	/** Enable showing detection radius as a decal projected on the ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection|Decal")
	bool bShowDetectionDecal = false;

	/** Material for the proximity radius decal. Should be a circular decal material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection|Decal", meta=(EditCondition="bShowDetectionDecal"))
	TObjectPtr<UMaterialInterface> ProximityDecalMaterial;

	/** Material for the sight cone decal. Should be a cone/fan shape decal material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Detection|Decal", meta=(EditCondition="bShowDetectionDecal"))
	TObjectPtr<UMaterialInterface> SightDecalMaterial;

	/* ── Debug ── */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug")
	bool bShowDebugPerception = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Debug")
	bool bShowDebugPack = false;

	/* ── Components ── */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UAIMovementComponent> AIMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UAIAnimationComponent> AIAnimationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UAICombatComponent> AICombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USplineComponent> PatrolSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDecalComponent> ProximityDecal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDecalComponent> SightDecal;

private:
	void UpdateDormancy();
	void UpdatePackFollow(float DeltaTime);
	void HandleRespawnTimer();

	UPROPERTY() EAIState CurrentState = EAIState::Idle;
	UPROPERTY() EAIAwarenessLevel AwarenessLevel = EAIAwarenessLevel::Unaware;
	UPROPERTY() TObjectPtr<AActor> CurrentTarget;

	FVector SpawnLocation;
	FRotator SpawnRotation;
	bool bIsDormant = false;
	float DormancyTimer = 0.f;
	FTimerHandle RespawnTimerHandle;
};
