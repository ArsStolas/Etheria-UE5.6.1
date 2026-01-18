/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "CombatComponent - Header"
 * Notes: Main combat component API and configuration. Implementations are split into feature .cpp files.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "CombatTypes.h"
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

    UFUNCTION(BlueprintCallable, Category="Combat")
    FORCEINLINE UWeaponData* GetCurrentWeaponData() const { return CurrentWeaponData ? CurrentWeaponData : WeaponData; }

    /** Sets a new weapon data (called by Equipment or Inventory). */
    void SetCurrentWeaponData(UWeaponData* NewWeaponData);

    /** Summary: Overrides the current attack list (usually from weapon data). */
    UFUNCTION(BlueprintCallable, Category="Combat") void SetAttacks(const TArray<FAttackSpecConfig>& InAttacks);
    /** Summary: Overrides the current combo list (usually from weapon data). */
    UFUNCTION(BlueprintCallable, Category="Combat") void SetCombos(const TArray<FComboSpecConfig>& InCombos);
    /** Summary: Attempts to execute the next valid primary attack for the current weapon. */
    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackPrimary();
    /** Summary: Attempts to execute a specific attack by id (optionally with a charge level). */
    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackById(FName AttackId, float ChargeLevel = 0.f);
    /** Summary: Attempts to execute an attack group, selecting a ground/air variant automatically. */
    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackGroup(FName GroupId); // picks ground/air variant automatically

    /** Summary: Queues a request to advance the current combo step when allowed. */
    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void RequestComboAdvance();

    /** Summary: Forces a target override (e.g., lock-on). Pass nullptr to clear. */
    UFUNCTION(BlueprintCallable, Category="Combat|Target") void SetExternalTarget(AActor* InTarget);
    /** Summary: Returns the current target (external override or best available). */
    UFUNCTION(BlueprintPure,   Category="Combat|Target") AActor* GetCurrentTarget() const;

    /** Summary: Opens the attack hit window (enables traces / hit detection). */
    UFUNCTION(BlueprintCallable, Category="Combat|Window") void BeginAttackWindow();
    /** Summary: Closes the attack hit window. */
    UFUNCTION(BlueprintCallable, Category="Combat|Window") void EndAttackWindow();
    UFUNCTION(BlueprintCallable, Category="Combat|Window") void EndHitWindow();

    /** Summary: Sets whether parry input is currently held. */
    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void SetParryHeld(bool bHeld);
    /** Summary: Returns whether parry input is held. */
    UFUNCTION(BlueprintPure,   Category="Combat|Parry") bool IsParryHeld() const { return bParryHeld; }
    /** Summary: Opens the perfect-parry timing window. */
    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void BeginPerfectParryWindow();
    /** Summary: Closes the perfect-parry timing window. */
    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void EndPerfectParryWindow();
    
    /** Summary: Starts dodge i-frames (optional duration override). */
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void StartDodgeIFrames(float DurationOverride = -1.f);
    /** Summary: Opens the perfect-dodge timing window. */
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void BeginPerfectDodgeWindow();
    /** Summary: Closes the perfect-dodge timing window. */
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void EndPerfectDodgeWindow();
    /** Summary: Handles a dodge tap (sprint double-tap) using a world-space movement direction. */
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void HandleDodgeInputTap(const FVector& WorldDirection);
    /** Summary: Tries to start a directional dodge using a world-space direction (player / AI). */
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") bool TryDodgeWorldDirection(const FVector& WorldDirection);
    /** Summary: Tries to start a directional dodge for a given enum direction. */
    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") bool TryDodgeDirection(EDodgeDirection DodgeDirection);
    UFUNCTION(BlueprintPure,   Category="Combat|Dodge") bool IsInIFrames() const { return bInDodgeIFrames; }

    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void BeginCharge(FName AttackId, float ExpectedDuration);
    /** Summary: Updates charge progress (called every tick while charging). */
    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void UpdateChargeProgress(float DeltaTime);
    /** Summary: Ends charging and optionally fires / executes the charged attack. */
    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void EndCharge(bool bCanceled);

    /** Summary: Opens the combo input window for a combo id. */
    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void BeginComboWindow(FName ComboId);
    /** Summary: Closes the combo input window for a combo id. */
    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void EndComboWindow(FName ComboId);
    /** Summary: Returns remaining cooldown time for a given combo id. */
    UFUNCTION(BlueprintPure, Category="Combat|Combo") float GetComboCooldownRemaining(FName ComboId) const;
    /** Summary: Clears all combo cooldowns (debug / reset). */
    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void ClearAllComboCooldowns();

    /** If true, use these values instead of those from WeaponData->Ranged */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Ranged|Camera") bool bUseCameraOverrides = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Ranged|Camera", meta=(EditCondition="bUseCameraOverrides")) float AimArmLengthOverride = 220.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Ranged|Camera", meta=(EditCondition="bUseCameraOverrides")) float AimFOVOverride = 70.f;

    // Weapon data hot-swap
    UFUNCTION(BlueprintCallable, Category="Combat|Weapon") void SetWeaponData(UWeaponData* InData);
    /** Summary: Applies the currently active weapon data to runtime attack/combo lists. */
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

    /** Summary: Returns true if attacks are globally blocked by cooldown. */
    UFUNCTION(BlueprintPure, Category="Combat|State") bool IsInCooldown() const;
    /** Summary: Returns true while the attack hit window is open. */
    UFUNCTION(BlueprintPure, Category="Combat|State") bool IsInAttackWindow() const { return bInAttackWindow; }
    /** Summary: Returns true if an attack is currently active. */
    UFUNCTION(BlueprintPure, Category="Combat|State") bool IsAttackActive() const { return CurrentAttackId != NAME_None; }

    void PushRecentHitActor(AActor* A);
    void ClearRecentHitActors();
    void GetRecentHitActors(TArray<AActor*>& Out) const;

    /** Summary: Returns true if combat state currently blocks jumping. */
    UFUNCTION(BlueprintPure, Category="Combat|Input") bool IsJumpBlocked() const;
    /** Summary: Returns true if combat state currently blocks crouching. */
    UFUNCTION(BlueprintPure, Category="Combat|Input") bool IsCrouchBlocked() const;
    /** Summary: Pushes an input lock layer that can block actions like jump/crouch. */
    UFUNCTION(BlueprintCallable, Category="Combat|Input") void PushInputLock(FName LockId, bool bBlockJump, bool bBlockCrouch);
    /** Summary: Pops a previously pushed input lock layer by id. */
    UFUNCTION(BlueprintCallable, Category="Combat|Input") void PopInputLock(FName LockId);

    // Force a fallback AnimBP (with a valid Slot) while playing montages
    UPROPERTY(EditAnywhere, Category="Combat|Montage") bool bForceFallbackAnimBPForMontages = true;
    UPROPERTY(EditAnywhere, Category="Combat|Montage") TSubclassOf<class UAnimInstance> FallbackMontageAnimClass;

    // Runtime bookkeeping
    TSubclassOf<class UAnimInstance> SavedAnimClass = nullptr;
    bool bUsingFallbackAnimClass = false;

    // Internal hooks
    UFUNCTION()
    void HandleMontageEnded_RestoreAnimClass(class UAnimMontage* Montage, bool bInterrupted);

    // Called before any Montage_Play / JumpToSection to ensure a valid Slot
    void PrePlayMontageSafety(const FAttackSpecConfig& Spec);

