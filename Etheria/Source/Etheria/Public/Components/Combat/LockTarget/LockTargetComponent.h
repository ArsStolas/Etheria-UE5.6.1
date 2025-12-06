/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "LockTargetComponent" - Header
 * Notes: Declares the lock-on component API, configuration and runtime state used for selecting, maintaining and broadcasting the current lock target.
 */
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockTargetComponent.generated.h"

class AActor;

/** Broadcast whenever the locked target changes (including when cleared with nullptr). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEELockChanged, AActor*, NewTarget);

/**
 * Component in charge of selecting and maintaining a lock-on target.
 * Uses the owner's view (camera / controller) as primary reference and can optionally
 * align camera + movement while a lock is active.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API ULockTargetComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULockTargetComponent();

    /** Summary: Toggles lock on/off. When enabling, picks the best target in front of the view (camera / controller). */
    UFUNCTION(BlueprintCallable, Category="Lock")
    bool ToggleLock(AActor* Preferred = nullptr);

    /** Summary: Clears the current lock and notifies listeners. */
    UFUNCTION(BlueprintCallable, Category="Lock")
    void ClearLock();

    /** Summary: Switches to the best target on the left/right of the current view, relative to the current lock. */
    UFUNCTION(BlueprintCallable, Category="Lock")
    bool SwitchTarget(bool bRight);

    /** Summary: Returns the currently locked actor (if any). */
    UFUNCTION(BlueprintPure, Category="Lock")
    AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

    /** Broadcast whenever the locked target changes (including when cleared with nullptr). */
    UPROPERTY(BlueprintAssignable, Category="Lock")
    FEELockChanged OnLockChanged;

protected:
    virtual void BeginPlay() override;

    /** Summary: Keeps the current lock updated each frame (auto-switch / clear) and handles camera / movement when enabled. */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
#pragma region TARGET_QUERY
    /** Summary: Finds the best candidate to lock onto, using the current view direction. */
    AActor* FindBestTarget(AActor* Preferred) const;

    /** Summary: Gathers all potential lock targets within distance + angle constraints of the current view. */
    TArray<AActor*> GatherCandidates() const;

    /** Summary: Scores a candidate based on alignment with the view and distance (higher = better). */
    float ScoreCandidate(AActor* Candidate, const FVector& EyeLoc, const FVector& Forward) const;
#pragma endregion

#pragma region RUNTIME_UPDATES
    /** Summary: Ensures the current target is still valid (distance, visibility, lifetime) and clears lock if not. */
    void ValidateCurrentTarget(const FVector& EyeLoc);

    /** Summary: Maintains the current lock, optionally auto-switching or clearing based on camera offset. */
    void UpdateLockMaintenance(const FVector& EyeLoc, const FVector& Forward);

    /** Summary: Aligns camera / controller and movement to the current lock target when enabled. */
    void UpdateCameraAndMovement(const FVector& EyeLoc, float DeltaTime);

    /** Summary: Applies or restores movement settings (use controller yaw vs orient to movement) when locking / unlocking. */
    void ApplyMovementSettingsForLock(bool bLocked);

    /** Summary: Draws debug shapes for lock direction / cone / target. */
    void DebugDrawLock(const FVector& EyeLoc, const FVector& Forward) const;

    /** Summary: Computes the current view origin and forward vector (camera / controller based when possible). */
    void GetViewLocationAndForward(FVector& OutLocation, FVector& OutForward) const;
#pragma endregion

#pragma region CONFIG_CORE
    /** Maximum distance at which lock targets can be acquired. */
    UPROPERTY(EditAnywhere, Category="Lock|Config", meta=(ClampMin="100.0"))
    float MaxDistance = 2500.f;

    /** Maximum view cone angle (degrees) in which a target can be considered for lock. */
    UPROPERTY(EditAnywhere, Category="Lock|Config", meta=(ClampMin="0.0", ClampMax="90.0"))
    float MaxAngleDeg = 55.f;

    /** Collision channel used to gather potential lock targets. */
    UPROPERTY(EditAnywhere, Category="Lock|Config")
    TEnumAsByte<ECollisionChannel> TargetChannel = ECC_Pawn;

    /** If true, only Pawns with the given HostileTagName will be considered lockable. */
    UPROPERTY(EditAnywhere, Category="Lock|Config")
    bool bRequireHostileTag = true;

    /** Actor tag required on Pawn to be considered a hostile lock target (e.g. "Hostile"). */
    UPROPERTY(EditAnywhere, Category="Lock|Config", meta=(EditCondition="bRequireHostileTag", EditConditionHides))
    FName HostileTagName = FName("Hostile");
    
    /** If true, the current lock will be automatically cleared when the target is farther than MaxDistance. */
    UPROPERTY(EditAnywhere, Category="Lock|Config")
    bool bAutoUnlockOnDistance = true;

    /** If true, the current lock will be automatically cleared when the target becomes invalid or destroyed. */
    UPROPERTY(EditAnywhere, Category="Lock|Config")
    bool bAutoUnlockOnInvalidTarget = true;

    /** If true, the current lock will be automatically cleared when the target becomes hidden in game. */
    UPROPERTY(EditAnywhere, Category="Lock|Config")
    bool bAutoUnlockOnHiddenTarget = true;
