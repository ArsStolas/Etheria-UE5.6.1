/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "EtheriaGameProgressSave - Header"
 * Notes: One save per slot (1 auto + 3 manual). Contains the player's actual progression:
 *        position, era, last campfire, plus a typed custom-data dictionary that any BP/C++
 *        system can write to via Set Save X / Get Save X on the SaveSubsystem.
 *        This is the main save the player thinks about when they hear "save game".
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/Save/SaveTypes.h"
#include "EtheriaGameProgressSave.generated.h"

UCLASS(Blueprintable, BlueprintType)
class ETHERIA_API UEtheriaGameProgressSave : public USaveGame
{
	GENERATED_BODY()

public:
	UEtheriaGameProgressSave();

	/* ═══════════ Slot Metadata ═══════════ */

	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|Metadata") ESaveSlotType SlotType = ESaveSlotType::Auto;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|Metadata") int32 SlotIndex = 0;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|Metadata") FDateTime SaveTimestamp;

	/** Friendly name shown in the load menu (e.g. "Forgotten Forest", "Campfire of Whispers"). */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|Metadata") FString LocationLabel;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|Metadata") int32 PlayerLevel = 1;

	/** Time played in this save (seconds). */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|Metadata") float PlayTime = 0.f;

	/* ═══════════ Player State ═══════════ */

	/** Which era the player is currently in. Drives the active Data Layer at load time. */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|Player") EGameEra Era = EGameEra::Present;

	/** Spawn transform when this save is loaded. Usually the last-used campfire. */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|Player") FTransform PlayerTransform;

	/** Stable ID of the last campfire used (for "Continue" button to spawn the player at the right place). */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|Player") FName LastCampfireID;

	/* ═══════════ Custom Typed Data (the "Set Save X" backbone) ═══════════ */
	// All BP/C++ Set Save X / Get Save X calls land in these maps.
	// Each map is a (FName Key) -> Value lookup. Keys should be globally unique across systems
	// (e.g. "Quest.MainQuest.GoblinSlain", "Inventory.GoldCount", "Player.Stamina").

	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, bool>            BoolData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, int32>           IntData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, float>           FloatData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, FString>         StringData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, FName>           NameData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, FVector>         VectorData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, FRotator>        RotatorData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, FTransform>      TransformData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, FSoftObjectPath> ObjectPathData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, FSoftClassPath>  ClassPathData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, FGuid>           GuidData;

	/** Raw byte blobs for custom-struct serialization (advanced — used by SetSaveStruct wildcard nodes). */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Progress|CustomData") TMap<FName, FSaveBlob>       BlobData;

	/* ═══════════ Helpers ═══════════ */

	/** Wipe every data map. Called by SaveSubsystem when starting a New Game. */
	void ClearAllData();

	/** Build a slot info struct from this save's metadata. */
	FSaveSlotInfo BuildSlotInfo() const;
};
