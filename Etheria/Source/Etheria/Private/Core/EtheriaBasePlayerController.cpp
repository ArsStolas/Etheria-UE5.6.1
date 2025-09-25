/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: EtheriaPlayerController - Source
*/

#include "Core/EtheriaBasePlayerController.h"

#include "Tests/TestCheatManager.h"

AEtheriaBasePlayerController::AEtheriaBasePlayerController()
{
	CheatClass = UTestCheatManager::StaticClass();
}
