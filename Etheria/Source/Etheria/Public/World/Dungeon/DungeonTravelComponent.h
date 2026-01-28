
/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDungeonTravelComponent" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/LevelStreamingDynamic.h"
#include "DungeonTravelComponent.generated.h"

USTRUCT(BlueprintType)
struct FDungeonReturnData
{
    GENERATED_BODY()

    UPROPERTY()
    FTransform ReturnTransform;

    UPROPERTY()
    TSoftObjectPtr<UWorld> OriginLevel;
};

UCLASS(ClassGroup=(Dungeon), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UDungeonTravelComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDungeonTravelComponent();

    UFUNCTION(BlueprintCallable, Category="Dungeon")
    void StartPreloadDungeon(TSoftObjectPtr<UWorld> DungeonLevel);

    UFUNCTION(BlueprintCallable, Category="Dungeon")
    void CommitTravel(const FName SpawnTag);

    UFUNCTION(BlueprintCallable, Category="Dungeon")
    void CommitReturn();

    void SaveReturnData(const FTransform& Transform, TSoftObjectPtr<UWorld> Origin);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    ULevelStreamingDynamic* StreamingLevel;

    UPROPERTY()
    TSoftObjectPtr<UWorld> PendingLevel;

    UPROPERTY()
    FDungeonReturnData ReturnData;

    bool bLevelLoaded = false;
};
