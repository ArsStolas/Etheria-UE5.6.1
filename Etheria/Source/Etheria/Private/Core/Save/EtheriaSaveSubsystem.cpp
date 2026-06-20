/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "EtheriaSaveSubsystem - Source"
 * Notes: Implements the central save/load dispatcher.
 *        Async save/load via UGameplayStatics::Async* APIs (no frame hitches).
 *        Custom typed maps power the Set Save X / Get Save X BP nodes.
 *        CustomThunk implementations of SetSaveStruct / GetSaveStruct serialize wildcard
 *        BP structs to/from FSaveBlob byte arrays via FObjectAndNameAsStringProxyArchive.
 */

#include "Core/Save/EtheriaSaveSubsystem.h"

#include "Core/Save/EtheriaPlayerProfileSave.h"
#include "Core/Save/EtheriaGameProgressSave.h"
#include "Core/Save/EtheriaTutorialSave.h"
#include "Core/Save/SaveableInterface.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Engine/GameInstance.h"

const FString UEtheriaSaveSubsystem::PROFILE_SLOT_NAME  = TEXT("PlayerProfile");
const FString UEtheriaSaveSubsystem::TUTORIAL_SLOT_NAME = TEXT("TutorialCheckpoint");

/* ═══════════ Subsystem Lifecycle ═══════════ */

void UEtheriaSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadOrCreateProfile();

	// Apply settings to engine ASAP so the boot sequence already respects user prefs.
	if (PlayerProfile)
	{
		ApplyAudioSettingsInternal(PlayerProfile->AudioSettings);
		ApplyGraphicsSettingsInternal(PlayerProfile->GraphicsSettings);
	}
}

void UEtheriaSaveSubsystem::Deinitialize()
{
	// Best-effort flush of profile (timestamps, accumulated playtime).
	if (PlayerProfile)
	{
		PlayerProfile->LastModified = FDateTime::UtcNow();
		UGameplayStatics::SaveGameToSlot(PlayerProfile, PROFILE_SLOT_NAME, DEFAULT_USER_INDEX);
	}

	SaveableObjects.Empty();
	Super::Deinitialize();
}

UEtheriaSaveSubsystem* UEtheriaSaveSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World) return nullptr;
	if (UGameInstance* GI = World->GetGameInstance())
		return GI->GetSubsystem<UEtheriaSaveSubsystem>();
	return nullptr;
}

/* ═══════════ Profile API ═══════════ */

void UEtheriaSaveSubsystem::LoadOrCreateProfile()
{
	if (UGameplayStatics::DoesSaveGameExist(PROFILE_SLOT_NAME, DEFAULT_USER_INDEX))
	{
		// Sync load — profile is small and we need it immediately at boot.
		USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(PROFILE_SLOT_NAME, DEFAULT_USER_INDEX);
		PlayerProfile = Cast<UEtheriaPlayerProfileSave>(Loaded);
	}

	if (!PlayerProfile)
	{
		PlayerProfile = Cast<UEtheriaPlayerProfileSave>(
			UGameplayStatics::CreateSaveGameObject(UEtheriaPlayerProfileSave::StaticClass()));
	}

	OnProfileLoaded.Broadcast(PlayerProfile);
}

bool UEtheriaSaveSubsystem::IsFirstLaunch() const
{
	return PlayerProfile ? !PlayerProfile->bHasPlayedBefore : true;
}

void UEtheriaSaveSubsystem::MarkAsLaunched()
{
	if (!PlayerProfile || PlayerProfile->bHasPlayedBefore) return;
	PlayerProfile->bHasPlayedBefore = true;
	SaveProfile();
}

bool UEtheriaSaveSubsystem::IsTutorialCompleted() const
{
	return PlayerProfile ? PlayerProfile->bTutorialCompleted : false;
}

void UEtheriaSaveSubsystem::MarkTutorialCompleted()
{
	if (!PlayerProfile) return;
	if (!PlayerProfile->bTutorialCompleted)
	{
		PlayerProfile->bTutorialCompleted = true;
		SaveProfile();
	}
	ClearTutorialCheckpoint();
	OnTutorialCompleted.Broadcast();
}

float UEtheriaSaveSubsystem::GetTotalPlayTime() const
{
	return PlayerProfile ? PlayerProfile->TotalPlayTime : 0.f;
}

void UEtheriaSaveSubsystem::SaveProfile()
{
	if (!PlayerProfile) return;
	PlayerProfile->LastModified = FDateTime::UtcNow();

	FAsyncSaveGameToSlotDelegate Done;
	Done.BindUObject(this, &UEtheriaSaveSubsystem::HandleProfileSaved);
	UGameplayStatics::AsyncSaveGameToSlot(PlayerProfile, PROFILE_SLOT_NAME, DEFAULT_USER_INDEX, Done);
}

