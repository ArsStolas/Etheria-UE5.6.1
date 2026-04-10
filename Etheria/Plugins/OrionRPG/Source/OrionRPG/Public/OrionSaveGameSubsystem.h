// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OrionSaveGame.h"
#include "Engine/LocalPlayer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OrionSaveGameSubsystem.generated.h"


DECLARE_DYNAMIC_DELEGATE_OneParam(FOrionSaveGameSignature, const bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOrionSaveGameMulticastSignature, const bool, bSuccess);

UCLASS()
class ORIONRPG_API UOrionSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()


public:
	UPROPERTY(EditDefaultsOnly, Category = "SaveSubsystem")
	TSubclassOf<UOrionSaveGame> SaveGameClass;

	UPROPERTY(EditDefaultsOnly, Category = "SaveSubsystem")
	TArray<TObjectPtr<UOrionSaveGame>> SaveGameSlots;

	UPROPERTY()
	int32 CurrentSaveSlot;

	UPROPERTY()
	TObjectPtr<UOrionSaveGame> CurrentSaveGame;


	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "SaveSubsystem")
	FOrionSaveGameMulticastSignature OnSaved;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "SaveSubsystem")
	FOrionSaveGameMulticastSignature OnLoaded;

private:
	FString TravelURL;
	bool bIsSavingGame;
	bool bIsLoadingSaveGame;
	TObjectPtr<class UOrionSetting> OrionSetting;

public:
	UOrionSaveGameSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	void InitializeLocalPlayerSaveGameSlots(ULocalPlayer* LocalPlayer);

	void HandlePreLoadMap(const FString& MapURL);
	void HandlePostLoadMap(UWorld* World);

	FString Platform_CreateSaveGameName(ULocalPlayer* LocalPlayer, int32 SaveSlotIndex) const;

public:
	UFUNCTION(BlueprintCallable, Category = "SaveSubsystem")
	void DeleteSaveGame(int32 Slot);

	// Save game at current save slot
	UFUNCTION(BlueprintCallable, Category = "SaveSubsystem")
	void SaveGame(int32 Slot);

	// Load game from save slot
	// @param Slot - Save slot index
	UFUNCTION(BlueprintCallable, Category = "SaveSubsystem")
	void LoadGame(int32 Slot);

	// Get save game object from current save slot
	// @return NULL if not using save slot (e.g PIE)
	UFUNCTION(BlueprintPure, Category = "SaveSubsystem")
	FORCEINLINE UOrionSaveGame* GetCurrentSaveGameObject() const
	{
		return (CurrentSaveSlot == -1) ? nullptr : SaveGameSlots[CurrentSaveSlot];
	}

	UFUNCTION(BlueprintPure, Category = "SaveSubsystem")
	FORCEINLINE UOrionSaveGame* GetSaveGameObject(int slot = 0) const
	{
		return (CurrentSaveSlot == -1) ? nullptr : SaveGameSlots[slot];
	}


	// Get save game list
	UFUNCTION(BlueprintPure, Category = "SaveSubsystem")
	FORCEINLINE TArray<UOrionSaveGame*> GetSaveSlots() const
	{
		return SaveGameSlots;
	}

	UFUNCTION(BlueprintPure, Category = "SaveSubsystem")
	FORCEINLINE bool IsLoadingSaveGame()
	{
		return bIsLoadingSaveGame;
	}

	// ================================================================================================ //
	// CONSOLE COMMANDS
	// ================================================================================================ //
	UFUNCTION(Exec)
	void orionSaveGame(const TArray<FString>& Slot);

	UFUNCTION(Exec)
	void orionLoadGame(const TArray<FString>& Slot);

};