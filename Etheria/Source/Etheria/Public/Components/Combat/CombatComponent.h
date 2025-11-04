/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UCombatComponent" - Header
 */
#pragma once

#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UCharacterMovementComponent;
class UAnimMontage;
class ULockTargetComponent;
class USkeletalMeshComponent;
class UMaterialInterface;
class UDecalComponent;
class AActor;

/* Basic enums */
UENUM(BlueprintType) enum class EEEAttackType : uint8 { Melee=0, Ranged=1, AoE=2 };
UENUM(BlueprintType) enum class EEETraceShape : uint8 { Line=0, Sphere=1, Capsule=2 };
UENUM(BlueprintType) enum class EEEPerfectKind : uint8 { None=0, Parry=1, Dodge=2 };
UENUM(BlueprintType) enum class EEECombatCuePhase : uint8 { Start=0, Impact=1, End=2 };
UENUM(BlueprintType) enum class EEEChargeShape : uint8 { None=0, Radial=1, FrontalRect=2 };

/* Charge structs */
USTRUCT(BlueprintType)
struct FEEChargeLevel
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float Time = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float DamageMultiplier = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float RangeMultiplier = 1.2f;
};

USTRUCT(BlueprintType)
struct FEEChargeSpec
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") bool bChargeable = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") EEEChargeShape Shape = EEEChargeShape::Radial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") bool bShowTelegraph = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float MaxRadius = 400.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float MaxLength = 600.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") float MaxWidth = 300.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") TArray<FEEChargeLevel> Levels;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") TArray<UAnimMontage*> ReleaseMontages;
};

/* Attack spec */
USTRUCT(BlueprintType)
struct FEEAttackSpec
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") FName AttackId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack") EEEAttackType AttackType = EEEAttackType::Melee;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float BaseDamage = 25.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float CritChance = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float CritMultiplier = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float HitStartDelay = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float HitWindow = 0.15f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Timing", meta=(ClampMin="0.0")) float Cooldown = 0.35f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") EEETraceShape TraceShape = EEETraceShape::Sphere;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") float Range = 220.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") float Radius = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee Trace") float CapsuleHalfHeight = 45.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") UAnimMontage* Montage = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") FName MontageSection = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Charge") FEEChargeSpec Charge;
};

/* Combo spec */
USTRUCT(BlueprintType)
struct FEEComboStep
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") FName AttackId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") float DamageOverride = -1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") float CritChanceOverride = -1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") float CritMultiplierOverride = -1.f;
};

USTRUCT(BlueprintType)
struct FEEComboSpec
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") FName ComboId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo") TArray<FEEComboStep> Steps;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combo", meta=(ClampMin="0.1")) float ResetDelay = 1.0f;
};

/* Delegates */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEEOnCue, FName, CueName, EEECombatCuePhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEEOnAttackEvent, FName, AttackId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEEOnHitEvent, AActor*, HitActor, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEEOnHitCritEvent, AActor*, HitActor, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEEOnPerfectEvent, EEEPerfectKind, PerfectKind);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

    UFUNCTION(BlueprintCallable, Category="Combat") void SetAttacks(const TArray<FEEAttackSpec>& InAttacks);
    UFUNCTION(BlueprintCallable, Category="Combat") void SetCombos(const TArray<FEEComboSpec>& InCombos);
    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackPrimary();
    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackById(FName AttackId);

    /* Combo control from input */
    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void RequestComboAdvance();

    /* Targeting */
    UFUNCTION(BlueprintCallable, Category="Combat|Target") void SetExternalTarget(AActor* InTarget);
    UFUNCTION(BlueprintPure,   Category="Combat|Target") AActor* GetCurrentTarget() const;

    /* Attack windows driven by notifies */
    UFUNCTION(BlueprintCallable, Category="Combat|Window") void BeginAttackWindow();
    UFUNCTION(BlueprintCallable, Category="Combat|Window") void EndAttackWindow();

    /* Parry and Dodge */
    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void SetParryHeld(bool bHeld);
    UFUNCTION(BlueprintPure,   Category="Combat|Parry") bool IsParryHeld() const { return bParryHeld; }
    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void BeginPerfectParryWindow();
    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void EndPerfectParryWindow();
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void StartDodgeIFrames(float DurationOverride = -1.f);
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void BeginPerfectDodgeWindow();
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void EndPerfectDodgeWindow();
    UFUNCTION(BlueprintPure,   Category="Combat|Dodge") bool IsInIFrames() const { return bInDodgeIFrames; }

    /* Charge API driven by AnimNotifyState_ChargeWindow */
    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void BeginCharge(FName AttackId, float ExpectedDuration);
    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void UpdateChargeProgress(float DeltaTime);
    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void EndCharge(bool bCanceled);

    /* Combo notifies */
    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void BeginComboWindow(FName ComboId);
    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void EndComboWindow(FName ComboId);

    /* Events */
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnCue OnCue;
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnAttackEvent OnAttackStarted;
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnAttackEvent OnAttackEnded;
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnHitEvent OnHit;
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnHitCritEvent OnHitCrit;
    UPROPERTY(BlueprintAssignable, Category="Events") FEEOnPerfectEvent OnPerfect;

    UFUNCTION(BlueprintPure, Category="Combat|State") bool IsInCooldown() const;
    UFUNCTION(BlueprintPure, Category="Combat|State") bool IsInAttackWindow() const { return bInAttackWindow; }

protected:
    virtual void BeginPlay() override;

