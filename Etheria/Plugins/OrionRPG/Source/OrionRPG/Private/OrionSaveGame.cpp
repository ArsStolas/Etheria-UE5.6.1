// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "OrionSaveGame.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "OrionSetting.h"
#include "QuestBuilderFunctionLibrary.h"
#include "QuestComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"
#include "Engine/World.h"

UOrionSaveGame::UOrionSaveGame()
{
	bIsSaving = false;
}

void UOrionSaveGame::ResetToDefault()
{
	PlayInfoData = FOrionPlayInfoSaveData();
	PlayerData = FOrionPlayerSaveData();
	QuestData = FOrionQuestSaveData();
	SavedActors.Empty();

	Super::ResetToDefault();
}

int32 UOrionSaveGame::GetLatestDataVersion() const
{
	return 1;
}

void UOrionSaveGame::HandlePreSave()
{
	UOrionSetting* OrionSetting = GetMutableDefault<UOrionSetting>();
	bIsSaving = true;
	Super::HandlePreSave();
	UWorld* World = OwningPlayer->GetWorld();
	
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController<APlayerController>();
	
	if (!PC) return;

	APawn* PlayerChar = PC->GetPawn<APawn>();

	// Play info data
	{
		const FDateTime CurrentDateTime = FDateTime::Now();
		const FTimespan Duration = CurrentDateTime - PlayInfoData.CreatedDateTime;
		PlayInfoData.LastPlayDateTime = CurrentDateTime;
		PlayInfoData.TotalPlayTimeSeconds = Duration.GetTotalSeconds();


		PlayInfoData.MapURL = GetCurrentMapURL();


		if (OrionSetting->bSaveDataLayer)
		{
			if (UDataLayerManager* DataLayerManager = UDataLayerManager::GetDataLayerManager(World))
			{
				PlayInfoData.DataLayerNames = DataLayerManager->GetEffectiveActiveDataLayerNames();
			}
		}
	}


	// Player data
	{
		if (PlayerChar)
		{
			PlayerData.Location = PlayerChar->GetActorLocation();
			PlayerData.Rotation = PlayerChar->GetActorRotation();
		}
	}

	//Quest Data
	{
		UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(PC);
		if (QuestComp)
		{
			QuestData = QuestComp->GetSaveGameData();
		}
	}

	CachedActors.Empty();

	TArray<AActor*> SaveGameActors;
	UGameplayStatics::GetAllActorsWithInterface(World, UOrionSaveGameInterface::StaticClass(), SaveGameActors);

	// Build TMap<FName, AActor*> for fast lookup
	TMap<FName, AActor*> ActorMap;
	for (AActor* Actor : SaveGameActors)
	{
		if (Actor)
		{
			ActorMap.Add(Actor->GetFName(), Actor);
		}
	}

	TArray<FOrionActorSaveData> SavedActorArray;

	SavedActors.GenerateValueArray(SavedActorArray);

	//ensure to remove saved actor that are destroyed and spawned
	for (int32 i = 0; i < SavedActorArray.Num(); ++i)
	{
		const FOrionActorSaveData& ActorData = SavedActorArray[i];

		AActor* FoundActor = ActorMap.FindRef(ActorData.ActorName);
		if(!FoundActor && !ActorData.bNetStartup && ActorData.MapURL.Equals(GetCurrentMapURL(), ESearchCase::IgnoreCase))
		{
			SavedActors.Remove(ActorData.ActorName);
		}
	}

	//Save the actors
	for (AActor* Actor : SaveGameActors)
	{
		SaveActor(Actor);
	}

}

void UOrionSaveGame::HandlePostSave(bool bSuccess)
{
	bIsSaving = false;
	if (bSuccess)
	{
		for (auto& Actor : CachedActors)
		{
			if (Actor.IsValid())
			{
				IOrionSaveGameInterface::Execute_OnSaved(Actor.Get());
			}
		}
	}
}

