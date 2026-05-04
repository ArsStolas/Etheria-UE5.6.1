/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "EtheriaPlayerProfileSave - Header"
 * Notes: Lives in slot "PlayerProfile". Contains everything that does NOT depend on a specific
 *        playthrough: first-launch flag, tutorial-completed flag, settings (audio/graphics/keybinds),
 *        language, total playtime across all saves.
 *        Loaded once at game startup by EtheriaSaveSubsystem.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/Save/SaveTypes.h"
#include "EtheriaPlayerProfileSave.generated.h"

UCLASS(Blueprintable, BlueprintType)
class ETHERIA_API UEtheriaPlayerProfileSave : public USaveGame
{
	GENERATED_BODY()

public:
	UEtheriaPlayerProfileSave();

	/* ═══════════ Lifecycle Flags ═══════════ */

	/** Has the game ever been launched on this install? Used to trigger the past/tutorial flow. */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile") bool bHasPlayedBefore = false;

	/** Has the player completed the past/tutorial section? */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile") bool bTutorialCompleted = false;

	/** Total accumulated play time across all saves (seconds). */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile") float TotalPlayTime = 0.f;

	/** Language code, e.g. "en", "fr", "ja". Used for localization at runtime. */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile") FString PreferredLanguage = TEXT("en");

	/** Last time the profile was saved. */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile") FDateTime LastModified;

	/* ═══════════ Settings ═══════════ */

	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile|Settings") FEtheriaAudioSettings AudioSettings;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile|Settings") FGraphicsSettings GraphicsSettings;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile|Settings") FKeybindSettings KeybindSettings;

	/* ═══════════ Custom Profile-Wide Data ═══════════ */

	/** Free-form profile data (achievements unlocked, lifetime stats, cosmetics owned, etc.).
	 *  Use SetProfileBool / GetProfileBool etc. on the SaveSubsystem for typed access. */
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile|CustomData") TMap<FName, bool>    BoolData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile|CustomData") TMap<FName, int32>   IntData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile|CustomData") TMap<FName, float>   FloatData;
	UPROPERTY(BlueprintReadWrite, Category="Etheria|Profile|CustomData") TMap<FName, FString> StringData;
};
