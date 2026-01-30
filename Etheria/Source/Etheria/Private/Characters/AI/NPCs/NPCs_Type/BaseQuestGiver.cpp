/*
* Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: BaseQuestGiver - Source
*/

#include "Characters/AI/NPCs/NPCs_Type/BaseQuestGiver.h"
#include "Components/Quests/QuestGiverComponent.h"

ABaseQuestGiver::ABaseQuestGiver()
{
 PrimaryActorTick.bCanEverTick = false;

 QuestGiverComp = CreateDefaultSubobject<UQuestGiverComponent>(TEXT("QuestGiverComp"));
}


void ABaseQuestGiver::BeginPlay()
{
 Super::BeginPlay();
 AIType = EAIType::Neutral;
}
