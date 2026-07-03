/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "AIDeathLedgerSubsystem - Header"
 * Notes: World-Partition-proof AI death persistence. Streamed-out corpses stay dead when their cell reloads.
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Characters/AI/AI_Types.h"
#include "AIDeathLedgerSubsystem.generated.h"

USTRUCT()
struct FAIDeathRecord
{
	GENERATED_BODY()

	double DeathTime = 0.0;
	EAIRespawnCondition Condition = EAIRespawnCondition::Never;
	float RespawnDelay = 0.f;
};

UCLASS()
class ETHERIA_API UAIDeathLedgerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void RegisterDeath(const AActor* Actor, EAIRespawnCondition Condition, float RespawnDelay);

	bool ShouldStayDead(const AActor* Actor);

	void ClearDeath(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="AI|Respawn",
		meta=(ToolTip="Flush death records matching a respawn condition (call from the save system with OnSave, from the day cycle with OnDayCycle) so those AI come back on their next cell load."))
	void FlushDeathsForCondition(EAIRespawnCondition Condition);

	UFUNCTION(BlueprintCallable, Category="AI|Respawn",
		meta=(ToolTip="Forget every death (full world reset)."))
	void ResetLedger();

private:
	TMap<FString, FAIDeathRecord> Deaths;
};
