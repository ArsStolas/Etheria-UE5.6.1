// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Decorator/IsQuestFailed.h"
#include "QuestComponent.h"
#include "QuestBuilderFunctionLibrary.h"
#include "Quest.h"
#include "Engine/World.h"

UIsQuestFailed::UIsQuestFailed()
{
    bUseQuestTag = true;
}

bool UIsQuestFailed::PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const
{
    FName QuestID = QuestTag.GetTagName();
    if (GetWorld())
    {
        if (UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController)))
        {
            if (UQuest* FoundedQuest = QuestComp->QuestMap.FindRef(QuestID))
            {
                return QuestComp->IsQuestAtState(QuestTag, EQuestState::E_Fail);
            }
        }
    }
    
    return false;
}

FString UIsQuestFailed::GetNodeDisplayText_Implementation() const
{
    return FString::Printf(TEXT("Is Quest Failed :  %s"), *GetShortTag(QuestTag));
}
