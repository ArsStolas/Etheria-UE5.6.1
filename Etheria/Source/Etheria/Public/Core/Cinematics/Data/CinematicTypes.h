/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "CinematicTypes" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Core/Common/Data/SessionControlTypes.h"
#include "CinematicTypes.generated.h"

class AActor;
class ACinematicCameraRigBase;
class UCameraShakeBase;

UENUM(BlueprintType)
enum class ECinematicEndReason : uint8
{
    Completed   UMETA(DisplayName="Completed"),
    Interrupted UMETA(DisplayName="Interrupted"),
    Skipped     UMETA(DisplayName="Skipped"),
    Cancelled   UMETA(DisplayName="Cancelled")
};

USTRUCT(BlueprintType)
struct FCinematicControlPolicy
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinematic|Policy")
    FSessionControlPolicy Controls;

    /** If true, player can request skip (your BP can react). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinematic|Policy")
    bool bAllowSkip = true;
};

USTRUCT(BlueprintType)
struct FCinematicSessionRequest
{
    GENERATED_BODY()

    /** Optional target actor (e.g. boss). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinematic")
    TSoftObjectPtr<AActor> Target;

    /** Use a rig (recommended for dynamic follow + look-at). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinematic")
    TSubclassOf<ACinematicCameraRigBase> CameraRigClass;

    /** Alternatively, set a static camera actor (CineCameraActor / CameraActor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinematic")
    TSoftObjectPtr<AActor> StaticCameraActor;

    /** Blend time when entering this cinematic camera. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinematic", meta=(ClampMin="0.0"))
    float BlendInTime = 0.25f;

    /** Blend time when leaving this cinematic camera. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinematic", meta=(ClampMin="0.0"))
    float BlendOutTime = 0.25f;

    /** If > 0, the subsystem auto-ends after this duration. Otherwise, you end manually. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinematic", meta=(ClampMin="0.0"))
    float AutoEndAfter = 0.0f;

    /** Policy applied on start; restored on end. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinematic")
    FCinematicControlPolicy Policy;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCinematicSessionStarted, const FCinematicSessionRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCinematicSessionEnded, ECinematicEndReason, Reason);