void UEtheriaSaveSubsystem::HandleProfileSaved(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	// Silent — no dispatcher for profile saves (it's an implementation detail).
}

/* ═══════════ Settings ═══════════ */

FEtheriaAudioSettings    UEtheriaSaveSubsystem::GetAudioSettings()    const { return PlayerProfile ? PlayerProfile->AudioSettings    : FEtheriaAudioSettings(); }
FGraphicsSettings UEtheriaSaveSubsystem::GetGraphicsSettings() const { return PlayerProfile ? PlayerProfile->GraphicsSettings : FGraphicsSettings(); }
FKeybindSettings  UEtheriaSaveSubsystem::GetKeybindSettings()  const { return PlayerProfile ? PlayerProfile->KeybindSettings  : FKeybindSettings(); }

void UEtheriaSaveSubsystem::ApplyAudioSettings(const FEtheriaAudioSettings& NewSettings)
{
	if (!PlayerProfile) return;
	PlayerProfile->AudioSettings = NewSettings;
	ApplyAudioSettingsInternal(NewSettings);
	SaveProfile();
	OnSettingsChanged.Broadcast();
}

void UEtheriaSaveSubsystem::ApplyGraphicsSettings(const FGraphicsSettings& NewSettings)
{
	if (!PlayerProfile) return;
	PlayerProfile->GraphicsSettings = NewSettings;
	ApplyGraphicsSettingsInternal(NewSettings);
	SaveProfile();
	OnSettingsChanged.Broadcast();
}

void UEtheriaSaveSubsystem::ApplyKeybindSettings(const FKeybindSettings& NewSettings)
{
	if (!PlayerProfile) return;
	PlayerProfile->KeybindSettings = NewSettings;
	SaveProfile();
	OnSettingsChanged.Broadcast();
}

void UEtheriaSaveSubsystem::ResetSettingsToDefaults()
{
	if (!PlayerProfile) return;
	PlayerProfile->AudioSettings    = FEtheriaAudioSettings();
	PlayerProfile->GraphicsSettings = FGraphicsSettings();
	PlayerProfile->KeybindSettings  = FKeybindSettings();

	ApplyAudioSettingsInternal(PlayerProfile->AudioSettings);
	ApplyGraphicsSettingsInternal(PlayerProfile->GraphicsSettings);
	SaveProfile();
	OnSettingsChanged.Broadcast();
}

void UEtheriaSaveSubsystem::ApplyAudioSettingsInternal(const FEtheriaAudioSettings& Settings)
{
	// Audio is applied by listening to OnSettingsChanged in BP and pushing to your Sound Classes.
	// We don't hard-code Sound Class refs here so the subsystem stays asset-agnostic.
}

void UEtheriaSaveSubsystem::ApplyGraphicsSettingsInternal(const FGraphicsSettings& Settings)
{
	UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!UserSettings) return;

	UserSettings->SetOverallScalabilityLevel(Settings.OverallQuality);

	// Per-group overrides — only applied if the user explicitly set a value (not -1).
	if (Settings.ViewDistanceQuality >= 0) UserSettings->SetViewDistanceQuality(Settings.ViewDistanceQuality);
	if (Settings.ShadowQuality       >= 0) UserSettings->SetShadowQuality(Settings.ShadowQuality);
	if (Settings.PostProcessQuality  >= 0) UserSettings->SetPostProcessingQuality(Settings.PostProcessQuality);
	if (Settings.TextureQuality      >= 0) UserSettings->SetTextureQuality(Settings.TextureQuality);
	if (Settings.EffectsQuality      >= 0) UserSettings->SetVisualEffectQuality(Settings.EffectsQuality);
	if (Settings.FoliageQuality      >= 0) UserSettings->SetFoliageQuality(Settings.FoliageQuality);

	UserSettings->SetScreenResolution(Settings.Resolution);
	UserSettings->SetFullscreenMode(Settings.bFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed);
	UserSettings->SetVSyncEnabled(Settings.bVSync);
	UserSettings->SetFrameRateLimit(static_cast<float>(Settings.FrameRateLimit));

	UserSettings->ApplySettings(false);
}

/* ═══════════ Game Progress — Save / Load ═══════════ */

UEtheriaGameProgressSave* UEtheriaSaveSubsystem::CreateFreshProgress() const
{
	return Cast<UEtheriaGameProgressSave>(
		UGameplayStatics::CreateSaveGameObject(UEtheriaGameProgressSave::StaticClass()));
}

