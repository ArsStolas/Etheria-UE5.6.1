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
class UStaticMeshComponent;
class UDecalComponent;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGolemActivated);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemAttackBegin, FName, AttackId, EGolemHazardShape, Shape);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemTelegraph, const FGolemTelegraph&, Telegraph);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemStrike, const FGolemStrikeEvent&, Strike);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemProjectileLaunch, const FGolemProjectileLaunch&, Launch);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemHazardTick, const FGolemHazardState&, State);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnGolemDamageDealt, AActor*, Victim, float, Damage, FName, AttackId, FVector, HitLocation);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemAttackRecovery, FName, AttackId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemAttackEnd, FName, AttackId, bool, bInterrupted);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnGolemAirZoneOpened, int32, ZoneId, FVector, Location, float, Radius, float, Lifetime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemAirZoneClosed, int32, ZoneId, FVector, Location);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGolemPhaseChanged, int32, OldPhase, int32, NewPhase, FName, PhaseName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGolemDefeated);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemWeakPointsExposed, bool, bExposed, FName, AttackId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGolemWeakPointHit, FName, Id, float, Damage, float, HealthRemaining);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemWeakPointBroken, FName, Id);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemToppled, float, Duration);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGolemRecoverFromTopple);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemCriticalHit, float, Damage, FName, Id);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemArenaCrystalSpawned, int32, Id, FVector, Location);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemArenaCrystalHit, int32, Id, int32, HitsRemaining);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemArenaCrystalDestroyed, int32, Id, FVector, Location);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGolemArenaCrystalsCleared);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemBigCrystalSpawned, FVector, Location);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemBigCrystalHit, int32, HitsRemaining);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemBigCrystalBroken, float, DamageDealt);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGolemAnimCue, FName, Tag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolemRepelledPlayer, AActor*, Player, FVector, Direction);

