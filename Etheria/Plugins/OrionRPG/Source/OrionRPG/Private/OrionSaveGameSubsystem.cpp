// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "OrionSaveGameSubsystem.h"
#include "Engine/GameInstance.h"
#include "OrionSaveGame.h"
#include "OrionSetting.h"
#include "Engine/World.h"
#include "QuestComponent.h"
#include "QuestBuilderFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "HAL/IConsoleManager.h"

UOrionSaveGameSubsystem::UOrionSaveGameSubsystem()
{
	bIsLoadingSaveGame = false;
	bIsSavingGame = false;
	CurrentSaveSlot = 0;
}

void UOrionSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	OrionSetting = GetMutableDefault<UOrionSetting>();
	SaveGameSlots.SetNum(OrionSetting->SaveSlotCapacity);
	SaveGameClass = OrionSetting->SaveGameClass;
	UGameInstance* GI = GetGameInstance();
	//Delegates
	{
		FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UOrionSaveGameSubsystem::HandlePreLoadMap);
		FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UOrionSaveGameSubsystem::HandlePostLoadMap);
		GI->OnLocalPlayerAddedEvent.AddUObject(this, &UOrionSaveGameSubsystem::InitializeLocalPlayerSaveGameSlots);
	}

	//Register Console Commands
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("orionSaveGame"),
		TEXT("Slot[int] Save game on specific save "),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::orionSaveGame),
		ECVF_Default);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("orionLoadGame"),
		TEXT("Slot[int] Load game on specific save"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ThisClass::orionLoadGame),
		ECVF_Default);

}


void UOrionSaveGameSubsystem::InitializeLocalPlayerSaveGameSlots(ULocalPlayer* LocalPlayer)
{
	check(LocalPlayer);

	for (int32 i = 0; i < SaveGameSlots.Num(); ++i)
	{
		const FString SaveName = Platform_CreateSaveGameName(LocalPlayer, i);
		UE_LOG(LogTemp, Log, TEXT("OrionSaveSubsystem: Initialize save game slot (Name: %s, Slot: %i)"), *SaveName, i);
		SaveGameSlots[i] = Cast<UOrionSaveGame>(ULocalPlayerSaveGame::LoadOrCreateSaveGameForLocalPlayer(SaveGameClass, LocalPlayer, SaveName));
	}
}

void UOrionSaveGameSubsystem::HandlePreLoadMap(const FString& MapURL)
{
	UE_LOG(LogTemp, Log, TEXT("OrionSaveSubsystem: HandlePreLoadMap (MapURL: %s)"), *MapURL);
}

void UOrionSaveGameSubsystem::HandlePostLoadMap(UWorld* World)
{
	FString CurrentMapName = World->GetMapName();
	CurrentMapName.RemoveFromStart(World->StreamingLevelsPrefix);
	UE_LOG(LogTemp, Log, TEXT("OrionSaveSubsystem: HandlePostLoadMap [%s]"), *CurrentMapName);

	APlayerController* PC = World->GetFirstPlayerController();
	

	if (OrionSetting->bSaveLevel && bIsLoadingSaveGame)
	{
		if (UOrionSaveGame* SaveGame = GetCurrentSaveGameObject())
		{
			SaveGame->HandleLoadGame(World, bIsLoadingSaveGame);
		}

		bIsLoadingSaveGame = false;
	}
}

FString UOrionSaveGameSubsystem::Platform_CreateSaveGameName(ULocalPlayer* LocalPlayer, int32 SaveSlotIndex) const
{
	const FString SaveGameName = FString::Printf(TEXT("save_%i"), SaveSlotIndex);

#if (UE_BUILD_SHIPPING && PLATFORM_WINDOWS)

	check(LocalPlayer);
	const FUniqueNetIdRepl UniqueNetId = LocalPlayer->GetUniqueNetIdForPlatformUser();
	check(UniqueNetId.IsValid());
	return FString::Printf(TEXT("%s/%s"), *UniqueNetId->ToString(), *SaveGameName);

#else
	return SaveGameName;

#endif // UE_BUILD_SHIPPING
}

