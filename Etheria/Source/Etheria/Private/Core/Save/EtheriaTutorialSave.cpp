/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "EtheriaTutorialSave - Source"
 * Notes: See header. Lightweight save, deleted automatically on tutorial completion.
 */

#include "Core/Save/EtheriaTutorialSave.h"

UEtheriaTutorialSave::UEtheriaTutorialSave()
{
	CheckpointID    = NAME_None;
	PlayerTransform = FTransform::Identity;
	Timestamp       = FDateTime::UtcNow();
}
