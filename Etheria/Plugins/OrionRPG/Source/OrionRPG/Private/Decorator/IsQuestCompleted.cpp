// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Decorator/IsQuestCompleted.h"
#include "QuestComponent.h"
#include "QuestBuilderFunctionLibrary.h"
#include "Quest.h"
#include "Engine/World.h"

UIsQuestCompleted::UIsQuestCompleted()
{
    bUseQuestTag = true;
}

bool UIsQuestCompleted::PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const
{
    FName QuestID = QuestTag.GetTagName();
    if (GetWorld())
    {
        if (UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController)))
        {
            return QuestComp->IsQuestAtState(QuestTag, EQuestState::E_Complete);
        }
    }
    
    return false;
}

FString UIsQuestCompleted::GetNodeDisplayText_Implementation() const
{
    return FString::Printf(TEXT("Is Quest Completed :  %s"), *GetShortTag(QuestTag));
}
