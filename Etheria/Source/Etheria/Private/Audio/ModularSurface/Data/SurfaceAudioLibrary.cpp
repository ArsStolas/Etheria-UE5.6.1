/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "USurfaceAudioLibrary" - Source
 */

#include "Audio/ModularSurface/Data/SurfaceAudioLibrary.h"

const FSurfaceAudioEntry& USurfaceAudioLibrary::GetEntry(EPhysicalSurface SurfaceType) const
{
    for (const FSurfaceAudioEntry& Entry : Entries)
    {
        if (Entry.SurfaceType == SurfaceType)
        {
            return Entry;
        }
    }

    return DefaultEntry;
}
