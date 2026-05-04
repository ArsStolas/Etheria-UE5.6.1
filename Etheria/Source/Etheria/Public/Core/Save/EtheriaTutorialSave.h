/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "EtheriaTutorialSave - Header"
 * Notes: Lightweight save in slot "TutorialCheckpoint". Auto-written by checkpoint triggers
 *        during the past/tutorial section. Auto-deleted by the SaveSubsystem when the tutorial
 *        is completed. Kept separate from GameProgress so the tutorial flow is fully isolated.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EtheriaTutorialSave.generated.h"

UCLASS(Blueprintable, BlueprintType)
class ETHERIA_API UEtheriaTutorialSave : public USaveGame
{
	GENERATED_BODY()

public:
	UEtheriaTutorialSave();

	/** Stable ID of the last checkpoint trigger crossed. Used to respawn at the right place. */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Tutorial") FName CheckpointID;

	/** Spawn transform on respawn. */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Tutorial") FTransform PlayerTransform;

	/** Steps completed during the tutorial (e.g. "LearnedDodge", "FirstFireExtinguished"). */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Tutorial") TMap<FName, bool> CompletedSteps;

	UPROPERTY(BlueprintReadWrite, Category="Etheria|Tutorial") FDateTime Timestamp;
};
