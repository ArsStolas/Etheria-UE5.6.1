/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "Zhailendra"
 * Class: "UCombatComponent" - Header
 */

#pragma once

#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class ABaseCharacter;
class UCharacterMovementComponent;
class UAnimMontage;
class ULockTargetComponent;
class USkeletalMeshComponent;
class UMaterialInterface;
class UDecalComponent;
class UWeaponData;
class AActor;
class UCharacterStateComponent;

#pragma region STRUCT ET ENUM
/* Basic enums */
UENUM(BlueprintType) enum class EStance : uint8 { Both=0, GroundOnly=1, AirOnly=2 };
UENUM(BlueprintType) enum class EAttackType : uint8 { Melee=0, Ranged=1, AoE=2 };
UENUM(BlueprintType) enum class ETraceShape : uint8 { Line=0, Sphere=1, Capsule=2 };
UENUM(BlueprintType) enum class EPerfectKind : uint8 { None=0, Parry=1, Dodge=2 };
UENUM(BlueprintType) enum class ECombatCuePhase : uint8 { Start=0, Impact=1, End=2 };
UENUM(BlueprintType) enum class EChargeShape : uint8 { None=0, Radial=1, FrontalRect=2 };
UENUM(BlueprintType) enum class ERangedMode : uint8 { None=0, Line=1 };

/* Charge structs */
USTRUCT(BlueprintType)
struct FChargeLevelConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float Time = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float DamageMultiplier = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float RangeMultiplier = 1.2f;
};

USTRUCT(BlueprintType)
struct FChargeSpecConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") bool bChargeable = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") EChargeShape Shape = EChargeShape::Radial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") bool bShowTelegraph = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float MaxRadius = 400.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float MaxLength = 600.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float MaxWidth = 300.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") TArray<FChargeLevelConfig> Levels;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") TArray<UAnimMontage*> ReleaseMontages;
};

/* Attack spec */
USTRUCT(BlueprintType)
struct FAttackSpecConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") FName Group = NAME_None; // "Light", "Heavy"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") EStance Stance = EStance::GroundOnly;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") FName AttackId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") EAttackType AttackType = EAttackType::Melee;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float BaseDamage = 25.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float CritChance = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float CritMultiplier = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float HitStartDelay = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float HitWindow = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float Cooldown = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") ETraceShape TraceShape = ETraceShape::Sphere;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") float Range = 220.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") float Radius = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") float CapsuleHalfHeight = 45.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") UAnimMontage* Montage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") FName MontageSection = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") FChargeSpecConfig Charge;
};

/* Combo spec */
USTRUCT(BlueprintType)
struct FComboStepConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") FName AttackId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") float DamageOverride = -1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") float CritChanceOverride = -1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") float CritMultiplierOverride = -1.f;
};

USTRUCT(BlueprintType)
struct FComboSpecConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") FName ComboId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") TArray<FComboStepConfig> Steps;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo", meta=(ClampMin="0.1")) float ResetDelay = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo", meta=(ClampMin="0.0")) float Cooldown = 0.0f;
};
#pragma endregion