UCLASS(ClassGroup = (AI), Blueprintable, meta = (BlueprintSpawnableComponent))
class ETHERIA_API UGolemBossComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGolemBossComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Golem") void ActivateBoss();

	UFUNCTION(BlueprintCallable, Category = "Golem") void DeactivateBoss();

	UFUNCTION(BlueprintPure, Category = "Golem") bool IsBossActive() const { return bActivated; }

	UFUNCTION(BlueprintCallable, Category = "Golem") bool ForceAttack(FName AttackId);

	UFUNCTION(BlueprintCallable, Category = "Golem") void TriggerStrikeNow();

	UFUNCTION(BlueprintCallable, Category = "Golem") void EndAttackNow();

	UFUNCTION(BlueprintCallable, Category = "Golem") void LaunchRockNow();

	UFUNCTION(BlueprintCallable, Category = "Golem") void FireAnimCue(FName Tag) { OnGolemAnimCue.Broadcast(Tag); }

	UFUNCTION(BlueprintCallable, Category = "Golem") void InterruptAttack();

	UFUNCTION(BlueprintCallable, Category = "Golem|AirZone") void OpenAirZone(FVector Location, float Radius, float Lifetime);

	UFUNCTION(BlueprintCallable, Category = "Golem|AirZone") void ClearAirZones();

	UFUNCTION(BlueprintPure, Category = "Golem|AirZone") bool HasActiveAirZone() const { return ActiveAirZones.Num() > 0; }

	UFUNCTION(BlueprintPure, Category = "Golem|AirZone") bool HasAirZoneWithTime(float RequiredSeconds) const;
	UFUNCTION(BlueprintPure, Category = "Golem|AirZone") bool IsLocationInAirZone(FVector Location) const;
	UFUNCTION(BlueprintPure, Category = "Golem|AirZone") const TArray<FGolemAirZone>& GetActiveAirZones() const { return ActiveAirZones; }

	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") bool HitWeakPoint(FName Id, float Damage, AActor* Instigator);

	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") bool HitArm(FName ArmId, float Damage, AActor* Instigator);

	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") bool RouteBodyHit(float Damage, AActor* Instigator);

	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") void SetWeakPointVulnerable(FName Id, bool bVulnerable);

	UFUNCTION(BlueprintCallable, Category = "Golem|Crystals") bool HitArenaCrystal(int32 CrystalId, float PlayerDamage, AActor* Instigator);

	UFUNCTION(BlueprintCallable, Category = "Golem|Crystals") bool HitBigCrystal(float PlayerDamage, AActor* Instigator);

	UFUNCTION(BlueprintPure, Category = "Golem|Crystals") bool IsBigCrystalActive() const { return bBigCrystalActive; }
	UFUNCTION(BlueprintPure, Category = "Golem|Crystals") const TArray<FGolemArenaCrystal>& GetArenaCrystals() const { return ArenaCrystals; }

	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") void ForceTopple();
	UFUNCTION(BlueprintCallable, Category = "Golem|WeakPoint") void EndTopple();

	UFUNCTION(BlueprintPure, Category = "Golem|WeakPoint") bool IsToppled() const { return bToppled; }
	UFUNCTION(BlueprintPure, Category = "Golem|WeakPoint") bool IsWeakPointBroken(FName Id) const;
	UFUNCTION(BlueprintPure, Category = "Golem|WeakPoint") bool IsWeakPointVulnerable(FName Id) const;
	UFUNCTION(BlueprintPure, Category = "Golem|WeakPoint") FVector GetWeakPointLocation(FName Id) const;

	UFUNCTION(BlueprintCallable, Category = "Golem|Damage")
	int32 ApplyRadialBurst(FVector Center, float Radius, float Damage, bool bAirborneIsSafe, float Knockback, FName AttackId, float MinSafeAltitude = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Golem|Damage")
	int32 ApplyRingBurst(FVector Center, float InnerRadius, float OuterRadius, float Damage, bool bAirborneIsSafe, float Knockback, FName AttackId);

	UFUNCTION(BlueprintCallable, Category = "Golem|Damage")
	int32 ApplyLineDamage(FVector A, FVector B, float HalfWidth, float Damage, bool bAirborneIsSafe, bool b3D, float Knockback, FName AttackId, float MinSafeAltitude = 0.f, FVector KnockbackDir = FVector::ZeroVector);

	UFUNCTION(BlueprintPure, Category = "Golem") EGolemAttackState GetAttackState() const { return State; }
	UFUNCTION(BlueprintPure, Category = "Golem") FName GetCurrentAttackId() const;
	UFUNCTION(BlueprintPure, Category = "Golem") int32 GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "Golem") FName GetPendingForcedAttackId() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golem|Arena") FVector GetArenaCentre() const;
	virtual FVector GetArenaCentre_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golem|Arena") float GetArenaRadius() const;
	virtual float GetArenaRadius_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golem|Arena") FVector ResolveTargetLocation() const;
	virtual FVector ResolveTargetLocation_Implementation() const;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Attacks") TArray<FGolemAttackConfig> Attacks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Phases") TArray<FGolemPhaseConfig> Phases;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing", meta = (ClampMin = "0",
		ToolTip = "Wait (s) between attacks so the player can breathe and hit back. 2-3 is a fair demo pace.")) float GlobalCooldown = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing", meta = (ClampMin = "0")) float GlobalCooldownRandom = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing", meta = (ClampMin = "0", ClampMax = "1")) float RepeatPenalty = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing") bool bAutoActivateOnTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Encounter") TObjectPtr<UAnimMontage> IntroMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Encounter", meta = (ClampMin = "0")) float IntroDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Pacing", meta = (ClampMin = "0.02")) float BrainInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Arena") FVector ArenaCentre = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Arena", meta = (ClampMin = "100")) float ArenaRadius = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Arena") bool bArenaCentreRelativeToActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Air Zone",
		meta = (ToolTip = "Set to BP_WindColumn (your glider air-zone). The rock-throw spawns it at the hole; it is destroyed when the zone closes.")) TSubclassOf<AActor> AirZoneActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint") TArray<FGolemWeakPoint> WeakPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint") bool bArmsAlwaysHittable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint",
		meta = (ToolTip = "ON: arm crystals only take break-progress while exposed by a slam. Body damage still lands anytime.")) bool bArmCrystalsChipOnlyWhenExposed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "0", ClampMax = "1"))
	float UnexposedArmDamageMultiplier = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "1",
		ToolTip = "Boss takes this much more damage during an attack's recovery window (the designed punish moment).")) float RecoveryDamageTakenMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "1")) int32 ArmCrystalHitsToBreak = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "1")) float BrokenArmDamageMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "0.5")) float ToppleDuration = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "1")) float HeadCritMultiplier = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint") bool bResetArmsOnRecover = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|WeakPoint", meta = (ClampMin = "0")) float WeakPointExposeLinger = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals",
		meta = (ToolTip = "Two-hand slam plants arena crystals (the new stun mechanic). OFF = use the arm-crystal/topple flow instead.")) bool bSlamPlantsArenaCrystals = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "1")) int32 ArenaCrystalHitsToBreak = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "0",
		ToolTip = "On destroy, a crystal deals player damage × this to the boss. 6 = the x6 burst.")) float ArenaCrystalBreakDamageMult = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "1")) int32 BigCrystalHitsToBreak = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "0", ClampMax = "1",
		ToolTip = "Break the big crystal → remove this fraction of the boss MAX HP. 0.5 = half its life.")) float BigCrystalHealthFraction = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "0.5",
		ToolTip = "Seconds the golem stays stunned/down after you break the big crystal, before it gets back up.")) float BigCrystalStunDuration = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals", ClampMin = "0")) float ArenaCrystalEdgeMargin = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals",
		ToolTip = "Crystal BP planted at each fissure point. Child of AGolemCrystal with a mesh. The slam spawns & destroys it automatically.")) TSubclassOf<AGolemCrystal> ArenaCrystalActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Crystals", meta = (EditCondition = "bSlamPlantsArenaCrystals",
		ToolTip = "Big crystal BP opened at the arena centre during the stun. Child of AGolemCrystal with a mesh.")) TSubclassOf<AGolemCrystal> BigCrystalActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Facing") bool bFaceTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Facing", meta = (ClampMin = "1", EditCondition = "bFaceTarget")) float FaceTurnSpeed = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Facing", meta = (ClampMin = "5", ClampMax = "180", EditCondition = "bFaceTarget"))
	float AttackFacingToleranceDeg = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Facing", meta = (ClampMin = "0", ClampMax = "0.9"))
	float WindupAimTrackFraction = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Contact") bool bRepelOnContact = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Contact", meta = (ClampMin = "0", EditCondition = "bRepelOnContact")) float RepulsionRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Contact", meta = (ClampMin = "0", EditCondition = "bRepelOnContact")) float RepulsionForce = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Contact", meta = (ClampMin = "0", EditCondition = "bRepelOnContact")) float RepulsionInterval = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Sockets") TArray<FName> EyeSocketNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Sockets") FName ThrowSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Sockets", meta = (ClampMin = "0")) float BombardmentSpawnHeight = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Decals") bool bSpawnTelegraphDecals = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Decals",
		meta = (ToolTip = "Assign a ground decal material (e.g. a red circle). Decals spawn at rock/impact/fissure spots on their own once this is set.")) TObjectPtr<UMaterialInterface> TelegraphDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Decals", meta = (ClampMin = "10")) float DecalProjectionDepth = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks") bool bSpawnRockMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks") TArray<TObjectPtr<UStaticMesh>> RockMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0.01")) float RockMeshScale = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockThrowArcHeight = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockReleaseTime = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockSpinSpeed = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks") TSubclassOf<AActor> RockActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Rocks", meta = (ClampMin = "0")) float RockThrowTravelTime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|UI") TSubclassOf<UGolemBossBarWidget> BossBarWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|UI") FText BossDisplayName = FText::FromString(TEXT("Golem"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|UI") int32 BossBarZOrder = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Audio") TObjectPtr<USoundBase> CombatMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Audio", meta = (ClampMin = "0")) float CombatMusicFadeIn = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Audio", meta = (ClampMin = "0")) float CombatMusicFadeOut = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Debug") bool bDrawDebugHazards = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Debug") bool bDrawDebugText = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|Debug", meta = (ClampMin = "0")) float DebugImpactLinger = 0.6f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	void BrainTick();
	void OnWindupElapsed();
	void OnRecoveryElapsed();
	void TickActive(float DeltaTime);
	void TickFacing(float DeltaTime);
	void TickContactRepulsion();

	void BeginAttack(int32 Index);
	void EnterActive();
	void EnterRecovery();
	void FinishAttack(bool bInterrupted);
	void ClearSequenceTimers();

	int32 SelectNextAttack();
	bool IsAttackUsable(int32 Index) const;
	int32 FindAttackIndex(FName AttackId) const;
	int32 FindAirZoneOpener() const;

	void BuildTelegraph(const FGolemAttackConfig& Cfg, FGolemTelegraph& Out);
	void DoStrike();
	void TickHazard(float DeltaTime, bool bForceFinal = false);
	void GetEyeOrigins(TArray<FVector>& Out) const;
	FVector GetThrowOrigin() const;
	UDecalComponent* SpawnTelegraphDecal(const FVector& Location, float RadiusXY, float LifeSpan) const;

	int32 ApplyLineDamageOncePerVictim(FVector A, FVector B, float HalfWidth, float Damage, bool bAirborneIsSafe, bool b3D,
		float Knockback, FName AttackId, float MinSafeAltitude, FVector KnockbackDir);

	void TickWindupAim(float DeltaTime);
	AGolemFallingRock* SpawnFallingRock(const FVector& Origin, const FVector& Target, float TravelTime, float ArcHeight);
	void SpawnHeldRock();
	void ApplyRockImpactDecal(AGolemFallingRock* Rock, const FVector& Target, float Radius, float StaticLifeSpan);
	void BuildBeams(const FGolemAttackConfig& Cfg, float RotateDeg, TArray<FGolemBeamSegment>& Out) const;
	void BuildBeamsToPoint(const FGolemAttackConfig& Cfg, const FVector& EndPoint, TArray<FGolemBeamSegment>& Out) const;

	void SpawnBeamVFX(const FGolemAttackConfig& Cfg, const TArray<FGolemBeamSegment>& Beams);
	void UpdateBeamVFX(const TArray<FGolemBeamSegment>& Beams);
	void ClearBeamVFX();
	void SpawnBeamFireVFX(UNiagaraSystem* System);
	void SpawnBeamMesh(const FGolemAttackConfig& Cfg, const TArray<FGolemBeamSegment>& Beams);
	void UpdateBeamMesh(const TArray<FGolemBeamSegment>& Beams);
	void ClearBeamMesh();
	FVector ResolveArenaCentre() const;
	FVector ClampToArena(const FVector& P, float Margin = 0.f) const;
	FVector RandomArenaPoint(float SpreadFraction) const;
	FVector ProjectToGround(const FVector& P) const;
	bool ResolveSocketPoints(const FGolemAttackConfig& Cfg, TArray<FVector>& Out) const;

	void GatherTargets(FVector Center, float Radius, TArray<AActor*>& Out) const;
	bool IsActorAirborne(const AActor* A) const;
	float CurrentDamageScale() const;
	void DealDamage(AActor* Victim, float Damage, FVector FromLocation, float Knockback, FName AttackId, FVector KnockbackDirOverride = FVector::ZeroVector);

	void DealDamageToBoss(float Amount, AActor* Instigator);

	void TickAirZones(float DeltaTime);

	void ExposeArmWeakPoints(bool bExpose, FName AttackId);
	void OnExposeLingerElapsed();
	void CheckTopple();
	void Topple();
	void OnToppleElapsed();
	FGolemWeakPoint* FindWeakPoint(FName Id);
	const FGolemWeakPoint* FindWeakPoint(FName Id) const;

	void PlantArenaCrystals();
	void ClearArenaCrystals();
	bool AllArenaCrystalsDestroyed() const;
	void SpawnBigCrystal();
	void EndBigCrystal();

	void DrawTelegraphDebug(const FGolemTelegraph& T, float Lifetime) const;
	void DrawImpactDebug(const FVector& Center, float Radius, bool bAirborneIsSafe) const;
	void DrawLiveDebug(float Lifetime) const;

	void UpdatePhaseFromHealth();
	float GetPhaseCooldownScale() const;

	UFUNCTION() void HandleOwnerDamaged(AActor* Instigator);
	UFUNCTION() void HandleOwnerDied();

	UFUNCTION() void ShowBossUIAndMusic();
	UFUNCTION() void HideBossUIAndMusic();
	UFUNCTION() void HandleBossHealthChanged(float NewHealth, float MaxHealth);

	UPROPERTY() TObjectPtr<ABaseAICharacter> OwnerCharacter;
	UPROPERTY() TObjectPtr<UHealthComponent> OwnerHealth;
	UPROPERTY(Transient) TObjectPtr<UGolemBossBarWidget> BossBar;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> CombatMusicComp;
	UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraComponent>> BeamVFXComps;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> BeamMeshComps;

	EGolemAttackState State = EGolemAttackState::Idle;
	bool bActivated = false;

	int32 CurrentAttackIndex = -1;
	int32 LastAttackIndex = -1;
	int32 PendingForcedAttack = -1;
	int32 CurrentPhase = 0;

	float ActiveTimer = 0.f;
	float ActiveDurationCache = 0.f;
	float DamageTickAccum = 0.f;
	float NextAttackReadyTime = 0.f;
	float IntroEndTime = 0.f;
	float NextRepulsionTime = 0.f;
	TArray<float> AttackReadyTimes;

	FGolemTelegraph CurrentTelegraph;
	TArray<bool> BombardImpactFired;
	TArray<bool> BombardLaunched;
	TSet<TWeakObjectPtr<AActor>> HazardVictimsThisAttack;
	TWeakObjectPtr<UDecalComponent> TrackedTelegraphDecal;
	float LastBrainTime = -1.f;

	FVector BeamRiseGround = FVector::ZeroVector;
	FVector BeamRiseHigh = FVector::ZeroVector;

	UPROPERTY() TArray<FGolemAirZone> ActiveAirZones;
	int32 NextAirZoneId = 1;

	UPROPERTY() TObjectPtr<AGolemFallingRock> HeldRock;

	bool bToppled = false;
	bool bRockLaunched = false;
	FName ExposingAttackId = NAME_None;

	UPROPERTY() TArray<FGolemArenaCrystal> ArenaCrystals;
	int32 NextArenaCrystalId = 1;
	bool bBigCrystalActive = false;
	int32 BigCrystalHitsRemaining = 0;
	UPROPERTY() TObjectPtr<AActor> BigCrystalActor;

	FTimerHandle BrainTimerHandle;
	FTimerHandle WindupTimerHandle;
	FTimerHandle RecoveryTimerHandle;
	FTimerHandle ToppleTimerHandle;
	FTimerHandle ExposeLingerTimerHandle;
	FTimerHandle RockReleaseTimerHandle;
	FTimerHandle BeamFireVFXTimerHandle;
};
