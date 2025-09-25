/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: EtheriaBaseGameInstance - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "EtheriaBaseGameInstance.generated.h"

class UHealthTestManager;

UCLASS()
class ETHERIA_API UEtheriaBaseGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UEtheriaBaseGameInstance();

	virtual void Init() override;

protected:
};
