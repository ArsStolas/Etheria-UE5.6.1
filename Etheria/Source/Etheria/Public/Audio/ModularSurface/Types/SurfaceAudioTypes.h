/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "SurfaceAudioTypes" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Sound/SoundBase.h"
#include "SurfaceAudioTypes.generated.h"

UENUM(BlueprintType)
enum class EFootstepGait : uint8
{
    Walk UMETA(DisplayName = "Walk"),
    Run UMETA(DisplayName = "Run"),
    Sprint UMETA(DisplayName = "Sprint")
};

UENUM(BlueprintType)
enum class EFootstepFoot : uint8
{
    Left UMETA(DisplayName = "Left"),
    Right UMETA(DisplayName = "Right")
};

UENUM(BlueprintType)
enum class EWetPlaybackMode : uint8
{
    Switch UMETA(DisplayName = "Switch (Perf)"),
    Crossfade UMETA(DisplayName = "Crossfade (Quality)")
};

USTRUCT(BlueprintType)
struct FSurfaceSoundList
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TArray<TObjectPtr<USoundBase>> Dry;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TArray<TObjectPtr<USoundBase>> Wet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    bool bAvoidImmediateRepeat = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
    float VolumeMin = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
    float VolumeMax = 1.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
    float PitchMin = 0.97f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
    float PitchMax = 1.03f;
};

USTRUCT(BlueprintType)
struct FFootstepAudioSet
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footsteps")
    FSurfaceSoundList Walk;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footsteps")
    FSurfaceSoundList Run;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footsteps")
    FSurfaceSoundList Sprint;
};

USTRUCT(BlueprintType)
struct FLandingAudioConfig
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing")
    FSurfaceSoundList Sounds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing")
    TObjectPtr<class UCurveFloat> VolumeCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing")
    TObjectPtr<class UCurveFloat> PitchCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing", meta = (ClampMin = "0.0"))
    float MinImpactSpeed = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing", meta = (ClampMin = "0.0"))
    float CooldownSeconds = 0.05f;
};

USTRUCT(BlueprintType)
struct FSurfaceAudioEntry
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
    TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType_Default;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
    FFootstepAudioSet Footsteps;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
    FLandingAudioConfig Landing;
};
