
/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ADungeonPortal" - Source
 */

#include "World/Dungeon/DungeonPortal.h"
#include "World/Dungeon/DungeonTravelComponent.h"

ADungeonPortal::ADungeonPortal()
{
    Root = CreateDefaultSubobject<USceneComponent>("Root");
    RootComponent = Root;

    InteractionBox = CreateDefaultSubobject<UBoxComponent>("InteractionBox");
    InteractionBox->SetupAttachment(Root);

    ReturnRoot = CreateDefaultSubobject<USceneComponent>("ReturnRoot");
    ReturnRoot->SetupAttachment(Root);

    ReturnPlane = CreateDefaultSubobject<UStaticMeshComponent>("ReturnPlane");
    ReturnPlane->SetupAttachment(ReturnRoot);

    ReturnArrow = CreateDefaultSubobject<UArrowComponent>("ReturnArrow");
    ReturnArrow->SetupAttachment(ReturnRoot);
}

void ADungeonPortal::NotifySequenceFinished(AActor* Interactor)
{
    if (!Interactor)
    {
        return;
    }

    UDungeonTravelComponent* TravelComp = Interactor->FindComponentByClass<UDungeonTravelComponent>();
    if (!TravelComp)
    {
        return;
    }

    if (bIsReturnPortal)
    {
        TravelComp->CommitReturn();
    }
    else
    {
        TravelComp->SaveReturnData(ReturnRoot->GetComponentTransform(), GetWorld());
        TravelComp->StartPreloadDungeon(DestinationLevel);
        TravelComp->CommitTravel(DestinationSpawnTag);
    }
}