#pragma region RANGED COMBAT

    /** Summary: Returns desired camera arm length and FOV while aiming. */
    UFUNCTION(BlueprintPure, Category="Combat|Ranged|Camera")
    void GetAimCameraParams(float& OutArmLength, float& OutFOV) const;

    /** Start and stop auto fire depending on weapon type. */
    void StartRangedFire();
    void StopRangedFire();

    /**
     * Applies damage for a ranged projectile impact (used by arrow/projectile actors).
     * AttackId is used to resolve damage/crit settings from the AttackSpec.
     */
    UFUNCTION(BlueprintCallable, Category="Combat|Ranged")
    void HandleRangedProjectileImpact(AActor* HitActor, const FHitResult& Hit, FName AttackId, float ChargeAlpha, float DamageScale = 1.f);

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

#pragma region INTERNAL EXECUTION
    bool ResolveOwnerRefs();
    bool CanExecuteAttack(const FName AttackId) const;
    void ExecuteAttack(const FAttackSpecConfig& Spec, float DamageScale = 1.f, float RangeScale = 1.f);
    void OpenWindowWithTimers(const FAttackSpecConfig& Spec);
    void CloseCurrentAttack();
    void PerformMeleeTrace(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale);
    void PerformAoE(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale);
    void PerformFrontalRect(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale);
#pragma endregion

#pragma region FACING AND TARGET ASSIST
    void NudgeOwnerRotationToward(const FVector& Direction, float MaxYawDeltaDeg) const;
    FVector GetEyeLocationForward(FVector& OutForward) const;
    AActor* ResolveBestTarget(const FVector& EyeLoc, const FVector& Forward) const;
    FVector ApplyMagnetismBias(const FVector& RawForward, const FVector& EyeLoc) const;
