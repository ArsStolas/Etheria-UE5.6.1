/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "SaveTypes - Header"
 * Notes: Shared enums, structs and metadata for the Etheria save system.
 *        FSaveSlotInfo is the BP-friendly summary used by load/save UIs.
 *        FAudio/Graphics/KeybindSettings live in the player profile save.
 */

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "SaveTypes.generated.h"

/* ═══════════ Enums ═══════════ */

/** Type of save slot. Auto = silent save at campfires. Manual = chosen by player from a menu. */
UENUM(BlueprintType)
enum class ESaveSlotType : uint8
{
	Auto    UMETA(DisplayName = "Auto Save"),
	Manual  UMETA(DisplayName = "Manual Save")
};

/** Game era — affects which Data Layer is active and which player avatar spawns. */
UENUM(BlueprintType)
enum class EGameEra : uint8
{
	Past    UMETA(DisplayName = "Past (Tutorial / Burning World)"),
	Present UMETA(DisplayName = "Present (Main Game)")
};

/** Result of a save/load operation — broadcast through OnGameSaved / OnGameLoaded. */
UENUM(BlueprintType)
enum class ESaveOpResult : uint8
{
	Success     UMETA(DisplayName = "Success"),
	Failed      UMETA(DisplayName = "Failed"),
	NotFound    UMETA(DisplayName = "Not Found"),
	Corrupted   UMETA(DisplayName = "Corrupted")
};

/* ═══════════ Slot Info ═══════════ */

/** Lightweight summary of a save slot. Used by the load/save menu UI. */
USTRUCT(BlueprintType)
struct ETHERIA_API FSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Etheria|Save") bool bExists = false;
	UPROPERTY(BlueprintReadOnly, Category="Etheria|Save") ESaveSlotType SlotType = ESaveSlotType::Auto;
	UPROPERTY(BlueprintReadOnly, Category="Etheria|Save") int32 SlotIndex = 0;
	UPROPERTY(BlueprintReadOnly, Category="Etheria|Save") FString SlotName;
	UPROPERTY(BlueprintReadOnly, Category="Etheria|Save") FDateTime Timestamp;
	UPROPERTY(BlueprintReadOnly, Category="Etheria|Save") FString LocationLabel;
	UPROPERTY(BlueprintReadOnly, Category="Etheria|Save") int32 PlayerLevel = 0;
	UPROPERTY(BlueprintReadOnly, Category="Etheria|Save") float PlayTime = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Etheria|Save") EGameEra Era = EGameEra::Present;
};

/* ═══════════ Settings ═══════════ */

/** Audio settings — stored in the player profile, applied via Sound Class volumes. */
USTRUCT(BlueprintType)
struct ETHERIA_API FEtheriaAudioSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0", ClampMax="1")) float MasterVolume = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0", ClampMax="1")) float MusicVolume = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0", ClampMax="1")) float SFXVolume = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0", ClampMax="1")) float VoiceVolume = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0", ClampMax="1")) float AmbientVolume = 0.7f;
};

/** Graphics settings — applied to UGameUserSettings on load and on change. */
USTRUCT(BlueprintType)
struct ETHERIA_API FGraphicsSettings
{
	GENERATED_BODY()

	/** Overall preset (0=Low, 1=Medium, 2=High, 3=Epic, 4=Cinematic). Drives the Scalability groups. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics", meta=(ClampMin="0", ClampMax="4"))
	int32 OverallQuality = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics") FIntPoint Resolution = FIntPoint(1920, 1080);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics") bool bFullscreen = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics") bool bVSync = true;

	/** Frame rate cap. Set 0 for unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics", meta=(ClampMin="0"))
	int32 FrameRateLimit = 60;

	/** Per-group scalability (overrides OverallQuality if any value differs from -1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics", meta=(ClampMin="-1", ClampMax="4")) int32 ViewDistanceQuality = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics", meta=(ClampMin="-1", ClampMax="4")) int32 ShadowQuality = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics", meta=(ClampMin="-1", ClampMax="4")) int32 PostProcessQuality = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics", meta=(ClampMin="-1", ClampMax="4")) int32 TextureQuality = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics", meta=(ClampMin="-1", ClampMax="4")) int32 EffectsQuality = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Graphics", meta=(ClampMin="-1", ClampMax="4")) int32 FoliageQuality = -1;
};

/** Per-action key remap — Enhanced Input compatible. */
USTRUCT(BlueprintType)
struct ETHERIA_API FKeybindEntry
{
	GENERATED_BODY()

	/** Name of the InputAction (e.g. "IA_Jump", "IA_Interact"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Keybinds") FName ActionName;

	/** Primary key. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Keybinds") FKey PrimaryKey;

	/** Optional secondary key. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Keybinds") FKey SecondaryKey;
};

/** All custom keybinds for the player. Stored in the player profile. */
USTRUCT(BlueprintType)
struct ETHERIA_API FKeybindSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Keybinds") TArray<FKeybindEntry> Bindings;

	/** Mouse sensitivity (1.0 = default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Keybinds", meta=(ClampMin="0.1", ClampMax="10")) float MouseSensitivity = 1.f;

	/** Invert Y axis on look. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Keybinds") bool bInvertY = false;
};

/* ═══════════ Blob (advanced) ═══════════ */

/** Raw byte container for serialized custom structs (used by SetSaveBytes / GetSaveBytes). */
USTRUCT(BlueprintType)
struct ETHERIA_API FSaveBlob
{
	GENERATED_BODY()

	UPROPERTY() TArray<uint8> Bytes;

	bool IsEmpty() const { return Bytes.Num() == 0; }
};