void UOrionSaveGame::HandleLoadGame(UWorld* World, bool bFromLoadGame)
{
	if (!World) return;
	UE_LOG(LogTemp, Log, TEXT("Handle load game (bFromLoadGame: %d)"), bFromLoadGame);

	APlayerController* PC = World->GetFirstPlayerController<APlayerController>();
	
	if (!PC) return;

	UOrionSetting* OrionSetting = GetMutableDefault<UOrionSetting>();

	// Data that we need to restore if load game only, not on every level transition)
	if (bFromLoadGame)
	{
		UGameInstance* GI = World->GetGameInstance<UGameInstance>();
		if (!GI) return;

		// PLAYER
		{
			APawn* PlayerChar = PC->GetPawn<APawn>();

			FString CurrentMapURL = GetCurrentMapURL();

			// Compare with saved URL
			UE_LOG(LogTemp, Log, TEXT("CurrentMapURL: %s, PlayInfoData.MapURL: %s"), *CurrentMapURL, *PlayInfoData.MapURL);
			// Compare with saved URL
			bool bIsSameMap = CurrentMapURL.Equals(PlayInfoData.MapURL, ESearchCase::IgnoreCase);

			if (bIsSameMap)
			{
				FHitResult HitResult;
				if (PlayerChar)
				{
					PlayerChar->SetActorTransform(FTransform(PlayerData.Rotation, PlayerData.Location), false, &HitResult, ETeleportType::TeleportPhysics);
				}
			}
			
		}

		

		// DATA LAYERS
		{
			if (OrionSetting->bSaveDataLayer)
			{
				UDataLayerManager* DataLayerManager = UDataLayerManager::GetDataLayerManager(World);

				for (const FName& DataLayerName : PlayInfoData.DataLayerNames)
				{
					const UDataLayerInstance* DataLayerInstance = DataLayerManager->GetDataLayerInstanceFromName(DataLayerName);
					
					if (!DataLayerInstance) return;
					DataLayerManager->SetDataLayerInstanceRuntimeState(DataLayerInstance, EDataLayerRuntimeState::Activated);
				}
			}
		}

		//ACTOR DATA
		{


			TArray<AActor*> SaveGameActors;
			UGameplayStatics::GetAllActorsWithInterface(World, UOrionSaveGameInterface::StaticClass(), SaveGameActors);

			// Build TMap<FName, AActor*> for fast lookup
			TMap<FName, AActor*> ActorMap;
			for (AActor* Actor : SaveGameActors)
			{
				if (Actor)
				{
					ActorMap.Add(Actor->GetFName(), Actor);
				}
			}

			TArray<FOrionActorSaveData> SavedActorArray;

			SavedActors.GenerateValueArray(SavedActorArray);

            for (int32 i = 0; i < SavedActorArray.Num(); ++i)
            {
				const FOrionActorSaveData& ActorData = SavedActorArray[i];

				AActor* FoundActor = ActorMap.FindRef(ActorData.ActorName);

				if (!FoundActor && !ActorData.bNetStartup)
				{
					FString CurrentMapURL = GetCurrentMapURL();
					bool bShouldSpawn = CurrentMapURL.Equals(ActorData.MapURL, ESearchCase::IgnoreCase);

					if (bShouldSpawn)
					{
						FActorSpawnParameters SpawnParams;
						SpawnParams.Name = ActorData.ActorName;
						SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ReturnNull;
						SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
						if (ActorData.ActorClass == nullptr)
						{
							continue;
						}
						FoundActor = World->SpawnActor<AActor>(ActorData.ActorClass, ActorData.ActorTransform, SpawnParams);
						if (FoundActor)
						{
							ActorMap.Add(ActorData.ActorName, FoundActor);
						}
					}
				}
				if (FoundActor)
				{
					FMemoryReader MyMemoryReader(ActorData.ByteData);
					FoundActor->SetActorTransform(ActorData.ActorTransform);
					FObjectAndNameAsStringProxyArchive Ar(MyMemoryReader, true);
					Ar.ArNoDelta = true;
					Ar.ArIsSaveGame = true;

					FoundActor->Serialize(Ar);

					IOrionSaveGameInterface::Execute_OnLoaded(FoundActor);
				}
            }
		}

	
		// QUEST
		{
			UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(PC);
			if (QuestComp)
			{
				QuestComp->InitializeFromSaveGame(QuestData);
			}
		}

	}


	
}

FString UOrionSaveGame::GetCurrentMapURL()
{
	UWorld* World = OwningPlayer->GetWorld();
	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix);

	FString MapPath;
	const FString PathName = World->GetPathName();
	int32 TokenIndex = INDEX_NONE;
	if (PathName.FindLastChar(TEXT('/'), TokenIndex))
	{
		MapPath = PathName.LeftChop(PathName.Len() - TokenIndex);
	}
	return FString::Printf(TEXT("%s/%s"), *MapPath, *MapName);
}

void UOrionSaveGame::SaveActor(AActor* Actor)
{
	if (Actor && Actor->Implements<UOrionSaveGameInterface>())
	{
		IOrionSaveGameInterface::Execute_OnPreSave(Actor);
		CachedActors.AddUnique(Actor);

		FOrionActorSaveData ActorData;
		ActorData.ActorName = Actor->GetFName();
		ActorData.ActorTransform = Actor->GetActorTransform();
		ActorData.MapURL = GetCurrentMapURL();
		ActorData.bNetStartup = Actor->bNetStartup;
		ActorData.ActorClass = Actor->GetClass();
	

		FMemoryWriter MyMemoryWriter(ActorData.ByteData);

		FObjectAndNameAsStringProxyArchive Ar(MyMemoryWriter, true);

		Ar.ArIsSaveGame = true;
		Ar.ArNoDelta = true;
		Actor->Serialize(Ar);

		SavedActors.Emplace(Actor->GetFName(), ActorData);
	}

	
}

void UOrionSaveGame::RemoveSavedActor(AActor* Actor)
{
	if (!Actor) return;
	SavedActors.Remove(Actor->GetFName());
}