#pragma endregion

#pragma region DAMAGE AND DEFENSE
    float ComputeFinalDamageForTarget(AActor* Victim, float RawDamage, bool& bOutCrit, float CritChance, float CritMultiplier) const;
#pragma endregion

#pragma region PERFECT BOOSTS
    UPROPERTY(EditAnywhere, Category="Combat|Perfect", meta=(ClampMin="1.0")) float PerfectMoveSpeedMultiplier = 1.25f;
    UPROPERTY(EditAnywhere, Category="Combat|Perfect", meta=(ClampMin="1.0")) float PerfectAnimRateMultiplier = 1.25f;
    UPROPERTY(EditAnywhere, Category="Combat|Perfect", meta=(ClampMin="0.1")) float PerfectBoostDuration = 2.0f;

    void ApplyPerfectBoost(EPerfectKind Kind);
    void RestoreBoosts();

    bool CanStartDodge() const;
    UAnimMontage* GetDodgeMontage(EDodgeDirection Direction) const;
#pragma endregion

#pragma region SEARCH HELPERS
    const FAttackSpecConfig* FindAttack(FName AttackId) const;
#pragma endregion

#pragma region COMBO RUNTIME HELPERS
    FName ActiveComboId = NAME_None;
    int32 ActiveComboStep = -1;
    bool bComboWindowOpen = false;
    bool bComboAdvanceRequested = false;
    float ComboResetTime = 0.f;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") TMap<FName, float> ComboCooldownUntil;

    void AdvanceComboIfRequested();
#pragma endregion

#pragma region RANGED
    UPROPERTY(EditAnywhere, Category="Combat|Ranged") ERangedMode RangedMode = ERangedMode::Line;
    UPROPERTY(EditAnywhere, Category="Combat|Ranged") FName MuzzleSocketName = "Muzzle";
#pragma endregion

#pragma region PARRY
    UPROPERTY(EditAnywhere, Category="Combat|Parry", meta=(ClampMin="0.0", ClampMax="1.0")) float ParryDamageFactorWhileHeld = 0.2f;
    UPROPERTY(EditAnywhere, Category="Combat|Parry") TArray<UAnimMontage*> ParryHitReactionMontages;
    UPROPERTY(EditAnywhere, Category="Combat|Parry", meta=(ClampMin="0.1")) float ParryMoveSpeedMultiplier = 0.6f;
#pragma endregion

#pragma region DODGE
    /** Base dodge i-frame duration if no override is provided. */
    UPROPERTY(EditAnywhere, Category="Combat|Dodge", meta=(ClampMin="0.05")) float DodgeIFrameDuration = 0.35f;

    /** Maximum delay between two sprint taps to trigger a dodge (Shift double-tap). */
    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Input", meta=(ClampMin="0.05")) float DodgeDoubleTapMaxDelay = 0.30f;

    /** Minimum input magnitude required to consider the movement direction valid. */
    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Input", meta=(ClampMin="0.0", ClampMax="1.0")) float DodgeMinInputThreshold = 0.25f;

    /** Directional dodge montages. */
    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Animation") UAnimMontage* DodgeForwardMontage = nullptr;
    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Animation") UAnimMontage* DodgeBackwardMontage = nullptr;
    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Animation") UAnimMontage* DodgeLeftMontage = nullptr;
    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Animation") UAnimMontage* DodgeRightMontage = nullptr;
#pragma endregion

#pragma region TELEGRAPH
    /** Material used for radial telegraph decals (circular AoE). */
    UPROPERTY(EditAnywhere, Category="Combat|Telegraph") UMaterialInterface* RadialDecalMaterial = nullptr;
    
    /** Material used for frontal telegraph decals (rectangular AoE). */
    UPROPERTY(EditAnywhere, Category="Combat|Telegraph") UMaterialInterface* RectDecalMaterial = nullptr;
    
    /** Thickness of the decal box along the projection direction (X). */
    UPROPERTY(EditAnywhere, Category="Combat|Telegraph", meta=(ClampMin="1.0")) float TelegraphDecalThickness = 256.f;

    /** Vertical offset so the telegraph is projected onto the ground instead of mid-air. */
    UPROPERTY(EditAnywhere, Category="Combat|Telegraph") float TelegraphHeightOffset = 80.f;

    /** Spawns the decal component for the current charged attack, if any. */
    void SpawnTelegraph();

    /** Updates decal size and position based on current charge alpha. */
    void UpdateTelegraph(float Alpha);

    /** Destroys the active telegraph decal, if present. */
    void DestroyTelegraph();

    /** Runtime decal component currently used for telegraph visuals. */
    UDecalComponent* ActiveDecal = nullptr;