/* Delegates */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEEOnCue, FName, CueName, ECombatCuePhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEEOnAttackEvent, FName, AttackId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEEOnHitEvent, AActor*, HitActor, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEEOnHitCritEvent, AActor*, HitActor, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEEOnPerfectEvent, EPerfectKind, PerfectKind);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();
    
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    const TArray<FAttackSpecConfig>& GetAttacks() const { return Attacks; }
    const TArray<FComboSpecConfig>& GetCombos() const { return Combos; }

    /** Returns the currently active weapon data if any. */
    UPROPERTY() UWeaponData* CurrentWeaponData = nullptr;
    UFUNCTION(BlueprintCallable, Category = "Combat")
    FORCEINLINE UWeaponData* GetCurrentWeaponData() const { return CurrentWeaponData ? CurrentWeaponData : WeaponData; }
    
    /** Sets a new weapon data (called by Equipment or Inventory). */
    void SetCurrentWeaponData(UWeaponData* NewWeaponData);
    
    UFUNCTION(BlueprintCallable, Category="Combat") void SetAttacks(const TArray<FAttackSpecConfig>& InAttacks);
    UFUNCTION(BlueprintCallable, Category="Combat") void SetCombos(const TArray<FComboSpecConfig>& InCombos);
    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackPrimary();
    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackById(FName AttackId, float ChargeLevel = 0.f);
    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackGroup(FName GroupId); // picks ground/air variant automatically
    
    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void RequestComboAdvance();

    UFUNCTION(BlueprintCallable, Category="Combat|Target") void SetExternalTarget(AActor* InTarget);
    UFUNCTION(BlueprintPure,   Category="Combat|Target") AActor* GetCurrentTarget() const;

    UFUNCTION(BlueprintCallable, Category="Combat|Window") void BeginAttackWindow();
    UFUNCTION(BlueprintCallable, Category="Combat|Window") void EndAttackWindow();

    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void SetParryHeld(bool bHeld);
    UFUNCTION(BlueprintPure,   Category="Combat|Parry") bool IsParryHeld() const { return bParryHeld; }
    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void BeginPerfectParryWindow();
    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void EndPerfectParryWindow();
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void StartDodgeIFrames(float DurationOverride = -1.f);
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void BeginPerfectDodgeWindow();
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void EndPerfectDodgeWindow();
    UFUNCTION(BlueprintPure,   Category="Combat|Dodge") bool IsInIFrames() const { return bInDodgeIFrames; }

    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void BeginCharge(FName AttackId, float ExpectedDuration);
    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void UpdateChargeProgress(float DeltaTime);
    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void EndCharge(bool bCanceled);

    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void BeginComboWindow(FName ComboId);
    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void EndComboWindow(FName ComboId);

    UFUNCTION(BlueprintPure, Category="Combat|Combo") float GetComboCooldownRemaining(FName ComboId) const;
    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void ClearAllComboCooldowns();

    // Weapon data hot-swap
    UFUNCTION(BlueprintCallable, Category="Combat|Weapon") void SetWeaponData(UWeaponData* InData);
    UFUNCTION(BlueprintCallable, Category="Combat|Weapon") void ApplyWeaponData();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Weapon") UWeaponData* WeaponData = nullptr;

    // Combo buffer timeout (editor tweak)
    UPROPERTY(EditAnywhere, Category="Combat|Combo", meta=(ClampMin="0.0")) float MaxComboBufferTime = 1.0f; // seconds
    UPROPERTY(EditAnywhere, Category="Combat|Combo", meta=(ClampMin="0.0")) float DefaultComboStartCooldown = 0.0f;
    float ComboBufferExpireAt = 0.0f;

    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnCue OnCue;
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnAttackEvent OnAttackStarted;
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnAttackEvent OnAttackEnded;
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnHitEvent OnHit;
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnHitCritEvent OnHitCrit;
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnPerfectEvent OnPerfect;

    UFUNCTION(BlueprintPure, Category="Combat|State") bool IsInCooldown() const;
    UFUNCTION(BlueprintPure, Category="Combat|State") bool IsInAttackWindow() const { return bInAttackWindow; }
    UFUNCTION(BlueprintPure, Category="Combat|State") bool IsAttackActive() const { return CurrentAttackId != NAME_None; }
    
    void PushRecentHitActor(AActor* A);
    void ClearRecentHitActors();
    void GetRecentHitActors(TArray<AActor*>& Out) const;

    UFUNCTION(BlueprintPure, Category="Combat|Input") bool IsJumpBlocked() const;
    UFUNCTION(BlueprintPure, Category="Combat|Input") bool IsCrouchBlocked() const;
    UFUNCTION(BlueprintCallable, Category="Combat|Input") void PushInputLock(FName LockId, bool bBlockJump, bool bBlockCrouch);
    UFUNCTION(BlueprintCallable, Category="Combat|Input") void PopInputLock(FName LockId);

    // Force a fallback AnimBP (with a valid Slot) while playing montages
    UPROPERTY(EditAnywhere, Category="Combat|Montage")
    bool bForceFallbackAnimBPForMontages = true;

    UPROPERTY(EditAnywhere, Category="Combat|Montage")
    TSubclassOf<class UAnimInstance> FallbackMontageAnimClass;

    // Runtime bookkeeping
    TSubclassOf<class UAnimInstance> SavedAnimClass = nullptr;
    bool bUsingFallbackAnimClass = false;

    // Internal hooks
    UFUNCTION()
    void HandleMontageEnded_RestoreAnimClass(class UAnimMontage* Montage, bool bInterrupted);

    // Called before any Montage_Play / JumpToSection to ensure a valid Slot
    void PrePlayMontageSafety(const FAttackSpecConfig& Spec);

