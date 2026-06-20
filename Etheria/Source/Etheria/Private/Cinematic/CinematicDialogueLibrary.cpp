/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: UCinematicDialogueLibrary - Source
 */

#include "Cinematic/CinematicDialogueLibrary.h"
#include "Cinematic/CinematicDirectorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

UCinematicDirectorComponent* UCinematicDialogueLibrary::GetCinematicDirector(const UObject* WorldContextObject)
{
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
    {
        return PC->FindComponentByClass<UCinematicDirectorComponent>();
    }
    return nullptr;
}

void UCinematicDialogueLibrary::ShowCinematicDialogue(const UObject* WorldContextObject, FText Speaker, FText Text, bool bWaitForInput)
{
    if (UCinematicDirectorComponent* Director = GetCinematicDirector(WorldContextObject))
    {
        Director->ShowDialogueLine(Speaker, Text, bWaitForInput);
    }
}

void UCinematicDialogueLibrary::HideCinematicDialogue(const UObject* WorldContextObject)
{
    if (UCinematicDirectorComponent* Director = GetCinematicDirector(WorldContextObject))
    {
        Director->HideDialogue();
    }
}
