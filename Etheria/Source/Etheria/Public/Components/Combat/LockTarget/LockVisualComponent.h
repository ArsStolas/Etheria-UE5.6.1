/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "LockVisualComponent" - Header
 * Notes: Declares visual feedback settings and data for lock-on, including overlay material setup, DOF configuration and cinematic lock events.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockVisualComponent.generated.h"

class ULockTargetComponent;
class UCameraComponent;
class UMeshComponent;
class UMaterialInterface;
class AActor;

/** Broadcast when cinematic lock state changes (use in UI to drive black bars, etc.). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEECinematicLockChanged, bool, bLocked);

/**
 * Backup of previous overlay materials
 */
USTRUCT()
struct FEEOverlayBackup
{
    GENERATED_BODY()

    /** Mesh that had its overlay material overridden. */
    UPROPERTY()
    TWeakObjectPtr<UMeshComponent> Mesh;

    /** Overlay material that was present before lock (can be nullptr). */
    UPROPERTY()
    UMaterialInterface* PreviousOverlay = nullptr;
};

/**
 * Component responsible for visual feedback when a target is locked:
 * - Overlay material applied on the locked target
 * - Depth of field focusing on the target while locked
 * - Blueprint event for cinematic black bars
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API ULockVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULockVisualComponent();

    /** Blueprint event: triggered when lock mode becomes active/inactive (drive black bars in UMG). */
    UPROPERTY(BlueprintAssignable, Category="LockVisual|Events")
    FEECinematicLockChanged OnCinematicLockChanged;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Bound to LockTargetComponent::OnLockChanged. */
    UFUNCTION()
    void HandleLockChanged(AActor* NewTarget);

private:
#pragma region OVERLAY
    /** Summary: Enables or disables overlay material on all mesh components of the actor. */
    void ApplyOverlayToActor(AActor* Target, bool bEnable);
#pragma endregion

#pragma region DOF
    /** Summary: Updates DOF focus and F-stop based on current target distance or restores base DOF when unlocking. */
    void UpdateDepthOfField(float DeltaTime);
#pragma endregion

#pragma region LOOKUP
    /** Summary: Tries to cache references to LockTargetComponent and CameraComponent on the owner. */
    void ResolveOwnerRefs();
#pragma endregion

#pragma region CONFIG_OVERLAY
    /** If true, apply OverlayMaterial on the currently locked target. */
    UPROPERTY(EditAnywhere, Category="LockVisual|Overlay")
    bool bUseOverlayMaterial = true;

    /** Overlay material to apply on all mesh elements of the locked target (kept optional). */
    UPROPERTY(EditAnywhere, Category="LockVisual|Overlay", meta=(EditCondition="bUseOverlayMaterial", EditConditionHides))
    UMaterialInterface* OverlayMaterial = nullptr;
#pragma endregion

#pragma region CONFIG_DOF
    /** If true, adjust DOF to focus on the current locked target. */
    UPROPERTY(EditAnywhere, Category="LockVisual|DOF")
    bool bEnableDOF = true;

    /** If true, smoothly restore original DOF values when the lock is released. */
    UPROPERTY(EditAnywhere, Category="LockVisual|DOF", meta=(EditCondition="bEnableDOF", EditConditionHides))
    bool bRestoreBaseDOFOnUnlock = true;

    /** Interp speed when moving DOF focus distance and F-stop. */
    UPROPERTY(EditAnywhere, Category="LockVisual|DOF", meta=(ClampMin="0.1", EditCondition="bEnableDOF", EditConditionHides))
    float DOFFocusInterpSpeed = 10.f;

    /** F-stop value when locked-on (smaller = stronger blur on background). */
    UPROPERTY(EditAnywhere, Category="LockVisual|DOF", meta=(EditCondition="bEnableDOF", EditConditionHides))
    float LockedFStop = 2.8f;
#pragma endregion

#pragma region CACHED
    /** Cached reference to the lock component on the same actor. */
    TWeakObjectPtr<ULockTargetComponent> LockComp;

    /** Cached reference to the camera component used for DOF calculations. */
    TWeakObjectPtr<UCameraComponent> CameraComp;

    /** Current locked target for visual effects. */
    TWeakObjectPtr<AActor> CurrentTarget;

    /** Backups of original materials for the currently highlighted target. */
    UPROPERTY()
    TArray<FEEOverlayBackup> OverlayBackups;

    /** Base DOF settings to restore when no lock is active. */
    float BaseFocalDistance = 1000.f;
    float BaseFStop         = 8.0f;
    bool  bHadFStopOverride = false;
    bool  bHadFocusOverride = false;

    /** True while we are interpolating DOF back to the base settings after unlock. */
    bool  bRestoringBaseDOF = false;
#pragma endregion
};
