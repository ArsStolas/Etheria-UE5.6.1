/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "QuestLocationTrigger" - Source
 */

#include "Components/Quests/System/QuestLocationTrigger.h"
#include "Components/BoxComponent.h"
#include "Components/Quests/QuestComponent.h"
#include "GameFramework/Actor.h"

AQuestLocationTrigger::AQuestLocationTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

    boxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
    RootComponent = boxComponent;

    boxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    boxComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void AQuestLocationTrigger::BeginPlay()
{
    Super::BeginPlay();

    if (boxComponent)
    {
        boxComponent->OnComponentBeginOverlap.AddDynamic(this, &AQuestLocationTrigger::HandleBeginOverlap);
    }
}

void AQuestLocationTrigger::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    UQuestComponent* QuestComp = OtherActor->FindComponentByClass<UQuestComponent>();
    if (!QuestComp)
    {
        return;
    }

    QuestComp->NotifyLocationReached(locationId, this);
}
