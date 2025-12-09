/**
 * Etheria's End Project, 2025
 * Created by: 0nnen
 * Last Updated by: 0nnen
 * Class: "AnimNotifyState_SlowMo" - Source
 */

#include "Components/Combat/Combat-AnimNotify/AnimNotifyState_SlowMo.h"

#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Curves/CurveFloat.h"

UAnimNotifyState_SlowMo::UAnimNotifyState_SlowMo()
{
    runtimeTotalDuration   = 0.0f;
    runtimeElapsed         = 0.0f;
    runtimeOriginalDilation = 1.0f;
    runtimeTargetDilation   = 1.0f;
    bHasCapturedOriginal    = false;
}

void UAnimNotifyState_SlowMo::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    runtimeTotalDuration = FMath::Max(TotalDuration, KINDA_SMALL_NUMBER);
    runtimeElapsed = 0.0f;
    bHasCapturedOriginal = false;

    if (!MeshComp)
    {
        return;
    }

    // Capture the current time dilation as "original" for this run
    runtimeOriginalDilation = GetCurrentDilation(MeshComp);
    bHasCapturedOriginal = true;

    // Prepare target for this run
    runtimeTargetDilation = FMath::Clamp(targetTimeDilation, 0.01f, 2.0f);

    if (bPrintDebug)
    {
        FString Msg = FString::Printf(TEXT("[SlowMo] BEGIN | Scope=%s | Original=%.3f | Target=%.3f | Duration=%.3fs"),
            scope == ESlowMoScope::Global ? TEXT("Global") : TEXT("Owner"),
            runtimeOriginalDilation,
            runtimeTargetDilation,
            runtimeTotalDuration);
        LogDebug(MeshComp, Msg);
    }

    // If there is no blend-in, apply instantly
    if (blendInTime <= 0.0f)
    {
        ApplyDilation(MeshComp, runtimeTargetDilation);
    }
}

void UAnimNotifyState_SlowMo::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

    if (!MeshComp || !bHasCapturedOriginal)
    {
        return;
    }

    runtimeElapsed += FrameDeltaTime;

    // Clamp elapsed within [0, totalDuration]
    runtimeElapsed = FMath::Clamp(runtimeElapsed, 0.0f, runtimeTotalDuration);

    const float Alpha = ComputeBlendAlpha();
    const float RemappedAlpha = ApplyCurveIfAny(Alpha);
    const float NewDilation = FMath::Lerp(runtimeOriginalDilation, runtimeTargetDilation, RemappedAlpha);

    ApplyDilation(MeshComp, NewDilation);

    if (bPrintDebug)
    {
        FString Msg = FString::Printf(TEXT("[SlowMo] TICK | Alpha=%.3f | Remapped=%.3f | Dilation=%.3f"),
            Alpha, RemappedAlpha, NewDilation);
        LogDebug(MeshComp, Msg);
    }
}

void UAnimNotifyState_SlowMo::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp || !bHasCapturedOriginal)
    {
        return;
    }

    // If we do not want to restore anything at the end, just keep the current value
    if (!bRestoreOnEnd)
    {
        if (bPrintDebug)
        {
            LogDebug(MeshComp, TEXT("[SlowMo] END | bRestoreOnEnd=false, keeping current dilation."));
        }
        return;
    }

    // Determine the value we want to restore to
    const float RestoreValue = (overrideRestoreDilation > 0.0f)
        ? overrideRestoreDilation
        : runtimeOriginalDilation;

    // If there was no blend-out configured, we hard reset here.
    if (blendOutTime <= 0.0f)
    {
        ApplyDilation(MeshComp, RestoreValue);

        if (bPrintDebug)
        {
            FString Msg = FString::Printf(TEXT("[SlowMo] END | Instant restore to %.3f"), RestoreValue);
            LogDebug(MeshComp, Msg);
        }
    }
    else
    {
        // If blendOutTime > 0, most of the smooth out should already have happened
        // during NotifyTick in the last part of the notify window.
        // We still force the final value here to avoid small drift.
        ApplyDilation(MeshComp, RestoreValue);

        if (bPrintDebug)
        {
            FString Msg = FString::Printf(TEXT("[SlowMo] END | Final restore to %.3f (after smooth-out)."), RestoreValue);
            LogDebug(MeshComp, Msg);
        }
    }

    // Reset runtime state
    runtimeTotalDuration    = 0.0f;
    runtimeElapsed          = 0.0f;
    runtimeOriginalDilation = 1.0f;
    runtimeTargetDilation   = 1.0f;
    bHasCapturedOriginal    = false;
}

