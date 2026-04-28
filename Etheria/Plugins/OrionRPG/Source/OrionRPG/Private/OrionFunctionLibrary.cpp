// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "OrionFunctionLibrary.h"
#include "OrionRPG.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "OrionSaveGameSubsystem.h"
#include "OrionSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/Vector.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"


UOrionSaveGame* UOrionFunctionLibrary::GetCurrentSaveGameObject(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UOrionSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UOrionSaveGameSubsystem>() : nullptr;
	UOrionSaveGame* SaveGameObject = SaveSubsystem ? SaveSubsystem->GetCurrentSaveGameObject() : nullptr;

	return SaveGameObject;
}

UOrionSaveGame* UOrionFunctionLibrary::GetSaveGameObject(const UObject* WorldContextObject, int slot)
{
	UWorld* World = WorldContextObject->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UOrionSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UOrionSaveGameSubsystem>() : nullptr;
	UOrionSaveGame* SaveGameObject = SaveSubsystem ? SaveSubsystem->GetSaveGameObject(slot) : nullptr;

	return SaveGameObject;
}

void UOrionFunctionLibrary::SaveActorData(const UObject* WorldContextObject, AActor* ActorToSave)
{
	if (!ActorToSave) return;

	UOrionSaveGame* SaveGameObject = UOrionFunctionLibrary::GetCurrentSaveGameObject(WorldContextObject);

	if(SaveGameObject)
	{
		SaveGameObject->SaveActor(ActorToSave);
	}
}

void UOrionFunctionLibrary::RemoveSavedActorData(const UObject* WorldContextObject, AActor* InActor)
{
	if (!InActor) return;

	UOrionSaveGame* SaveGameObject = UOrionFunctionLibrary::GetCurrentSaveGameObject(WorldContextObject);

	if (SaveGameObject)
	{
		SaveGameObject->RemoveSavedActor(InActor);
	}
}

void UOrionFunctionLibrary::SaveActorDataToSlot(const UObject* WorldContextObject, int slot, AActor* ActorToSave)
{
	if (!ActorToSave) return;

	UOrionSaveGame* SaveGameObject = UOrionFunctionLibrary::GetSaveGameObject(WorldContextObject, slot);

	if (SaveGameObject)
	{
		SaveGameObject->SaveActor(ActorToSave);
	}
}


