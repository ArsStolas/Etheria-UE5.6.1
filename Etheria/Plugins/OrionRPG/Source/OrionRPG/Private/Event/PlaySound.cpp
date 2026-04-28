// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Event/PlaySound.h"
#include "Quest.h"
#include "QuestComponent.h"
#include "QuestBuilderGraph.h"
#include "Kismet/GameplayStatics.h"
#include "QuestBuilderNode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"

UPlaySound::UPlaySound()
{
    VolumeMultiplier = 1.0f;
    PitchMultiplier = 1.0f;
	bPlayAtAttachedPawnLocation = true;
}

void UPlaySound::BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn)
{
    if (SoundToPlay)
    {
        if (SoundType == EPlaySoundType::E_Sound2D)
        {
			UGameplayStatics::PlaySound2D(OwnerController, SoundToPlay, VolumeMultiplier, PitchMultiplier);
        }
        else if (SoundType == EPlaySoundType::E_SoundLocation)
        {
            FVector SoundLocation;
            if (ControlledPawn && bPlayAtAttachedPawnLocation)
                SoundLocation = ControlledPawn->GetActorLocation() + Offset;
            else
                SoundLocation = Location;

            if (GetWorld())
            {
                UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundToPlay, SoundLocation, VolumeMultiplier, PitchMultiplier);
            }
        }
    }
    EndEvent();
}

FString UPlaySound::GetNodeDisplayText_Implementation() const
{
    if (SoundToPlay)
    {
        if (SoundType == EPlaySoundType::E_Sound2D)
        {
            return FString::Printf(TEXT("Play Sound 2D :  %s"), *SoundToPlay->GetFName().ToString());
        }
        else if (SoundType == EPlaySoundType::E_SoundLocation)
        {
            return FString::Printf(TEXT("Play Sound at Location :  %s"), *SoundToPlay->GetFName().ToString());
        }
    }

    return FString("Play Sound Event");
}
