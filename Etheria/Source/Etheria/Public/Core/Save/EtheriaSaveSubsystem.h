/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "EtheriaSaveSubsystem - Header"
 * Notes: Central save/load dispatcher accessible from anywhere in BP via Get Save Subsystem.
 *
 *        Ultra-simple BP workflow:
 *          1) Anywhere: Set Save X (Key, Value)   ─── stores into the active progress save
 *          2) When ready: Save Progress (SlotType, [SlotIndex])   ─── writes to disk async
 *
 *        On load:
 *          1) Load Progress (SlotType, [SlotIndex])   ─── reads from disk async
 *          2) Anywhere: Get Save X (Key, Default)
 *          OnLoadCompleted is also broadcast to every actor implementing ISaveable.
 *
 *        This subsystem owns three SaveGames:
 *          - PlayerProfile (boot-loaded, contains settings + first-launch flag)
 *          - GameProgress  (active playthrough; one of 4 slots)
 *          - TutorialSave  (auto-checkpoint during the past section, auto-deleted when done)
 *
 *        All save/load uses Async UGameplayStatics calls to avoid frame hitches.
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/Save/SaveTypes.h"
#include "EtheriaSaveSubsystem.generated.h"

class UEtheriaPlayerProfileSave;
class UEtheriaGameProgressSave;
class UEtheriaTutorialSave;
class USaveGame;

/* ═══════════ Dispatchers ═══════════ */