#pragma endregion

#pragma region CONFIG_CAMERA
    /** If true, camera/controller will be softly aligned toward the locked target. */
    UPROPERTY(EditAnywhere, Category="Lock|Camera")
    bool bEnableCameraLock = true;

    /** If true, pitch will also be aligned toward the target (otherwise only yaw is locked). */
    UPROPERTY(EditAnywhere, Category="Lock|Camera", meta=(EditCondition="bEnableCameraLock", EditConditionHides))
    bool bLockPitch = false;

    /** If true, clamp pitch between MinPitchDeg and MaxPitchDeg when locking pitch. */
    UPROPERTY(EditAnywhere, Category="Lock|Camera", meta=(EditCondition="bEnableCameraLock && bLockPitch", EditConditionHides))
    bool bClampPitch = true;

    /** Minimum pitch angle when clamping (degrees). */
    UPROPERTY(EditAnywhere, Category="Lock|Camera", meta=(EditCondition="bEnableCameraLock && bLockPitch && bClampPitch", EditConditionHides))
    float MinPitchDeg = -40.f;

    /** Maximum pitch angle when clamping (degrees). */
    UPROPERTY(EditAnywhere, Category="Lock|Camera", meta=(EditCondition="bEnableCameraLock && bLockPitch && bClampPitch", EditConditionHides))
    float MaxPitchDeg = 40.f;

    /** Vertical offset applied to the target location when computing the look-at direction (focus on chest/head instead of feet). */
    UPROPERTY(EditAnywhere, Category="Lock|Camera", meta=(EditCondition="bEnableCameraLock", EditConditionHides))
    float TargetHeightOffset = 80.f;

    /** Interp speed for yaw alignment of the camera/controller toward the target. */
    UPROPERTY(EditAnywhere, Category="Lock|Camera", meta=(ClampMin="0.1", EditCondition="bEnableCameraLock", EditConditionHides))
    float CameraYawInterpSpeed = 8.f;

    /** Interp speed for pitch alignment of the camera/controller toward the target. */
    UPROPERTY(EditAnywhere, Category="Lock|Camera", meta=(ClampMin="0.1", EditCondition="bEnableCameraLock && bLockPitch", EditConditionHides))
    float CameraPitchInterpSpeed = 6.f;
#pragma endregion

#pragma region CONFIG_SOFTLOCK
    /** Within this angle (degrees), the current target is kept and the camera can offset freely. */
    UPROPERTY(EditAnywhere, Category="Lock|SoftLock", meta=(ClampMin="0.0", ClampMax="90.0"))
    float MaintainLockAngleDeg = 20.f;

    /** When the angle between view and current target exceeds this, we can try to auto-switch to a better target. */
    UPROPERTY(EditAnywhere, Category="Lock|SoftLock", meta=(ClampMin="0.0", ClampMax="90.0"))
    float SwitchLockAngleDeg = 35.f;

    /** If true, soft lock logic will try to keep the current target while the camera stays within the allowed cone. */
    UPROPERTY(EditAnywhere, Category="Lock|SoftLock")
    bool bEnableSoftLock = true;

    /** If true, soft lock can automatically switch to a better target when the camera is significantly offset. */
    UPROPERTY(EditAnywhere, Category="Lock|SoftLock", meta=(EditCondition="bEnableSoftLock", EditConditionHides))
    bool bEnableAutoSwitch = true;
#pragma endregion

#pragma region CONFIG_MOVEMENT_DEBUG
    /** If true, toggles character movement mode to a lock-on style (faces target, strafes around). */
    UPROPERTY(EditAnywhere, Category="Lock|Movement")
    bool bUseLockMovementMode = true;

    /** Debug: if true, draws view direction and current target marker. */
    UPROPERTY(EditAnywhere, Category="Lock|Debug")
    bool bDebugDrawLock = false;
#pragma endregion

#pragma region RUNTIME_STATE
    /** Current target being locked, if any. */
    TWeakObjectPtr<AActor> CurrentTarget;
#pragma endregion
};