void UOrionSaveGameSubsystem::DeleteSaveGame(int32 Slot)
{
	if (Slot < 0 || Slot >= SaveGameSlots.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Fail to delete save game at slot (%i). Invalid save slot!"), Slot);
		return;
	}

	UGameInstance* GI = GetGameInstance();
	ULocalPlayer* LocalPlayer = GI ? GI->GetLocalPlayerByIndex(0) : nullptr;
	check(LocalPlayer);

	CurrentSaveSlot = Slot;
	const FString SaveName = Platform_CreateSaveGameName(LocalPlayer, CurrentSaveSlot);
	UE_LOG(LogTemp, Log, TEXT("Delete and create new save game (SaveSlot: %i, SaveName: %s"), CurrentSaveSlot, *SaveName);
	SaveGameSlots[CurrentSaveSlot] = Cast<UOrionSaveGame>(ULocalPlayerSaveGame::CreateNewSaveGameForLocalPlayer(SaveGameClass, LocalPlayer, SaveName));
}

void UOrionSaveGameSubsystem::SaveGame(int32 Slot)
{
	if (Slot < 0 || Slot >= SaveGameSlots.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("OrionSaveSubsystem: Fail to save game from slot (%i). Invalid save slot!"), Slot);
		return;
	}

	CurrentSaveSlot = Slot;
	TObjectPtr<UOrionSaveGame> SaveGameObject = SaveGameSlots[CurrentSaveSlot];

	if (!SaveGameObject) return;

	if (SaveGameObject->IsSavingGame())
	{
		UE_LOG(LogTemp, Error, TEXT("OrionSaveSubsystem: Fail to save game, Save game in progress on slot (%i)."), Slot);
		return;
	}
	

	UE_LOG(LogTemp, Log, TEXT("OrionSaveSubsystem: Save game (SaveSlot: %i, SaveName: %s)"), CurrentSaveSlot, *SaveGameObject->GetSaveSlotName());
	SaveGameObject->AsyncSaveGameToSlotForLocalPlayer();
}

void UOrionSaveGameSubsystem::LoadGame(int32 Slot)
{
	if (bIsLoadingSaveGame) return; // already loading a save game

	if (Slot < 0 || Slot >= SaveGameSlots.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("OrionSaveSubsystem: Fail to load game from slot (%i). Invalid save slot!"), Slot);
		return;
	}

	UGameInstance* GI = GetGameInstance();
	check(GI);
	ULocalPlayer* LocalPlayer = GI->GetLocalPlayerByIndex(0);
	check(LocalPlayer);

	CurrentSaveSlot = Slot;


	UOrionSaveGame* SaveGameObject = SaveGameSlots[CurrentSaveSlot];
	check(SaveGameObject);


	if (SaveGameObject->GetSavedDataVersion() == SaveGameObject->GetInvalidDataVersion()) // no need to load anything if save data version is invalid | newly created | being reset
	{
		UE_LOG(LogTemp, Warning, TEXT("OrionSaveSubsystem: Abort load game from slot (%i). save data version invalid"), Slot);
		return;
	}

	if (OrionSetting->bSaveLevel)
	{
		const FOrionPlayInfoSaveData& PlayInfo = SaveGameObject->PlayInfoData;

		TravelURL = FString::Printf(TEXT("%s"), *PlayInfo.MapURL);

		UE_LOG(LogTemp, Log, TEXT("OrionSaveSubsystem: Load game (SaveSlot: %i, SaveName: %s, TravelURL: %s)"), CurrentSaveSlot, *SaveGameObject->GetSaveSlotName(), *TravelURL);

		bIsLoadingSaveGame = true;
		GetWorld()->ServerTravel(TravelURL, true);
	}
	else
	{
		bIsLoadingSaveGame = true;
		SaveGameObject->HandleLoadGame(GetWorld(), true);
		bIsLoadingSaveGame = false;
	}
}

// ================================================================================================ //
// CONSOLE COMMANDS
// ================================================================================================ //
void UOrionSaveGameSubsystem::orionSaveGame(const TArray<FString>& Slot)
{
	if (Slot[0].IsEmpty()) return;
	SaveGame(FCString::Atoi(*Slot[0]));
	
}

void UOrionSaveGameSubsystem::orionLoadGame(const TArray<FString>& Slot)
{
	if (Slot[0].IsEmpty()) return;
	LoadGame(FCString::Atoi(*Slot[0]));
}