#pragma endregion

#pragma region RUNTIME
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") FName CurrentAttackId = NAME_None;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") TObjectPtr<UAnimMontage> CurrentAttackMontage = nullptr;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") FName LastAttackId = NAME_None;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bInAttackWindow = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") float CooldownEndTime = 0.f;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bParryHeld = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bPerfectParryWindow = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bPerfectDodgeWindow = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool bInDodgeIFrames = false;

    /** Dodge double-tap detection runtime state. */
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") bool  bDodgeTapPending = false;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") float LastDodgeTapTime = 0.f;
    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") FVector LastDodgeDirection = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, Category="Combat|Runtime") TWeakObjectPtr<AActor> ExternalTarget;
    TArray<TWeakObjectPtr<AActor>> RecentHitActors;
    TSet<FName> JumpLocks;
    TSet<FName> CrouchLocks;
#pragma endregion

#pragma region CHARGE RUNTIME
    bool  bCharging = false;
    float ChargeStartTime = 0.f;
    float ChargeExpectedDuration = 0.f;
    float ChargeAccumulated = 0.f;
    int32 ChargeLevelIndex = -1;
    int32 ObservedChargeLevel = -1;

    /** Cached walk speed while charging, restored in EndCharge. */
    float SavedChargeMoveSpeed = -1.f;

    /** True if we disabled movement mode instead of just scaling walk speed. */
    bool bChargeDisabledMovement = false;

    /** Applies movement lock / slowdown while charging. */
    void ApplyChargeMovementLock();

    /** Restores movement settings after charge ends. */
    void RestoreChargeMovementLock();
#pragma endregion

#pragma region CACHED
    TWeakObjectPtr<ABaseCharacter> OwnerCharacter;
    TWeakObjectPtr<USkeletalMeshComponent> OwnerMesh;
    TWeakObjectPtr<UCharacterMovementComponent> MoveComp;
    TWeakObjectPtr<ULockTargetComponent> LockComp;
    TWeakObjectPtr<UCharacterStateComponent> StateComp;
    float BaseWalkSpeed = -1.f;
    float BaseGlobalAnimRate = 1.f;
#pragma endregion

#pragma region CONFIG
    UPROPERTY(EditAnywhere, Category="Combat|Config") TArray<FAttackSpecConfig> Attacks;
    UPROPERTY(EditAnywhere, Category="Combat|Config") TArray<FComboSpecConfig> Combos;
    UPROPERTY(EditAnywhere, Category="Combat|Config") TEnumAsByte<ECollisionChannel> DamageTraceChannel = ECC_Pawn;
    UPROPERTY(EditAnywhere, Category="Combat|Config") bool bIgnoreOwner = true;
    UPROPERTY(EditAnywhere, Category="Combat|Magnetism", meta=(ClampMin="0.0", ClampMax="90.0")) float MagnetismAngleDeg = 35.f;
    UPROPERTY(EditAnywhere, Category="Combat|Magnetism", meta=(ClampMin="0.0", ClampMax="1.0")) float MagnetismStrength = 0.55f;
    UPROPERTY(EditAnywhere, Category="Combat|Facing", meta=(ClampMin="0.0", ClampMax="45.0")) float MaxAutoYawOnAttackDeg = 20.f;
    UPROPERTY(EditAnywhere, Category="Combat|Direction") bool bUseMeshFacing = true;

    /** If true, movement will be limited/blocked while charging a manual charge attack. */
    UPROPERTY(EditAnywhere, Category="Combat|Charge") bool bBlockMovementDuringCharge = true;

    /**
     * Multiplier applied to walk speed while charging.
     * 0.0 = fully locked in place, 1.0 = normal speed, 0.2 = heavy slow.
     */
    UPROPERTY(EditAnywhere, Category="Combat|Charge", meta=(ClampMin="0.0", ClampMax="1.0")) float ChargeMoveSpeedMultiplier = 0.0f;
#pragma endregion
    
#pragma region DEBUG
    UPROPERTY(EditAnywhere, Category="Combat|Debug") bool bDebugDraw = false;
#pragma endregion

#pragma region UTILS
    TArray<AActor*> UniqueActorsFromHits(const TArray<FHitResult>& Hits) const;
    void PlayOrJumpMontageSection(const FAttackSpecConfig& Spec);
    void PerformRangedLine(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale);
#pragma endregion


};