#pragma region RANGED COMBAT
    
    /** Start and stop auto fire depending on weapon type. */
    void StartRangedFire();
    void StopRangedFire();
    
#pragma endregion
    
protected:
    virtual void BeginPlay() override;

#pragma region RANGED COMBAT

    /** Timer for auto fire control. */
    FTimerHandle AutoFireHandle;

    /** Internal fire helper */
    void PerformRangedFire();
    
    /** Called every frame to handle charge build-up for bow */
    void UpdateCharge(float DeltaTime);

    /** For bow charge (0–1 range) */
    float CurrentChargeLevel = 0.f;

    /** True if charging is active (for bow) */
    bool bIsCharging = false;

#pragma endregion

private:
    
#pragma region "Internal execution"
    bool ResolveOwnerRefs();
    bool CanExecuteAttack(const FName AttackId) const;
    void ExecuteAttack(const FAttackSpecConfig& Spec, float DamageScale = 1.f, float RangeScale = 1.f);
    void OpenWindowWithTimers(const FAttackSpecConfig& Spec);
    void CloseCurrentAttack();
    void PerformMeleeTrace(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale);
    void PerformAoE(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale);
    void PerformFrontalRect(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale);
#pragma endregion

#pragma region "Facing and target assist"
    void NudgeOwnerRotationToward(const FVector& Direction, float MaxYawDeltaDeg) const;
    FVector GetEyeLocationForward(FVector& OutForward) const;
    AActor* ResolveBestTarget(const FVector& EyeLoc, const FVector& Forward) const;
    FVector ApplyMagnetismBias(const FVector& RawForward, const FVector& EyeLoc) const;
#pragma endregion

#pragma region "Damage and defense"
    float ComputeFinalDamageForTarget(AActor* Victim, float RawDamage, bool& bOutCrit, float CritChance, float CritMultiplier) const;
#pragma endregion

#pragma region "Perfect boosts"
    void ApplyPerfectBoost(EPerfectKind Kind);
    void RestoreBoosts();
#pragma endregion

#pragma region "Search helpers"
    const FAttackSpecConfig* FindAttack(FName AttackId) const;
#pragma endregion

#pragma region "Combo runtime helpers"
    void AdvanceComboIfRequested();
#pragma endregion

#pragma region "Telegraph"
    void SpawnTelegraph();
    void UpdateTelegraph(float Alpha);
    void DestroyTelegraph();
#pragma endregion

#pragma region "Utils"
    TArray<AActor*> UniqueActorsFromHits(const TArray<FHitResult>& Hits) const;
    void PlayOrJumpMontageSection(const FAttackSpecConfig& Spec);
    void PerformRangedLine(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale);
#pragma endregion

#pragma region "Config"
    UPROPERTY(EditAnywhere, Category="Combat|Config") TArray<FAttackSpecConfig> Attacks;
    UPROPERTY(EditAnywhere, Category="Combat|Config") TArray<FComboSpecConfig> Combos;
    UPROPERTY(EditAnywhere, Category="Combat|Config") TEnumAsByte<ECollisionChannel> DamageTraceChannel = ECC_Pawn;
    UPROPERTY(EditAnywhere, Category="Combat|Config") bool bIgnoreOwner = true;
    UPROPERTY(EditAnywhere, Category="Combat|Magnetism", meta=(ClampMin="0.0", ClampMax="90.0")) float MagnetismAngleDeg = 35.f;
    UPROPERTY(EditAnywhere, Category="Combat|Magnetism", meta=(ClampMin="0.0", ClampMax="1.0")) float MagnetismStrength = 0.55f;
    UPROPERTY(EditAnywhere, Category="Combat|Facing", meta=(ClampMin="0.0", ClampMax="45.0")) float MaxAutoYawOnAttackDeg = 20.f;
    UPROPERTY(EditAnywhere, Category="Combat|Direction") bool bUseMeshFacing = true;
