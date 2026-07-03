/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: ArsStolas
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
class UMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

	bool bComboAdvanceRequested = false;
	UFUNCTION(BlueprintCallable, Category="Combat|Combo")
	void RequestComboAdvanceAI();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponDissolveRequest);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    const TArray<FAttackSpecConfig>& GetAttacks() const { return Attacks; }
    const TArray<FComboSpecConfig>& GetCombos() const { return Combos; }

    UFUNCTION(BlueprintCallable, Category="Combat")
    float GetCurrentAttackRange() const;

    UPROPERTY() UWeaponData* CurrentWeaponData = nullptr;

    UFUNCTION(BlueprintCallable, Category="Combat")
    FORCEINLINE UWeaponData* GetCurrentWeaponData() const { return CurrentWeaponData ? CurrentWeaponData : WeaponData; }

    void SetCurrentWeaponData(UWeaponData* NewWeaponData);

    UFUNCTION(BlueprintCallable, Category="Combat") void SetAttacks(const TArray<FAttackSpecConfig>& InAttacks);

    UFUNCTION(BlueprintCallable, Category="Combat") void SetCombos(const TArray<FComboSpecConfig>& InCombos);

    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackPrimary();

    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackById(FName AttackId, float ChargeLevel = 0.f);

    UFUNCTION(BlueprintCallable, Category="Combat") bool TryAttackGroup(FName GroupId);

    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void RequestComboAdvance();

    UFUNCTION(BlueprintCallable, Category="Combat|Target") void SetExternalTarget(AActor* InTarget);

    UFUNCTION(BlueprintPure,   Category="Combat|Target") AActor* GetCurrentTarget() const;

    UFUNCTION(BlueprintCallable, Category="Combat|Window") void BeginAttackWindow();

    UFUNCTION(BlueprintCallable, Category="Combat|Window") void EndAttackWindow();
    UFUNCTION(BlueprintCallable, Category="Combat|Window") void EndHitWindow();

    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void SetParryHeld(bool bHeld);

    UFUNCTION(BlueprintPure,   Category="Combat|Parry") bool IsParryHeld() const { return bParryHeld; }

    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void BeginPerfectParryWindow();

    UFUNCTION(BlueprintCallable, Category="Combat|Parry") void EndPerfectParryWindow();

    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void StartDodgeIFrames(float DurationOverride = -1.f);

    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void BeginPerfectDodgeWindow();

    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void EndPerfectDodgeWindow();

    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") void HandleDodgeInputTap(const FVector& WorldDirection);

    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") bool TryDodgeWorldDirection(const FVector& WorldDirection);

    UFUNCTION(BlueprintCallable, Category="Combat|Dodge") bool TryDodgeDirection(EDodgeDirection DodgeDirection);
    UFUNCTION(BlueprintPure,   Category="Combat|Dodge") bool IsInIFrames() const { return bInDodgeIFrames; }

    UFUNCTION(BlueprintCallable, Category="Combat|Defense")
    float MitigateIncomingDamage(float RawDamage, AActor* Attacker, bool& bOutFullyNegated);

    UFUNCTION(BlueprintPure, Category="Combat|Charge") bool IsMeleeCharging() const { return bCharging; }

    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void BeginCharge(FName AttackId, float ExpectedDuration);

    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void UpdateChargeProgress(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category="Combat|Charge") void EndCharge(bool bCanceled);

    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void BeginComboWindow(FName ComboId);

    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void EndComboWindow(FName ComboId);

    UFUNCTION(BlueprintPure, Category="Combat|Combo") float GetComboCooldownRemaining(FName ComboId) const;

    UFUNCTION(BlueprintCallable, Category="Combat|Combo") void ClearAllComboCooldowns();

    UFUNCTION(BlueprintCallable, Category="Combat|Weapon") void SetWeaponData(UWeaponData* InData);

    UFUNCTION(BlueprintCallable, Category="Combat|Weapon") void ApplyWeaponData();

#pragma region WEAPON DISSOLVE

	UFUNCTION(BlueprintCallable, Category="Weapon|Dissolve")
	void WeaponDissolve_PingActivity();

	UFUNCTION(BlueprintCallable, Category="Weapon|Dissolve")
	void WeaponDissolve_PushHold();

	UFUNCTION(BlueprintCallable, Category="Weapon|Dissolve")
	void WeaponDissolve_PopHold();

	UFUNCTION(BlueprintCallable, Category="Weapon|Dissolve")
	void WeaponDissolve_ApplyParams(float Dissolve, float ColorOpacity, float StrengthVN);

	UFUNCTION(BlueprintCallable, Category="Weapon|Dissolve")
	void WeaponDissolve_SetMeshesHidden(bool bHidden);

	UFUNCTION(BlueprintCallable, Category="Weapon|Dissolve")
	void WeaponDissolve_RefreshCaches();

	UPROPERTY(BlueprintAssignable, Category="Weapon|Dissolve|Events")
	FOnWeaponDissolveRequest OnWeaponShowRequested;

	UPROPERTY(BlueprintAssignable, Category="Weapon|Dissolve|Events")
	FOnWeaponDissolveRequest OnWeaponHideRequested;

