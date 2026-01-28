
/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDungeonTravelComponent" - Source
 */

#include "World/Dungeon/DungeonTravelComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

UDungeonTravelComponent::UDungeonTravelComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UDungeonTravelComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UDungeonTravelComponent::SaveReturnData(const FTransform& Transform, TSoftObjectPtr<UWorld> Origin)
{
    ReturnData.ReturnTransform = Transform;
    ReturnData.OriginLevel = Origin;
}

void UDungeonTravelComponent::StartPreloadDungeon(TSoftObjectPtr<UWorld> DungeonLevel)
{
    if (!DungeonLevel.IsValid())
    {
        return;
    }

    PendingLevel = DungeonLevel;

    StreamingLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
        GetWorld(),
        DungeonLevel,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        bLevelLoaded
    );
}

void UDungeonTravelComponent::CommitTravel(const FName SpawnTag)
{
    if (!bLevelLoaded || !StreamingLevel)
    {
        return;
    }

    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return;
    }

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag(SpawnTag))
        {
            Character->SetActorTransform(It->GetActorTransform());
            break;
        }
    }
}

void UDungeonTravelComponent::CommitReturn()
{
    ACharacter* Character = Cast<ACharacter>(GetOwner());
    if (!Character)
    {
        return;
    }

    Character->SetActorTransform(ReturnData.ReturnTransform);

    if (StreamingLevel)
    {
        StreamingLevel->SetIsRequestingUnloadAndRemoval(true);
    }
}
