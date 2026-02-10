/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: BaseTestManager - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BaseTestManager.generated.h"

UCLASS(Blueprintable)
class ETHERIA_API UBaseTestManager : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(UObject* Outer);
};
