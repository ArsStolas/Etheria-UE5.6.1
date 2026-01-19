/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UFootPlacementProviderInterface" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SurfaceAudioTypes.h"
#include "FootPlacementProviderInterface.generated.h"

UINTERFACE(BlueprintType)
class MODULARSURFACEAUDIO_API UFootPlacementProviderInterface : public UInterface
{
    GENERATED_BODY()
};

class MODULARSURFACEAUDIO_API IFootPlacementProviderInterface
{
    GENERATED_BODY()

public:
    /**
     * Optional IK hook. Return the current world location of the requested foot.
     * If returns false, the system falls back to socket location.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SurfaceAudio|IK")
    bool GetFootWorldLocation(EFootstepFoot Foot, FVector& OutWorldLocation) const;
};
