/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: EtheriaBaseGameMode - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EtheriaBaseGameMode.generated.h"

UCLASS()
class ETHERIA_API AEtheriaBaseGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEtheriaBaseGameMode();

protected:
	virtual void BeginPlay() override;
};