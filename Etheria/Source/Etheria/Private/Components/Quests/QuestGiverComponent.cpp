/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "QuestGiverComponent" - Source
 */

#include "Components/Quests/QuestGiverComponent.h"
#include "Components/Quests/QuestComponent.h"
#include "Components/Quests/System/QuestDefinition.h"

UQuestGiverComponent::UQuestGiverComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UQuestGiverComponent::BeginPlay()
{
    Super::BeginPlay();
}

bool UQuestGiverComponent::OfferQuestToTarget(UQuestComponent* TargetQuestComponent, UQuestDefinition* QuestDef)
{
    if (!TargetQuestComponent || !QuestDef)
    {
        return false;
    }

    if (TargetQuestComponent->IsQuestActive(QuestDef->questId) ||
        TargetQuestComponent->IsQuestCompleted(QuestDef->questId))
    {
        return false;
    }

    return TargetQuestComponent->StartQuest(QuestDef);
}

void UQuestGiverComponent::OfferAllAvailableToTarget(UQuestComponent* TargetQuestComponent)
{
    if (!TargetQuestComponent)
    {
        return;
    }

    for (UQuestDefinition* Def : questsToOffer)
    {
        OfferQuestToTarget(TargetQuestComponent, Def);
    }
}