private:
    /* Internal execution */
    bool ResolveOwnerRefs();
    bool CanExecuteAttack(const FName AttackId) const;
    void ExecuteAttack(const FEEAttackSpec& Spec, float DamageScale = 1.f, float RangeScale = 1.f);
    void OpenWindowWithTimers(const FEEAttackSpec& Spec);
    void CloseCurrentAttack();
    void PerformMeleeTrace(const FEEAttackSpec& Spec, float DamageScale, float RangeScale);
    void PerformAoE(const FEEAttackSpec& Spec, float DamageScale, float RangeScale);
    void PerformFrontalRect(const FEEAttackSpec& Spec, float DamageScale, float RangeScale);

    /* Facing and target assist */
    void NudgeOwnerRotationToward(const FVector& Direction, float MaxYawDeltaDeg) const;
    FVector GetEyeLocationForward(FVector& OutForward) const;
    AActor* ResolveBestTarget(const FVector& EyeLoc, const FVector& Forward) const;
    FVector ApplyMagnetismBias(const FVector& RawForward, const FVector& EyeLoc) const;

    /* Damage and defense */
    float ComputeFinalDamageForTarget(AActor* Victim, float RawDamage, bool& bOutCrit, float CritChance, float CritMultiplier) const;

    /* Legacy hooks */
    void BindDamageHooks();
    UFUNCTION() void HandleAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);
    UFUNCTION() void HandlePointDamage(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser);

    /* Perfect boosts */
    void ApplyPerfectBoost(EEEPerfectKind Kind);
    void RestoreBoosts();

    /* Search helpers */
    const FEEAttackSpec* FindAttack(FName AttackId) const;

    /* Combo runtime helpers */
    void AdvanceComboIfRequested();

    /* Telegraph */
    void SpawnTelegraph();
    void UpdateTelegraph(float Alpha);
    void DestroyTelegraph();

    /* Utils */
    TArray<AActor*> UniqueActorsFromHits(const TArray<FHitResult>& Hits) const;

private:
    /* Config */
    UPROPERTY(EditAnywhere, Category="Combat|Config") TArray<FEEAttackSpec> Attacks;
    UPROPERTY(EditAnywhere, Category="Combat|Config") TArray<FEEComboSpec> Combos;
    UPROPERTY(EditAnywhere, Category="Combat|Config") TEnumAsByte<ECollisionChannel> DamageTraceChannel = ECC_Pawn;
    UPROPERTY(EditAnywhere, Category="Combat|Config") bool bIgnoreOwner = true;
    UPROPERTY(EditAnywhere, Category="Combat|Magnetism", meta=(ClampMin="0.0", ClampMax="90.0")) float MagnetismAngleDeg = 35.f;
    UPROPERTY(EditAnywhere, Category="Combat|Magnetism", meta=(ClampMin="0.0", ClampMax="1.0")) float MagnetismStrength = 0.55f;
    UPROPERTY(EditAnywhere, Category="Combat|Facing", meta=(ClampMin="0.0", ClampMax="45.0")) float MaxAutoYawOnAttackDeg = 20.f;

    /* Parry */
    UPROPERTY(EditAnywhere, Category="Combat|Parry", meta=(ClampMin="0.0", ClampMax="1.0")) float ParryDamageFactorWhileHeld = 0.2f;
    UPROPERTY(EditAnywhere, Category="Combat|Parry") TArray<UAnimMontage*> ParryHitReactionMontages;
    UPROPERTY(EditAnywhere, Category="Combat|Parry", meta=(ClampMin="0.1")) float ParryMoveSpeedMultiplier = 0.6f;

    /* Dodge */
    UPROPERTY(EditAnywhere, Category="Combat|Dodge", meta=(ClampMin="0.05")) float DodgeIFrameDuration = 0.35f;

    /* Perfect boost */
    UPROPERTY(EditAnywhere, Category="Combat|Perfect", meta=(ClampMin="1.0")) float PerfectMoveSpeedMultiplier = 1.25f;
    UPROPERTY(EditAnywhere, Category="Combat|Perfect", meta=(ClampMin="1.0")) float PerfectAnimRateMultiplier = 1.25f;
    UPROPERTY(EditAnywhere, Category="Combat|Perfect", meta=(ClampMin="0.1")) float PerfectBoostDuration = 2.0f;

    /* Telegraph visuals */
    UPROPERTY(EditAnywhere, Category="Combat|Telegraph") UMaterialInterface* RadialDecalMaterial = nullptr;
    UPROPERTY(EditAnywhere, Category="Combat|Telegraph") UMaterialInterface* RectDecalMaterial = nullptr;

    /* Runtime */
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") FName CurrentAttackId = NAME_None;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bInAttackWindow = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") float CooldownEndTime = 0.f;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bParryHeld = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bPerfectParryWindow = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bPerfectDodgeWindow = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bInDodgeIFrames = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") TWeakObjectPtr<AActor> ExternalTarget;

    /* Charge runtime */
    bool bCharging = false;
    float ChargeStartTime = 0.f;
    float ChargeExpectedDuration = 0.f;
    float ChargeAccumulated = 0.f;
    int32 ChargeLevelIndex = -1;

    /* Combo runtime */
    FName ActiveComboId = NAME_None;
    int32 ActiveComboStep = -1;
    bool bComboWindowOpen = false;
    bool bComboAdvanceRequested = false;
    float ComboResetTime = 0.f;

    /* Cached */
    TWeakObjectPtr<class ACharacter> OwnerCharacter;
    TWeakObjectPtr<USkeletalMeshComponent> OwnerMesh;
    TWeakObjectPtr<UCharacterMovementComponent> MoveComp;
    TWeakObjectPtr<ULockTargetComponent> LockComp;
    float BaseWalkSpeed = -1.f;
    float BaseGlobalAnimRate = 1.f;

    /* Telegraph component */
    UDecalComponent* ActiveDecal = nullptr;
};