FString UEtheriaSaveSubsystem::BuildSlotName(ESaveSlotType SlotType, int32 SlotIndex) const
{
	if (SlotType == ESaveSlotType::Auto) return TEXT("AutoSave");
	const int32 Idx = FMath::Clamp(SlotIndex, 0, MANUAL_SLOT_COUNT - 1);
	return FString::Printf(TEXT("Manual_%d"), Idx);
}

void UEtheriaSaveSubsystem::SaveProgress(ESaveSlotType SlotType, int32 SlotIndex)
{
	if (!CurrentProgress) CurrentProgress = CreateFreshProgress();
	if (!CurrentProgress) return;

	OnPreSave.Broadcast(SlotType);
	BroadcastSaveablePreSave();

	// Stamp metadata
	CurrentProgress->SlotType      = SlotType;
	CurrentProgress->SlotIndex     = (SlotType == ESaveSlotType::Auto) ? 0 : FMath::Clamp(SlotIndex, 0, MANUAL_SLOT_COUNT - 1);
	CurrentProgress->SaveTimestamp = FDateTime::UtcNow();

	const FString SlotName = BuildSlotName(SlotType, SlotIndex);

	FAsyncSaveGameToSlotDelegate Done;
	Done.BindUObject(this, &UEtheriaSaveSubsystem::HandleProgressSaved);
	UGameplayStatics::AsyncSaveGameToSlot(CurrentProgress, SlotName, DEFAULT_USER_INDEX, Done);
}

bool UEtheriaSaveSubsystem::SaveProgressSync(ESaveSlotType SlotType, int32 SlotIndex)
{
	if (!CurrentProgress) CurrentProgress = CreateFreshProgress();
	if (!CurrentProgress) return false;

	OnPreSave.Broadcast(SlotType);
	BroadcastSaveablePreSave();

	CurrentProgress->SlotType      = SlotType;
	CurrentProgress->SlotIndex     = (SlotType == ESaveSlotType::Auto) ? 0 : FMath::Clamp(SlotIndex, 0, MANUAL_SLOT_COUNT - 1);
	CurrentProgress->SaveTimestamp = FDateTime::UtcNow();

	const FString SlotName = BuildSlotName(SlotType, SlotIndex);
	const bool bOk = UGameplayStatics::SaveGameToSlot(CurrentProgress, SlotName, DEFAULT_USER_INDEX);

	FSaveSlotInfo Info = CurrentProgress->BuildSlotInfo();
	Info.SlotName = SlotName;
	OnGameSaved.Broadcast(Info, bOk ? ESaveOpResult::Success : ESaveOpResult::Failed);
	return bOk;
}

void UEtheriaSaveSubsystem::HandleProgressSaved(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	FSaveSlotInfo Info;
	if (CurrentProgress)
	{
		Info = CurrentProgress->BuildSlotInfo();
	}
	Info.SlotName = SlotName;
	Info.bExists  = bSuccess;
	OnGameSaved.Broadcast(Info, bSuccess ? ESaveOpResult::Success : ESaveOpResult::Failed);
}

void UEtheriaSaveSubsystem::LoadProgress(ESaveSlotType SlotType, int32 SlotIndex)
{
	const FString SlotName = BuildSlotName(SlotType, SlotIndex);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, DEFAULT_USER_INDEX))
	{
		FSaveSlotInfo Info;
		Info.SlotName  = SlotName;
		Info.SlotType  = SlotType;
		Info.SlotIndex = SlotIndex;
		OnGameLoaded.Broadcast(Info, ESaveOpResult::NotFound);
		return;
	}

	OnPreLoad.Broadcast(SlotType);

	FAsyncLoadGameFromSlotDelegate Done;
	Done.BindUObject(this, &UEtheriaSaveSubsystem::HandleProgressLoaded);
	UGameplayStatics::AsyncLoadGameFromSlot(SlotName, DEFAULT_USER_INDEX, Done);
}

void UEtheriaSaveSubsystem::HandleProgressLoaded(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGame)
{
	UEtheriaGameProgressSave* AsProgress = Cast<UEtheriaGameProgressSave>(LoadedGame);
	if (!AsProgress)
	{
		FSaveSlotInfo Info;
		Info.SlotName = SlotName;
		OnGameLoaded.Broadcast(Info, ESaveOpResult::Corrupted);
		return;
	}

	CurrentProgress = AsProgress;

	FSaveSlotInfo Info = CurrentProgress->BuildSlotInfo();
	Info.SlotName = SlotName;

	OnGameLoaded.Broadcast(Info, ESaveOpResult::Success);
	BroadcastSaveablePostLoad();
}

