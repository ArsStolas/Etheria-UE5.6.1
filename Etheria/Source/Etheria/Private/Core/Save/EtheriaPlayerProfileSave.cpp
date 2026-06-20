/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "EtheriaPlayerProfileSave - Source"
 * Notes: See header. Default values are tuned for fresh installs.
 */

#include "Core/Save/EtheriaPlayerProfileSave.h"

UEtheriaPlayerProfileSave::UEtheriaPlayerProfileSave()
{
	bHasPlayedBefore   = false;
	bTutorialCompleted = false;
	TotalPlayTime      = 0.f;
	PreferredLanguage  = TEXT("en");
	LastModified       = FDateTime::UtcNow();
}
