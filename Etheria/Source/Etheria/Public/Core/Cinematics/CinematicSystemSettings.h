/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UCinematicSystemSettings" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CinematicSystemSettings.generated.h"

class ACinematicCameraRigBase;

/**
 * Project settings for the Cinematic system (Project Settings -> Game -> Cinematic System).
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Cinematic System"))
class ETHERIA_API UCinematicSystemSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const UCinematicSystemSettings* Get();

    /** Optional default rig class used when a request doesn't specify CameraRigClass. */
    UPROPERTY(Config, EditAnywhere, Category="Defaults")
    TSubclassOf<ACinematicCameraRigBase> DefaultRigClass;

    UPROPERTY(Config, EditAnywhere, Category="Defaults", meta=(ClampMin="0.0"))
    float DefaultBlendInTime = 0.25f;

    UPROPERTY(Config, EditAnywhere, Category="Defaults", meta=(ClampMin="0.0"))
    float DefaultBlendOutTime = 0.25f;
};