void UEtheriaSaveSubsystem::LoadMostRecentProgress()
{
	const FSaveSlotInfo Recent = GetMostRecentSlotInfo();
	if (!Recent.bExists)
	{
		FSaveSlotInfo Empty;
		OnGameLoaded.Broadcast(Empty, ESaveOpResult::NotFound);
		return;
	}
	LoadProgress(Recent.SlotType, Recent.SlotIndex);
}

FSaveSlotInfo UEtheriaSaveSubsystem::GetMostRecentSlotInfo()
{
	FSaveSlotInfo Best;
	Best.bExists = false;
	FDateTime BestTime = FDateTime::MinValue();

	const TArray<FSaveSlotInfo> All = GetAllSlotsInfo();
	for (const FSaveSlotInfo& Info : All)
	{
		if (Info.bExists && Info.Timestamp > BestTime)
		{
			BestTime = Info.Timestamp;
			Best = Info;
		}
	}
	return Best;
}

FSaveSlotInfo UEtheriaSaveSubsystem::GetSlotInfo(ESaveSlotType SlotType, int32 SlotIndex)
{
	FSaveSlotInfo Info;
	Info.SlotType  = SlotType;
	Info.SlotIndex = (SlotType == ESaveSlotType::Auto) ? 0 : FMath::Clamp(SlotIndex, 0, MANUAL_SLOT_COUNT - 1);
	Info.SlotName  = BuildSlotName(SlotType, Info.SlotIndex);
	Info.bExists   = false;

	if (!UGameplayStatics::DoesSaveGameExist(Info.SlotName, DEFAULT_USER_INDEX)) return Info;

	// Sync read for metadata. This is OK — it only happens when the user opens the load menu.
	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(Info.SlotName, DEFAULT_USER_INDEX);
	if (UEtheriaGameProgressSave* AsProgress = Cast<UEtheriaGameProgressSave>(Loaded))
	{
		Info = AsProgress->BuildSlotInfo();
		Info.SlotName = BuildSlotName(SlotType, Info.SlotIndex);
	}
	return Info;
}

TArray<FSaveSlotInfo> UEtheriaSaveSubsystem::GetAllSlotsInfo()
{
	TArray<FSaveSlotInfo> Out;
	Out.Reserve(1 + MANUAL_SLOT_COUNT);
	Out.Add(GetSlotInfo(ESaveSlotType::Auto, 0));
	for (int32 i = 0; i < MANUAL_SLOT_COUNT; ++i)
		Out.Add(GetSlotInfo(ESaveSlotType::Manual, i));
	return Out;
}

bool UEtheriaSaveSubsystem::DoesSlotExist(ESaveSlotType SlotType, int32 SlotIndex) const
{
	return UGameplayStatics::DoesSaveGameExist(BuildSlotName(SlotType, SlotIndex), DEFAULT_USER_INDEX);
}

bool UEtheriaSaveSubsystem::HasAnySave() const
{
	if (DoesSlotExist(ESaveSlotType::Auto, 0)) return true;
	for (int32 i = 0; i < MANUAL_SLOT_COUNT; ++i)
		if (DoesSlotExist(ESaveSlotType::Manual, i)) return true;
	return false;
}

bool UEtheriaSaveSubsystem::DeleteSlot(ESaveSlotType SlotType, int32 SlotIndex)
{
	return UGameplayStatics::DeleteGameInSlot(BuildSlotName(SlotType, SlotIndex), DEFAULT_USER_INDEX);
}

void UEtheriaSaveSubsystem::DeleteAllSlots()
{
	DeleteSlot(ESaveSlotType::Auto, 0);
	for (int32 i = 0; i < MANUAL_SLOT_COUNT; ++i) DeleteSlot(ESaveSlotType::Manual, i);
}

void UEtheriaSaveSubsystem::StartNewGame(EGameEra StartingEra)
{
	CurrentProgress = CreateFreshProgress();
	if (CurrentProgress)
	{
		CurrentProgress->Era = StartingEra;
		CurrentProgress->SaveTimestamp = FDateTime::UtcNow();
	}
	OnNewGameStarted.Broadcast();
}

/* ═══════════ Player Position ═══════════ */

void UEtheriaSaveSubsystem::RecordPlayerCheckpoint(const FTransform& Where, FName CampfireID, const FString& InLocationLabel)
{
	if (!CurrentProgress) CurrentProgress = CreateFreshProgress();
	if (!CurrentProgress) return;
	CurrentProgress->PlayerTransform = Where;
	CurrentProgress->LastCampfireID  = CampfireID;
	CurrentProgress->LocationLabel   = InLocationLabel;
}

