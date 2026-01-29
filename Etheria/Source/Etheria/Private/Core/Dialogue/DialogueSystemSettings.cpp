/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDialogueSystemSettings" - Source
 */
#include "Core/Dialogue/DialogueSystemSettings.h"

const UDialogueSystemSettings* UDialogueSystemSettings::Get()
{
    return GetDefault<UDialogueSystemSettings>();
}
