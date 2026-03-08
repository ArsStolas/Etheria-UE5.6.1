/*
* Etheria's End Project, 2025
* Boss Blackboard Data - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BlackboardData.h"
#include "BossBlackboardData.generated.h"

namespace BossBlackboardKeys
{
	const FName TargetActor(TEXT("TargetActor"));
	const FName BossPhase(TEXT("BossPhase"));
}

/** Blackboard du boss : TargetActor (Object), BossPhase (Int). */
UCLASS()
class ETHERIA_API UBossBlackboardData : public UBlackboardData
{
	GENERATED_BODY()

public:
	UBossBlackboardData(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
