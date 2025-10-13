/*
* Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: BaseEnemy - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/AI/BaseAI.h"
#include "BaseEnemy.generated.h"

UCLASS(Abstract)
class ETHERIA_API ABaseEnemy : public ABaseAI
{
	GENERATED_BODY()

public:
	ABaseEnemy();

protected:
	virtual void BeginPlay() override;
};
