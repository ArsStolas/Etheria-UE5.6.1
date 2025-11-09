/*
* Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: BaseNPC - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/AI/BaseAI.h"
#include "BaseNPC.generated.h"

UCLASS(Abstract)
class ETHERIA_API ABaseNPC : public ABaseAI
{
	GENERATED_BODY()

public:
	ABaseNPC();

protected:
	virtual void BeginPlay() override;
};