#pragma endregion

#pragma region "Ranged"
    UPROPERTY(EditAnywhere, Category="Combat|Ranged") ERangedMode RangedMode = ERangedMode::Line;
    UPROPERTY(EditAnywhere, Category="Combat|Ranged") FName MuzzleSocketName = "Muzzle";
#pragma endregion
    
#pragma region "Parry"
    UPROPERTY(EditAnywhere, Category="Combat|Parry", meta=(ClampMin="0.0", ClampMax="1.0")) float ParryDamageFactorWhileHeld = 0.2f;
    UPROPERTY(EditAnywhere, Category="Combat|Parry") TArray<UAnimMontage*> ParryHitReactionMontages;
    UPROPERTY(EditAnywhere, Category="Combat|Parry", meta=(ClampMin="0.1")) float ParryMoveSpeedMultiplier = 0.6f;
#pragma endregion

#pragma region "Dodge"
    UPROPERTY(EditAnywhere, Category="Combat|Dodge", meta=(ClampMin="0.05")) float DodgeIFrameDuration = 0.35f;
#pragma endregion

#pragma region "Perfect boost"
    UPROPERTY(EditAnywhere, Category="Combat|Perfect", meta=(ClampMin="1.0")) float PerfectMoveSpeedMultiplier = 1.25f;
    UPROPERTY(EditAnywhere, Category="Combat|Perfect", meta=(ClampMin="1.0")) float PerfectAnimRateMultiplier = 1.25f;
    UPROPERTY(EditAnywhere, Category="Combat|Perfect", meta=(ClampMin="0.1")) float PerfectBoostDuration = 2.0f;
#pragma endregion

#pragma region "Telegraph visuals"
    UPROPERTY(EditAnywhere, Category="Combat|Telegraph") UMaterialInterface* RadialDecalMaterial = nullptr;
    UPROPERTY(EditAnywhere, Category="Combat|Telegraph") UMaterialInterface* RectDecalMaterial = nullptr;
#pragma endregion

#pragma region "Runtime"
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") FName CurrentAttackId = NAME_None;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") FName LastAttackId = NAME_None;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bInAttackWindow = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") float CooldownEndTime = 0.f;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bParryHeld = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bPerfectParryWindow = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bPerfectDodgeWindow = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bInDodgeIFrames = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") TWeakObjectPtr<AActor> ExternalTarget;
    TArray<TWeakObjectPtr<AActor>> RecentHitActors;
    TSet<FName> JumpLocks;
    TSet<FName> CrouchLocks;
#pragma endregion
    
#pragma region "Debug"
    UPROPERTY(EditAnywhere, Category="Combat|Debug") bool bDebugDraw = false;
#pragma endregion
    
#pragma region "Charge runtime"
    bool bCharging = false;
    float ChargeStartTime = 0.f;
    float ChargeExpectedDuration = 0.f;
    float ChargeAccumulated = 0.f;
    int32 ChargeLevelIndex = -1;
    int32 ObservedChargeLevel = -1;
#pragma endregion

#pragma region "Combo runtime"
    FName ActiveComboId = NAME_None;
    int32 ActiveComboStep = -1;
    bool bComboWindowOpen = false;
    bool bComboAdvanceRequested = false;
    float ComboResetTime = 0.f;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") TMap<FName, float> ComboCooldownUntil;
#pragma endregion

#pragma region "Cached"
    TWeakObjectPtr<ABaseCharacter> OwnerCharacter;
    TWeakObjectPtr<USkeletalMeshComponent> OwnerMesh;
    TWeakObjectPtr<UCharacterMovementComponent> MoveComp;
    TWeakObjectPtr<ULockTargetComponent> LockComp;
    TWeakObjectPtr<UCharacterStateComponent> StateComp;
    float BaseWalkSpeed = -1.f;
    float BaseGlobalAnimRate = 1.f;
#pragma endregion

#pragma region "Telegraph component"
    UDecalComponent* ActiveDecal = nullptr;
#pragma endregion
};