#pragma endregion

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Weapon") UWeaponData* WeaponData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Ranged|Camera") bool bUseCameraOverrides = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Ranged|Camera", meta=(EditCondition="bUseCameraOverrides")) float AimArmLengthOverride = 220.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Ranged|Camera", meta=(EditCondition="bUseCameraOverrides")) float AimFOVOverride = 70.f;

    UPROPERTY(EditAnywhere, Category="Combat|Combo", meta=(ClampMin="0.0")) float MaxComboBufferTime = 1.0f;
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

    UPROPERTY(EditAnywhere, Category="Combat|Montage") bool bForceFallbackAnimBPForMontages = true;
    UPROPERTY(EditAnywhere, Category="Combat|Montage") TSubclassOf<class UAnimInstance> FallbackMontageAnimClass;

    TSubclassOf<class UAnimInstance> SavedAnimClass = nullptr;
    bool bUsingFallbackAnimClass = false;

    UFUNCTION()
    void HandleMontageEnded_RestoreAnimClass(class UAnimMontage* Montage, bool bInterrupted);

    void PrePlayMontageSafety(const FAttackSpecConfig& Spec);

#pragma region RANGED COMBAT

    UFUNCTION(BlueprintPure, Category="Combat|Ranged|Camera")
    void GetAimCameraParams(float& OutArmLength, float& OutFOV) const;

    void StartRangedFire();
    void StopRangedFire();

    UFUNCTION(BlueprintCallable, Category="Combat|Ranged")
    void HandleRangedProjectileImpact(AActor* HitActor, const FHitResult& Hit, FName AttackId, float ChargeAlpha, float DamageScale = 1.f);

#pragma endregion

protected:
    virtual void BeginPlay() override;

#pragma region RANGED COMBAT

    FTimerHandle AutoFireHandle;

    void PerformRangedFire();

    void UpdateCharge(float DeltaTime);

    float CurrentChargeLevel = 0.f;

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

    UPROPERTY(EditAnywhere, Category="Combat|Parry", meta=(ClampMin="0.0", ClampMax="0.5")) float AutoPerfectParryWindow = 0.18f;

    UPROPERTY(EditAnywhere, Category="Combat|Dodge", meta=(ClampMin="0.0", ClampMax="0.5")) float AutoPerfectDodgeWindow = 0.15f;

    FTimerHandle AutoPerfectParryHandle;
    FTimerHandle AutoPerfectDodgeHandle;
    FTimerHandle PerfectBoostRestoreHandle;
    float LastGuardReactTime = -1000.f;
#pragma endregion

#pragma region DODGE

    UPROPERTY(EditAnywhere, Category="Combat|Dodge", meta=(ClampMin="0.05")) float DodgeIFrameDuration = 0.35f;

    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Input", meta=(ClampMin="0.05")) float DodgeDoubleTapMaxDelay = 0.30f;

    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Input", meta=(ClampMin="0.0", ClampMax="1.0")) float DodgeMinInputThreshold = 0.25f;

    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Animation") UAnimMontage* DodgeForwardMontage = nullptr;
    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Animation") UAnimMontage* DodgeBackwardMontage = nullptr;
    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Animation") UAnimMontage* DodgeLeftMontage = nullptr;
    UPROPERTY(EditAnywhere, Category="Combat|Dodge|Animation") UAnimMontage* DodgeRightMontage = nullptr;
#pragma endregion

#pragma region TELEGRAPH

    UPROPERTY(EditAnywhere, Category="Combat|Telegraph") UMaterialInterface* RadialDecalMaterial = nullptr;

    UPROPERTY(EditAnywhere, Category="Combat|Telegraph") UMaterialInterface* RectDecalMaterial = nullptr;

    UPROPERTY(EditAnywhere, Category="Combat|Telegraph", meta=(ClampMin="1.0")) float TelegraphDecalThickness = 256.f;

    UPROPERTY(EditAnywhere, Category="Combat|Telegraph") float TelegraphHeightOffset = 80.f;

    void SpawnTelegraph();

    void UpdateTelegraph(float Alpha);

    void DestroyTelegraph();

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

    float SavedChargeMoveSpeed = -1.f;

    bool bChargeDisabledMovement = false;

    void ApplyChargeMovementLock();

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

#pragma region WEAPON DISSOLVE

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve")
	TArray<FComponentReference> WeaponMeshReferences;

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve")
	bool bWeaponAutoCollectByTag = true;

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve", meta=(EditCondition="bWeaponAutoCollectByTag"))
	FName WeaponAutoCollectTag = TEXT("Weapon");

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve")
	FName DissolveParamName = TEXT("Dissolve");

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve")
	FName ColorOpacityParamName = TEXT("Color Opacity");

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve")
	FName StrengthVNParamName = TEXT("Strength VN");

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve", meta=(ClampMin="0.0"))
	float WeaponAutoHideDelay = 1.25f;

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve")
	bool bWeaponStartHidden = true;

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve")
	float HiddenDissolve = 1.0f;

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve")
	float HiddenColorOpacity = 0.0f;

	UPROPERTY(EditAnywhere, Category="Weapon|Dissolve")
	float HiddenStrengthVN = 0.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> WeaponMeshesCached;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> WeaponMIDsCached;

	FTimerHandle WeaponAutoHideTimer;

	int32 WeaponVisibilityHoldCount = 0;
	bool bWeaponRequestedVisible = false;

	void WeaponDissolve_RequestShow_Internal();
	void WeaponDissolve_RequestHide_Internal();
	void WeaponDissolve_OnAutoHideTimer();

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

    UPROPERTY(EditAnywhere, Category="Combat|Charge") bool bBlockMovementDuringCharge = true;

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
