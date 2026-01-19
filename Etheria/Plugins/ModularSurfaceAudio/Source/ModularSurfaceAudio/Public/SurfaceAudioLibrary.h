/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "USurfaceAudioLibrary" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SurfaceAudioTypes.h"
#include "SurfaceAudioLibrary.generated.h"

UCLASS(BlueprintType)
class MODULARSURFACEAUDIO_API USurfaceAudioLibrary : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio")
    FSurfaceAudioEntry DefaultEntry;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SurfaceAudio")
    TArray<FSurfaceAudioEntry> Entries;

    const FSurfaceAudioEntry& GetEntry(EPhysicalSurface SurfaceType) const;
};