/* ═══════════ Era ═══════════ */

EGameEra UEtheriaSaveSubsystem::GetCurrentEra() const
{
	return CurrentProgress ? CurrentProgress->Era : EGameEra::Present;
}

void UEtheriaSaveSubsystem::SetCurrentEra(EGameEra NewEra)
{
	if (!CurrentProgress) CurrentProgress = CreateFreshProgress();
	if (!CurrentProgress) return;
	const EGameEra Old = CurrentProgress->Era;
	if (Old == NewEra) return;
	CurrentProgress->Era = NewEra;
	OnEraChanged.Broadcast(Old, NewEra);
}

/* ═══════════ Tutorial ═══════════ */

void UEtheriaSaveSubsystem::SaveTutorialCheckpoint(FName CheckpointID, const FTransform& PlayerTransform)
{
	if (!CurrentTutorial)
	{
		CurrentTutorial = Cast<UEtheriaTutorialSave>(
			UGameplayStatics::CreateSaveGameObject(UEtheriaTutorialSave::StaticClass()));
	}
	if (!CurrentTutorial) return;

	CurrentTutorial->CheckpointID    = CheckpointID;
	CurrentTutorial->PlayerTransform = PlayerTransform;
	CurrentTutorial->Timestamp       = FDateTime::UtcNow();

	UGameplayStatics::AsyncSaveGameToSlot(CurrentTutorial, TUTORIAL_SLOT_NAME, DEFAULT_USER_INDEX);
}

bool UEtheriaSaveSubsystem::LoadTutorialCheckpoint(UEtheriaTutorialSave*& OutSave)
{
	OutSave = nullptr;
	if (!UGameplayStatics::DoesSaveGameExist(TUTORIAL_SLOT_NAME, DEFAULT_USER_INDEX)) return false;

	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(TUTORIAL_SLOT_NAME, DEFAULT_USER_INDEX);
	CurrentTutorial = Cast<UEtheriaTutorialSave>(Loaded);
	OutSave = CurrentTutorial;
	return CurrentTutorial != nullptr;
}

void UEtheriaSaveSubsystem::MarkTutorialStepCompleted(FName StepID)
{
	if (!CurrentTutorial)
	{
		CurrentTutorial = Cast<UEtheriaTutorialSave>(
			UGameplayStatics::CreateSaveGameObject(UEtheriaTutorialSave::StaticClass()));
	}
	if (CurrentTutorial) CurrentTutorial->CompletedSteps.Add(StepID, true);
}

bool UEtheriaSaveSubsystem::IsTutorialStepCompleted(FName StepID) const
{
	if (!CurrentTutorial) return false;
	const bool* Found = CurrentTutorial->CompletedSteps.Find(StepID);
	return Found && *Found;
}

void UEtheriaSaveSubsystem::ClearTutorialCheckpoint()
{
	UGameplayStatics::DeleteGameInSlot(TUTORIAL_SLOT_NAME, DEFAULT_USER_INDEX);
	CurrentTutorial = nullptr;
}

/* ═══════════ Custom Data — typed setters/getters ═══════════ */

#define ETHERIA_ENSURE_PROGRESS() \
	if (!CurrentProgress) CurrentProgress = CreateFreshProgress(); \
	if (!CurrentProgress) return;

#define ETHERIA_ENSURE_PROGRESS_RETURN(Default) \
	if (!CurrentProgress) return Default;

void  UEtheriaSaveSubsystem::SetSaveBool   (FName Key, bool   Value) { ETHERIA_ENSURE_PROGRESS(); CurrentProgress->BoolData.Add(Key, Value); }
bool  UEtheriaSaveSubsystem::GetSaveBool   (FName Key, bool   DefaultValue) const { ETHERIA_ENSURE_PROGRESS_RETURN(DefaultValue); const bool*  V = CurrentProgress->BoolData.Find(Key); return V ? *V : DefaultValue; }

void  UEtheriaSaveSubsystem::SetSaveInt    (FName Key, int32  Value) { ETHERIA_ENSURE_PROGRESS(); CurrentProgress->IntData.Add(Key, Value); }
int32 UEtheriaSaveSubsystem::GetSaveInt    (FName Key, int32  DefaultValue) const { ETHERIA_ENSURE_PROGRESS_RETURN(DefaultValue); const int32* V = CurrentProgress->IntData.Find(Key); return V ? *V : DefaultValue; }

