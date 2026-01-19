/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "USurfaceAudioComponent" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "SurfaceAudioTypes.h"
#include "SurfaceAudioLibrary.h"
#include "FootPlacementProviderInterface.h"
#include "SurfaceAudioComponent.generated.h"

UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class MODULARSURFACEAUDIO_API USurfaceAudioComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USurfaceAudioComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio")
    TObjectPtr<USurfaceAudioLibrary> Library = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio|Wet", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Wetness = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio|Wet")
    EWetPlaybackMode WetPlaybackMode = EWetPlaybackMode::Switch;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio|Wet", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WetnessThreshold = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio|Trace")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio|Trace", meta = (ClampMin = "1.0"))
    float TraceDistance = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio|Trace")
    bool bTraceComplex = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio|Debug")
    bool bDebugDraw = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio|Debug", meta = (ClampMin = "0.0"))
    float DebugDrawDuration = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio|Landing")
    bool bEnableLandingVelocityCache = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio|Landing", meta = (ClampMin = "0.01"))
    float LandingVelocitySampleInterval = 0.02f;

    UFUNCTION(BlueprintCallable, Category = "SurfaceAudio|Wet")
    void SetWetness(float NewWetness);

    /**
     * Called by AnimNotify. Plays a spatialized footstep at the left/right foot.
     * If bTryIK is true and the owner implements FootPlacementProviderInterface, IK location is used.
     */
    UFUNCTION(BlueprintCallable, Category = "SurfaceAudio|Footsteps")
    void PlayFootstepFromNotify(EFootstepFoot Foot, EFootstepGait Gait, FName FootSocketName, bool bTryIK);

    /**
     * Plays landing SFX using a provided impact speed (Abs(Velocity.Z)) and hit surface.
     */
    UFUNCTION(BlueprintCallable, Category = "SurfaceAudio|Landing")
    void PlayLandingFromHit(const FHitResult& Hit, float ImpactSpeedAbs);

    /**
     * Plays landing SFX using cached Abs(Velocity.Z) sampled by timer during Falling.
     * If cache isn't running, it falls back to the current velocity.
     */
    UFUNCTION(BlueprintCallable, Category = "SurfaceAudio|Landing")
    void PlayLandingFromCache(const FHitResult& Hit);

    /** Start/stop landing velocity cache (timer). Recommended: start on Falling, stop on Landed. */
    UFUNCTION(BlueprintCallable, Category = "SurfaceAudio|Landing")
    void StartLandingVelocityCache();

    UFUNCTION(BlueprintCallable, Category = "SurfaceAudio|Landing")
    void StopLandingVelocityCache();

protected:
    virtual void BeginPlay() override;

private:
    float CachedAbsVelocityZ = 0.0f;
    bool bLandingCacheActive = false;

    float LastLandingPlayTime = -9999.0f;

    FTimerHandle LandingVelocityTimerHandle;

    // anti-repeat memory (per list) kept simple
    int32 LastFootstepIndex = INDEX_NONE;
    int32 LastLandingIndex = INDEX_NONE;

    bool ResolveFootWorldLocation(EFootstepFoot Foot, FName FootSocketName, bool bTryIK, FVector& OutWorldLocation) const;
    EPhysicalSurface GetSurfaceTypeAtLocation(const FVector& WorldLocation, FHitResult* OutHit = nullptr) const;

    const FSurfaceSoundList& SelectFootstepList(const FFootstepAudioSet& Set, EFootstepGait Gait) const;

    USoundBase* PickRandomSound(const FSurfaceSoundList& List, int32& InOutLastIndex) const;
    void SpawnOneShotAtLocation(USoundBase* Sound, const FVector& Location, float Volume, float Pitch) const;

    void PlayOneShotFromList(const FSurfaceSoundList& List, int32& InOutLastIndex, const FVector& Location, float VolumeMul, float PitchMul) const;

    void HandleLandingVelocitySample();
};