float UAnimNotifyState_SlowMo::GetCurrentDilation(USkeletalMeshComponent* MeshComp) const
{
    if (!MeshComp)
    {
        return 1.0f;
    }

    if (scope == ESlowMoScope::Global)
    {
        if (UWorld* World = MeshComp->GetWorld())
        {
            return UGameplayStatics::GetGlobalTimeDilation(World);
        }
    }
    else if (scope == ESlowMoScope::OwnerActor)
    {
        if (AActor* Owner = MeshComp->GetOwner())
        {
            return Owner->CustomTimeDilation;
        }
    }

    return 1.0f;
}

void UAnimNotifyState_SlowMo::ApplyDilation(USkeletalMeshComponent* MeshComp, float NewValue) const
{
    if (!MeshComp)
    {
        return;
    }

    if (scope == ESlowMoScope::Global)
    {
        if (UWorld* World = MeshComp->GetWorld())
        {
            UGameplayStatics::SetGlobalTimeDilation(World, NewValue);
        }
    }
    else if (scope == ESlowMoScope::OwnerActor)
    {
        if (AActor* Owner = MeshComp->GetOwner())
        {
            Owner->CustomTimeDilation = NewValue;
        }
    }
}

float UAnimNotifyState_SlowMo::ComputeBlendAlpha() const
{
    const float SafeTotal = FMath::Max(runtimeTotalDuration, KINDA_SMALL_NUMBER);
    const float InTime    = FMath::Max(blendInTime, 0.0f);
    const float OutTime   = FMath::Max(blendOutTime, 0.0f);

    const float TimeRemaining = SafeTotal - runtimeElapsed;

    float Alpha = 0.0f;

    // 1) Blend-in phase
    if (InTime > KINDA_SMALL_NUMBER && runtimeElapsed < InTime)
    {
        Alpha = FMath::Clamp(runtimeElapsed / InTime, 0.0f, 1.0f);
    }
    // 2) Blend-out phase (only if we have a blendOutTime and we are close to the end)
    else if (OutTime > KINDA_SMALL_NUMBER && TimeRemaining < OutTime)
    {
        const float FadeOutElapsed = FMath::Clamp(OutTime - TimeRemaining, 0.0f, OutTime);
        const float T = FMath::Clamp(FadeOutElapsed / OutTime, 0.0f, 1.0f);

        // We want to go from Alpha=1 at start of fade-out to Alpha=0 at the very end.
        Alpha = 1.0f - T;
    }
    // 3) Middle of the notify (in between blend-in and blend-out)
    else
    {
        Alpha = bHoldAtTarget ? 1.0f : 0.0f;
    }

    return Alpha;
}

float UAnimNotifyState_SlowMo::ApplyCurveIfAny(float Alpha) const
{
    if (!blendCurve)
    {
        return Alpha;
    }

    const float CurveTime = FMath::Clamp(Alpha, 0.0f, 1.0f);
    const float CurveValue = blendCurve->GetFloatValue(CurveTime);
    return FMath::Clamp(CurveValue, 0.0f, 1.0f);
}

void UAnimNotifyState_SlowMo::LogDebug(USkeletalMeshComponent* MeshComp, const FString& Message) const
{
    if (!MeshComp)
    {
        UE_LOG(LogTemp, Log, TEXT("[SlowMo] %s"), *Message);
        return;
    }

    AActor* Owner = MeshComp->GetOwner();
    const FString OwnerName = Owner ? Owner->GetName() : TEXT("None");

    UE_LOG(LogTemp, Log, TEXT("[SlowMo] Owner=%s | %s"), *OwnerName, *Message);
}