void  UEtheriaSaveSubsystem::SetSaveFloat  (FName Key, float  Value) { ETHERIA_ENSURE_PROGRESS(); CurrentProgress->FloatData.Add(Key, Value); }
float UEtheriaSaveSubsystem::GetSaveFloat  (FName Key, float  DefaultValue) const { ETHERIA_ENSURE_PROGRESS_RETURN(DefaultValue); const float* V = CurrentProgress->FloatData.Find(Key); return V ? *V : DefaultValue; }

void    UEtheriaSaveSubsystem::SetSaveString (FName Key, const FString& Value) { ETHERIA_ENSURE_PROGRESS(); CurrentProgress->StringData.Add(Key, Value); }
FString UEtheriaSaveSubsystem::GetSaveString (FName Key, const FString& DefaultValue) const { ETHERIA_ENSURE_PROGRESS_RETURN(DefaultValue); const FString* V = CurrentProgress->StringData.Find(Key); return V ? *V : DefaultValue; }

void  UEtheriaSaveSubsystem::SetSaveName   (FName Key, FName  Value) { ETHERIA_ENSURE_PROGRESS(); CurrentProgress->NameData.Add(Key, Value); }
FName UEtheriaSaveSubsystem::GetSaveName   (FName Key, FName  DefaultValue) const { ETHERIA_ENSURE_PROGRESS_RETURN(DefaultValue); const FName* V = CurrentProgress->NameData.Find(Key); return V ? *V : DefaultValue; }

void    UEtheriaSaveSubsystem::SetSaveVector    (FName Key, FVector    Value) { ETHERIA_ENSURE_PROGRESS(); CurrentProgress->VectorData.Add(Key, Value); }
FVector UEtheriaSaveSubsystem::GetSaveVector    (FName Key, FVector    DefaultValue) const { ETHERIA_ENSURE_PROGRESS_RETURN(DefaultValue); const FVector* V = CurrentProgress->VectorData.Find(Key); return V ? *V : DefaultValue; }

void     UEtheriaSaveSubsystem::SetSaveRotator   (FName Key, FRotator   Value) { ETHERIA_ENSURE_PROGRESS(); CurrentProgress->RotatorData.Add(Key, Value); }
FRotator UEtheriaSaveSubsystem::GetSaveRotator   (FName Key, FRotator   DefaultValue) const { ETHERIA_ENSURE_PROGRESS_RETURN(DefaultValue); const FRotator* V = CurrentProgress->RotatorData.Find(Key); return V ? *V : DefaultValue; }

void       UEtheriaSaveSubsystem::SetSaveTransform (FName Key, FTransform Value) { ETHERIA_ENSURE_PROGRESS(); CurrentProgress->TransformData.Add(Key, Value); }
FTransform UEtheriaSaveSubsystem::GetSaveTransform (FName Key, FTransform DefaultValue) const { ETHERIA_ENSURE_PROGRESS_RETURN(DefaultValue); const FTransform* V = CurrentProgress->TransformData.Find(Key); return V ? *V : DefaultValue; }

void UEtheriaSaveSubsystem::SetSaveObject(FName Key, UObject* Value)
{
	ETHERIA_ENSURE_PROGRESS();
	CurrentProgress->ObjectPathData.Add(Key, Value ? FSoftObjectPath(Value) : FSoftObjectPath());
}

UObject* UEtheriaSaveSubsystem::GetSaveObject(FName Key) const
{
	ETHERIA_ENSURE_PROGRESS_RETURN(nullptr);
	const FSoftObjectPath* Path = CurrentProgress->ObjectPathData.Find(Key);
	return (Path && Path->IsValid()) ? Path->TryLoad() : nullptr;
}

void UEtheriaSaveSubsystem::SetSaveClass(FName Key, UClass* Value)
{
	ETHERIA_ENSURE_PROGRESS();
	CurrentProgress->ClassPathData.Add(Key, Value ? FSoftClassPath(Value) : FSoftClassPath());
}

UClass* UEtheriaSaveSubsystem::GetSaveClass(FName Key) const
{
	ETHERIA_ENSURE_PROGRESS_RETURN(nullptr);
	const FSoftClassPath* Path = CurrentProgress->ClassPathData.Find(Key);
	return (Path && Path->IsValid()) ? Path->TryLoadClass<UObject>() : nullptr;
}

void  UEtheriaSaveSubsystem::SetSaveGuid (FName Key, FGuid Value) { ETHERIA_ENSURE_PROGRESS(); CurrentProgress->GuidData.Add(Key, Value); }
FGuid UEtheriaSaveSubsystem::GetSaveGuid (FName Key, FGuid DefaultValue) const { ETHERIA_ENSURE_PROGRESS_RETURN(DefaultValue); const FGuid* V = CurrentProgress->GuidData.Find(Key); return V ? *V : DefaultValue; }