/** Fires just BEFORE writing to disk — registered ISaveable actors get OnSaveRequested at this point. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPreSave, ESaveSlotType, SlotType);

/** Fires after a save operation completes (success or failure). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameSaved, const FSaveSlotInfo&, SlotInfo, ESaveOpResult, Result);

/** Fires just BEFORE loading from disk — gives systems a chance to clear transient state. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPreLoad, ESaveSlotType, SlotType);

/** Fires after a load operation completes — registered ISaveable actors get OnLoadCompleted at this point. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameLoaded, const FSaveSlotInfo&, SlotInfo, ESaveOpResult, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNewGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialCompletedDispatch);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEraChanged, EGameEra, OldEra, EGameEra, NewEra);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProfileLoaded, UEtheriaPlayerProfileSave*, Profile);

UCLASS()
class ETHERIA_API UEtheriaSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/* ═══════════ Subsystem Lifecycle ═══════════ */

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Static helper to grab the subsystem from anywhere with a WorldContext. BP-callable shortcut. */
	UFUNCTION(BlueprintPure, Category="Etheria|Save", meta=(WorldContext="WorldContextObject", DisplayName="Get Save Subsystem"))
	static UEtheriaSaveSubsystem* Get(const UObject* WorldContextObject);

	/* ═══════════ Profile API ═══════════ */

	/** True the very first time the game ever launches on this install. */
	UFUNCTION(BlueprintPure, Category="Etheria|Save|Profile") bool IsFirstLaunch() const;

	/** Mark the profile as launched (called automatically once gameplay actually begins). */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Profile") void MarkAsLaunched();

	/** True if the past/tutorial section has been completed at least once. */
	UFUNCTION(BlueprintPure, Category="Etheria|Save|Profile") bool IsTutorialCompleted() const;

	/** Marks the tutorial as completed, deletes the tutorial-checkpoint save, broadcasts OnTutorialCompleted. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Profile") void MarkTutorialCompleted();

	/** Total play time accumulated across all sessions (seconds). */
	UFUNCTION(BlueprintPure, Category="Etheria|Save|Profile") float GetTotalPlayTime() const;

	/** Direct access to the profile object (mostly for advanced cases). */
	UFUNCTION(BlueprintPure, Category="Etheria|Save|Profile") UEtheriaPlayerProfileSave* GetPlayerProfile() const { return PlayerProfile; }

	/** Persist the profile to disk (async). Settings APIs already call this internally. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Profile") void SaveProfile();

	/* ═══════════ Settings API ═══════════ */

	UFUNCTION(BlueprintPure, Category="Etheria|Save|Settings") FEtheriaAudioSettings GetAudioSettings() const;
	UFUNCTION(BlueprintPure, Category="Etheria|Save|Settings") FGraphicsSettings GetGraphicsSettings() const;
	UFUNCTION(BlueprintPure, Category="Etheria|Save|Settings") FKeybindSettings GetKeybindSettings() const;

	/** Apply + persist audio settings. Broadcasts OnSettingsChanged. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Settings") void ApplyAudioSettings(const FEtheriaAudioSettings& NewSettings);

	/** Apply + persist graphics settings (also pushes to UGameUserSettings). Broadcasts OnSettingsChanged. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Settings") void ApplyGraphicsSettings(const FGraphicsSettings& NewSettings);

	/** Apply + persist keybind settings. Broadcasts OnSettingsChanged. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Settings") void ApplyKeybindSettings(const FKeybindSettings& NewSettings);

	/** Reset every setting to engine defaults. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Settings") void ResetSettingsToDefaults();

	/* ═══════════ Game Progress — Save / Load ═══════════ */

	/** Async-save the current progress to a slot. Broadcasts OnGameSaved when done.
	 *  For Auto, SlotIndex is ignored (always slot 0). For Manual, SlotIndex is 0/1/2. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress")
	void SaveProgress(ESaveSlotType SlotType, int32 SlotIndex = 0);

	/** Sync save (blocks the game thread until the file is written). Use only if you really need the result this frame. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress")
	bool SaveProgressSync(ESaveSlotType SlotType, int32 SlotIndex = 0);

	/** Async-load progress from a slot. Broadcasts OnGameLoaded when done. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress")
	void LoadProgress(ESaveSlotType SlotType, int32 SlotIndex = 0);

	/** Auto-pick the most recent of the 4 slots and load it. Useful for the Continue button. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress")
	void LoadMostRecentProgress();

	/** Find the most recent slot's metadata without actually loading the full save. Returns bExists=false if none. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress")
	FSaveSlotInfo GetMostRecentSlotInfo();

	/** Get the metadata for a specific slot. bExists=false if it doesn't exist. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress")
	FSaveSlotInfo GetSlotInfo(ESaveSlotType SlotType, int32 SlotIndex = 0);

	/** Get metadata for all 4 slots in a single call. Sorted by SlotType then SlotIndex. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress")
	TArray<FSaveSlotInfo> GetAllSlotsInfo();

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress") bool DoesSlotExist(ESaveSlotType SlotType, int32 SlotIndex = 0) const;
	UFUNCTION(BlueprintPure,    Category="Etheria|Save|Progress") bool HasAnySave() const;
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress") bool DeleteSlot(ESaveSlotType SlotType, int32 SlotIndex = 0);
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress") void DeleteAllSlots();

	/** Reset the in-memory progress save to defaults and broadcast OnNewGameStarted.
	 *  Does NOT write to disk yet — call SaveProgress(Auto) afterwards (typically at the first campfire). */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress")
	void StartNewGame(EGameEra StartingEra = EGameEra::Past);

	/** Direct access to the current in-memory progress (mostly for advanced cases). */
	UFUNCTION(BlueprintPure, Category="Etheria|Save|Progress")
	UEtheriaGameProgressSave* GetCurrentProgress() const { return CurrentProgress; }

	/* ═══════════ Player Position Helpers ═══════════ */

	/** Convenience: write the player's current location into the active progress.
	 *  Called by campfires before SaveProgress. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Progress")
	void RecordPlayerCheckpoint(const FTransform& Where, FName CampfireID, const FString& LocationLabel);

	/* ═══════════ Era ═══════════ */

	UFUNCTION(BlueprintPure, Category="Etheria|Save|Era") EGameEra GetCurrentEra() const;
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Era") void SetCurrentEra(EGameEra NewEra);

	/* ═══════════ Tutorial Checkpoints ═══════════ */

	/** Write a tutorial checkpoint (auto-save during the past section). */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Tutorial")
	void SaveTutorialCheckpoint(FName CheckpointID, const FTransform& PlayerTransform);

	/** Load the latest tutorial checkpoint, returns false if none exists. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Tutorial")
	bool LoadTutorialCheckpoint(UEtheriaTutorialSave*& OutSave);

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Tutorial") void MarkTutorialStepCompleted(FName StepID);
	UFUNCTION(BlueprintPure,    Category="Etheria|Save|Tutorial") bool IsTutorialStepCompleted(FName StepID) const;
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Tutorial") void ClearTutorialCheckpoint();

	/* ═══════════ Custom Data — Set/Get (THE killer BP API) ═══════════ */
	// Each pair stores into / reads from the active GameProgress save.
	// Keys should be globally unique. Use a namespacing convention: "Quest.MainQuest.GoblinSlain".

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveBool   (FName Key, bool   Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") bool   GetSaveBool   (FName Key, bool   DefaultValue = false) const;

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveInt    (FName Key, int32  Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") int32  GetSaveInt    (FName Key, int32  DefaultValue = 0) const;

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveFloat  (FName Key, float  Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") float  GetSaveFloat  (FName Key, float  DefaultValue = 0.f) const;

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveString (FName Key, const FString& Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") FString GetSaveString(FName Key, const FString& DefaultValue = TEXT("")) const;

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveName   (FName Key, FName  Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") FName  GetSaveName   (FName Key, FName  DefaultValue) const;

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveVector    (FName Key, FVector    Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") FVector    GetSaveVector    (FName Key, FVector    DefaultValue) const;

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveRotator   (FName Key, FRotator   Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") FRotator   GetSaveRotator   (FName Key, FRotator   DefaultValue) const;

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveTransform (FName Key, FTransform Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") FTransform GetSaveTransform (FName Key, FTransform DefaultValue) const;

	/** Save a soft pointer to any UObject. Use for refs that survive level streaming. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveObject (FName Key, UObject* Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") UObject* GetSaveObject(FName Key) const;

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveClass  (FName Key, UClass* Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") UClass* GetSaveClass(FName Key) const;

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveGuid   (FName Key, FGuid Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") FGuid GetSaveGuid (FName Key, FGuid DefaultValue) const;

	/* ═══════════ Custom Data — Wildcard Struct Support (advanced) ═══════════ */

	/** Save ANY struct by value. Wildcard pin in BP — connect any struct, it just works.
	 *  Internally serialized to FSaveBlob via FObjectAndNameAsStringProxyArchive. */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Etheria|Save|Data",
		meta=(CustomStructureParam="Value", DisplayName="Set Save Struct"))
	void SetSaveStruct(FName Key, const int32& Value);
	DECLARE_FUNCTION(execSetSaveStruct);

	/** Read ANY struct by value. Wildcard pin — connect a struct ref, get true if found. */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Etheria|Save|Data",
		meta=(CustomStructureParam="OutValue", DisplayName="Get Save Struct"))
	bool GetSaveStruct(FName Key, int32& OutValue);
	DECLARE_FUNCTION(execGetSaveStruct);

	/** Save an array of bytes directly (for custom serialization). */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void SetSaveBytes(FName Key, const TArray<uint8>& Bytes);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") bool GetSaveBytes(FName Key, TArray<uint8>& OutBytes) const;

	/* ═══════════ Custom Data — Utility ═══════════ */

	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Data") bool HasSaveKey(FName Key) const;
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void ClearSaveKey(FName Key);
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Data") void ClearAllSaveData();

	/* ═══════════ Profile-Wide Custom Data ═══════════ */
	// Same idea as above, but written to the PROFILE save (lifetime stats, achievements, cosmetics).

	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Profile|Data") void SetProfileBool  (FName Key, bool Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Profile|Data") bool GetProfileBool  (FName Key, bool DefaultValue = false) const;
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Profile|Data") void SetProfileInt   (FName Key, int32 Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Profile|Data") int32 GetProfileInt  (FName Key, int32 DefaultValue = 0) const;
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Profile|Data") void SetProfileFloat (FName Key, float Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Profile|Data") float GetProfileFloat(FName Key, float DefaultValue = 0.f) const;
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Profile|Data") void SetProfileString(FName Key, const FString& Value);
	UFUNCTION(BlueprintPure,     Category="Etheria|Save|Profile|Data") FString GetProfileString(FName Key, const FString& DefaultValue = TEXT("")) const;

	/* ═══════════ Saveable Actor Registry ═══════════ */

	/** Register an actor implementing ISaveable so it gets OnSaveRequested / OnLoadCompleted callbacks. */
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Saveable") void RegisterSaveable(UObject* SaveableObject);
	UFUNCTION(BlueprintCallable, Category="Etheria|Save|Saveable") void UnregisterSaveable(UObject* SaveableObject);

	/* ═══════════ Dispatchers — BIND THESE IN YOUR BP ═══════════ */

	UPROPERTY(BlueprintAssignable, Category="Etheria|Save|Events") FOnPreSave                  OnPreSave;
	UPROPERTY(BlueprintAssignable, Category="Etheria|Save|Events") FOnGameSaved                OnGameSaved;
	UPROPERTY(BlueprintAssignable, Category="Etheria|Save|Events") FOnPreLoad                  OnPreLoad;
	UPROPERTY(BlueprintAssignable, Category="Etheria|Save|Events") FOnGameLoaded               OnGameLoaded;
	UPROPERTY(BlueprintAssignable, Category="Etheria|Save|Events") FOnSettingsChanged          OnSettingsChanged;
	UPROPERTY(BlueprintAssignable, Category="Etheria|Save|Events") FOnNewGameStarted           OnNewGameStarted;
	UPROPERTY(BlueprintAssignable, Category="Etheria|Save|Events") FOnTutorialCompletedDispatch OnTutorialCompleted;
	UPROPERTY(BlueprintAssignable, Category="Etheria|Save|Events") FOnEraChanged               OnEraChanged;
	UPROPERTY(BlueprintAssignable, Category="Etheria|Save|Events") FOnProfileLoaded            OnProfileLoaded;

	/* ═══════════ Configuration ═══════════ */

	/** Slot name used for the player profile. Don't change at runtime. */
	static const FString PROFILE_SLOT_NAME;

	/** Slot name used for the tutorial checkpoint. */
	static const FString TUTORIAL_SLOT_NAME;

	/** Default user index for all save operations (single-player = 0). */
	static constexpr int32 DEFAULT_USER_INDEX = 0;

	/** Number of manual slots available (3 by design). */
	static constexpr int32 MANUAL_SLOT_COUNT = 3;

private:
	/* ═══════════ Internal State ═══════════ */

	UPROPERTY() TObjectPtr<UEtheriaPlayerProfileSave>  PlayerProfile;
	UPROPERTY() TObjectPtr<UEtheriaGameProgressSave>   CurrentProgress;
	UPROPERTY() TObjectPtr<UEtheriaTutorialSave>       CurrentTutorial;

	UPROPERTY() TArray<TWeakObjectPtr<UObject>> SaveableObjects;

	/* ═══════════ Internal Helpers ═══════════ */

	void LoadOrCreateProfile();
	void ApplyAudioSettingsInternal(const FEtheriaAudioSettings& Settings);
	void ApplyGraphicsSettingsInternal(const FGraphicsSettings& Settings);

	UEtheriaGameProgressSave* CreateFreshProgress() const;
	FString BuildSlotName(ESaveSlotType SlotType, int32 SlotIndex) const;

	void BroadcastSaveablePreSave();
	void BroadcastSaveablePostLoad();
	void PruneStaleSaveables();

	/** Async save callbacks. */
	void HandleProgressSaved(const FString& SlotName, const int32 UserIndex, bool bSuccess);
	void HandleProgressLoaded(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGame);
	void HandleProfileSaved(const FString& SlotName, const int32 UserIndex, bool bSuccess);

	FSaveSlotInfo BuildSlotInfoFromSave(const UEtheriaGameProgressSave* Save, ESaveSlotType SlotType, int32 SlotIndex) const;
};
