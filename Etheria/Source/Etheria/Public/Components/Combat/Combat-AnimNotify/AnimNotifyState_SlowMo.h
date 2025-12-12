/**
 * Etheria's End Project, 2025
 * Created by: 0nnen
 * Last Updated by: 0nnen
 * Class: "AnimNotifyState_SlowMo" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_SlowMo.generated.h"

class UCurveFloat;

/**
 * Scope of the slow-motion effect.
 * - Global: uses global time dilation (bullet time for the whole world).
 * - OwnerActor: only slows the owning actor via CustomTimeDilation.
 */
UENUM(BlueprintType)
enum class ESlowMoScope : uint8
{
    Global      UMETA(DisplayName = "Global Time Dilation"),
    OwnerActor  UMETA(DisplayName = "Owner Actor Only"),
};

/**
 * Dynamic slow-motion AnimNotifyState.
 *
 * - Smoothly blends IN to a target time dilation at the beginning of the notify.
 * - (Optionally) holds the target value for the middle of the notify.
 * - Smoothly blends OUT back to the original value at the end of the notify.
 *
 * Can work globally or only on the owning actor, and supports an optional curve
 * for more advanced easing (ease-in/out, custom shapes, etc.).
 */
UCLASS()
class ETHERIA_API UAnimNotifyState_SlowMo : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UAnimNotifyState_SlowMo();

    /** Called when the notify state begins. */
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    
    /** Called every tick while the notify state is active. */
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

    /** Called when the notify state ends (notify window finished or montage stopped). */
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:

    /** Where to apply the slow-motion effect. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SlowMo")
    ESlowMoScope scope = ESlowMoScope::Global;

    /**
     * Target time dilation while in slow motion.
     * 1.0 = normal speed, 0.1 = very slow.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SlowMo", meta = (ClampMin = "0.01", ClampMax = "2.0"))
    float targetTimeDilation = 0.2f;

    /**
     * Duration (in seconds) to blend from the current value to targetTimeDilation
     * at the beginning of the notify.
     * 0 = instant.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SlowMo", meta = (ClampMin = "0.0"))
    float blendInTime = 0.2f;

    /**
     * Duration (in seconds) to blend back from targetTimeDilation to the restore value
     * at the end of the notify.
     * 0 = instant.
     * If 0, no smooth out will be performed inside the notify's lifetime, only a hard reset in NotifyEnd.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SlowMo", meta = (ClampMin = "0.0"))
    float blendOutTime = 0.3f;

    /**
     * Optional float curve used for the blend alpha (0..1).
     * X = normalized blend alpha, Y = remapped alpha.
     * If nullptr, linear interpolation is used.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SlowMo")
    UCurveFloat* blendCurve = nullptr;

    /**
     * If true, the time dilation will stay at targetTimeDilation between the end of the blend-in
     * and the start of the blend-out.
     * If false, it will lerp back to the original value immediately after the blend-in.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SlowMo")
    bool bHoldAtTarget = true;

    /**
     * If true, NotifyEnd will restore a "normal" value (captured at begin or overridden below).
     * If false, the current dilation at the end of the notify is kept as-is.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SlowMo")
    bool bRestoreOnEnd = true;

    /**
     * If > 0, this value will be used as the restore value instead of the original captured one.
     * Example: you already have an existing slow-mo system at 0.5 and want to return to 0.5
     * instead of "whatever it was when the notify started".
     * <= 0 means "use captured original value".
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SlowMo", meta = (ClampMin = "0.0"))
    float overrideRestoreDilation = 0.0f;

    /**
     * Optional debug flag to print logs when the notify starts/updates/ends.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SlowMo|Debug")
    bool bPrintDebug = false;

private:

    /** Total duration of the notify during this play (seconds). */
    float runtimeTotalDuration = 0.0f;

    /** Accumulated time since the notify started (seconds). */
    float runtimeElapsed = 0.0f;

    /** Value captured at notify begin (global or actor-based depending on scope). */
    float runtimeOriginalDilation = 1.0f;

    /** Target value we effectively use for this run (clamped, etc.). */
    float runtimeTargetDilation = 1.0f;

    /** True once we successfully captured the original value. */
    bool bHasCapturedOriginal = false;

    /** Read current dilation based on scope (global or actor). */
    float GetCurrentDilation(USkeletalMeshComponent* MeshComp) const;

    /** Apply a dilation value based on scope (global or actor). */
    void ApplyDilation(USkeletalMeshComponent* MeshComp, float NewValue) const;

    /** Compute alpha (0..1) describing how far we are between original and target at this time. */
    float ComputeBlendAlpha() const;

    /** Optionally remap alpha via blendCurve. */
    float ApplyCurveIfAny(float Alpha) const;

    /** Small helper to log when bPrintDebug is true. */
    void LogDebug(USkeletalMeshComponent* MeshComp, const FString& Message) const;
};