/* ═══════════ Custom Data — Wildcard struct (CustomThunk) ═══════════ */

void UEtheriaSaveSubsystem::SetSaveStruct(FName Key, const int32& Value)
{
	// CustomThunk — never called directly. The real impl is execSetSaveStruct below.
	checkNoEntry();
}

DEFINE_FUNCTION(UEtheriaSaveSubsystem::execSetSaveStruct)
{
	P_GET_PROPERTY(FNameProperty, Key);

	// Step over the wildcard pin and grab the actual struct property + address.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentProperty        = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	void* StructAddr = Stack.MostRecentPropertyAddress;
	FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);

	P_FINISH;

	if (!StructProp || !StructAddr || !P_THIS) return;

	P_NATIVE_BEGIN;
	if (!P_THIS->CurrentProgress) P_THIS->CurrentProgress = P_THIS->CreateFreshProgress();
	if (!P_THIS->CurrentProgress) return;

	FSaveBlob Blob;
	FMemoryWriter MemWriter(Blob.Bytes, /*bIsPersistent=*/true);
	FObjectAndNameAsStringProxyArchive Ar(MemWriter, /*bLoadIfFindFails=*/false);
	Ar.ArIsSaveGame = true;
	StructProp->Struct->SerializeBin(Ar, StructAddr);

	P_THIS->CurrentProgress->BlobData.Add(Key, Blob);
	P_NATIVE_END;
}

bool UEtheriaSaveSubsystem::GetSaveStruct(FName Key, int32& OutValue)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UEtheriaSaveSubsystem::execGetSaveStruct)
{
	P_GET_PROPERTY(FNameProperty, Key);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentProperty        = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	void* OutAddr = Stack.MostRecentPropertyAddress;
	FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);

	P_FINISH;

	bool bResult = false;
	if (StructProp && OutAddr && P_THIS && P_THIS->CurrentProgress)
	{
		P_NATIVE_BEGIN;
		if (const FSaveBlob* Blob = P_THIS->CurrentProgress->BlobData.Find(Key))
		{
			FMemoryReader MemReader(Blob->Bytes, /*bIsPersistent=*/true);
			FObjectAndNameAsStringProxyArchive Ar(MemReader, /*bLoadIfFindFails=*/true);
			Ar.ArIsSaveGame = true;
			StructProp->Struct->SerializeBin(Ar, OutAddr);
			bResult = true;
		}
		P_NATIVE_END;
	}

	*static_cast<bool*>(RESULT_PARAM) = bResult;
}

/* ═══════════ Custom Data — Bytes ═══════════ */

void UEtheriaSaveSubsystem::SetSaveBytes(FName Key, const TArray<uint8>& Bytes)
{
	ETHERIA_ENSURE_PROGRESS();
	FSaveBlob Blob;
	Blob.Bytes = Bytes;
	CurrentProgress->BlobData.Add(Key, Blob);
}

bool UEtheriaSaveSubsystem::GetSaveBytes(FName Key, TArray<uint8>& OutBytes) const
{
	ETHERIA_ENSURE_PROGRESS_RETURN(false);
	if (const FSaveBlob* Blob = CurrentProgress->BlobData.Find(Key))
	{
		OutBytes = Blob->Bytes;
		return true;
	}
	return false;
}

/* ═══════════ Custom Data — Utility ═══════════ */

bool UEtheriaSaveSubsystem::HasSaveKey(FName Key) const
{
	ETHERIA_ENSURE_PROGRESS_RETURN(false);
	return CurrentProgress->BoolData.Contains(Key)
		|| CurrentProgress->IntData.Contains(Key)
		|| CurrentProgress->FloatData.Contains(Key)
		|| CurrentProgress->StringData.Contains(Key)
		|| CurrentProgress->NameData.Contains(Key)
		|| CurrentProgress->VectorData.Contains(Key)
		|| CurrentProgress->RotatorData.Contains(Key)
		|| CurrentProgress->TransformData.Contains(Key)
		|| CurrentProgress->ObjectPathData.Contains(Key)
		|| CurrentProgress->ClassPathData.Contains(Key)
		|| CurrentProgress->GuidData.Contains(Key)
		|| CurrentProgress->BlobData.Contains(Key);
}

void UEtheriaSaveSubsystem::ClearSaveKey(FName Key)
{
	ETHERIA_ENSURE_PROGRESS();
	CurrentProgress->BoolData.Remove(Key);
	CurrentProgress->IntData.Remove(Key);
	CurrentProgress->FloatData.Remove(Key);
	CurrentProgress->StringData.Remove(Key);
	CurrentProgress->NameData.Remove(Key);
	CurrentProgress->VectorData.Remove(Key);
	CurrentProgress->RotatorData.Remove(Key);
	CurrentProgress->TransformData.Remove(Key);
	CurrentProgress->ObjectPathData.Remove(Key);
	CurrentProgress->ClassPathData.Remove(Key);
	CurrentProgress->GuidData.Remove(Key);
	CurrentProgress->BlobData.Remove(Key);
}

void UEtheriaSaveSubsystem::ClearAllSaveData()
{
	if (CurrentProgress) CurrentProgress->ClearAllData();
}

/* ═══════════ Profile-Wide Custom Data ═══════════ */

void UEtheriaSaveSubsystem::SetProfileBool   (FName Key, bool Value)            { if (PlayerProfile) { PlayerProfile->BoolData.Add(Key, Value);   SaveProfile(); } }
bool UEtheriaSaveSubsystem::GetProfileBool   (FName Key, bool DefaultValue) const { if (!PlayerProfile) return DefaultValue; const bool*  V = PlayerProfile->BoolData.Find(Key);   return V ? *V : DefaultValue; }

void  UEtheriaSaveSubsystem::SetProfileInt   (FName Key, int32 Value)           { if (PlayerProfile) { PlayerProfile->IntData.Add(Key, Value);    SaveProfile(); } }
int32 UEtheriaSaveSubsystem::GetProfileInt   (FName Key, int32 DefaultValue) const { if (!PlayerProfile) return DefaultValue; const int32* V = PlayerProfile->IntData.Find(Key);    return V ? *V : DefaultValue; }

void  UEtheriaSaveSubsystem::SetProfileFloat (FName Key, float Value)           { if (PlayerProfile) { PlayerProfile->FloatData.Add(Key, Value);  SaveProfile(); } }
float UEtheriaSaveSubsystem::GetProfileFloat (FName Key, float DefaultValue) const { if (!PlayerProfile) return DefaultValue; const float* V = PlayerProfile->FloatData.Find(Key);  return V ? *V : DefaultValue; }

void    UEtheriaSaveSubsystem::SetProfileString(FName Key, const FString& Value)            { if (PlayerProfile) { PlayerProfile->StringData.Add(Key, Value); SaveProfile(); } }
FString UEtheriaSaveSubsystem::GetProfileString(FName Key, const FString& DefaultValue) const { if (!PlayerProfile) return DefaultValue; const FString* V = PlayerProfile->StringData.Find(Key); return V ? *V : DefaultValue; }

/* ═══════════ Saveable Registry ═══════════ */

void UEtheriaSaveSubsystem::RegisterSaveable(UObject* SaveableObject)
{
	if (!SaveableObject || !SaveableObject->Implements<USaveable>()) return;
	SaveableObjects.AddUnique(SaveableObject);
}

void UEtheriaSaveSubsystem::UnregisterSaveable(UObject* SaveableObject)
{
	if (!SaveableObject) return;
	SaveableObjects.RemoveAll([SaveableObject](const TWeakObjectPtr<UObject>& W) { return W.Get() == SaveableObject; });
}

void UEtheriaSaveSubsystem::PruneStaleSaveables()
{
	SaveableObjects.RemoveAll([](const TWeakObjectPtr<UObject>& W) { return !W.IsValid(); });
}

void UEtheriaSaveSubsystem::BroadcastSaveablePreSave()
{
	PruneStaleSaveables();
	for (const TWeakObjectPtr<UObject>& Weak : SaveableObjects)
	{
		if (UObject* Obj = Weak.Get())
		{
			if (Obj->Implements<USaveable>())
				ISaveable::Execute_OnSaveRequested(Obj);
		}
	}
}

void UEtheriaSaveSubsystem::BroadcastSaveablePostLoad()
{
	PruneStaleSaveables();
	for (const TWeakObjectPtr<UObject>& Weak : SaveableObjects)
	{
		if (UObject* Obj = Weak.Get())
		{
			if (Obj->Implements<USaveable>())
				ISaveable::Execute_OnLoadCompleted(Obj);
		}
	}
}

FSaveSlotInfo UEtheriaSaveSubsystem::BuildSlotInfoFromSave(const UEtheriaGameProgressSave* Save, ESaveSlotType SlotType, int32 SlotIndex) const
{
	FSaveSlotInfo Info;
	if (!Save) return Info;
	Info = Save->BuildSlotInfo();
	Info.SlotType  = SlotType;
	Info.SlotIndex = SlotIndex;
	return Info;
}

#undef ETHERIA_ENSURE_PROGRESS
#undef ETHERIA_ENSURE_PROGRESS_RETURN
