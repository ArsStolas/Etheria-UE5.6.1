// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Decorator/IsQuestAtState.h"
#include "QuestComponent.h"
#include "QuestBuilderFunctionLibrary.h"
#include "Engine/World.h"
#include "Quest.h"

UIsQuestAtState::UIsQuestAtState()
{
    bUseQuestTag = true;
}

bool UIsQuestAtState::PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const
{
    FName QuestID = QuestTag.GetTagName();
    if (GetWorld())
    {
        if (UQuestComponent* QuestComp = UQuestBuilderFunctionLibrary::GetQuestComponentFromTarget(Cast<AActor>(OwnerController)))
        {
            if (UQuest* FoundedQuest = QuestComp->QuestMap.FindRef(QuestID))
            {
                return QuestComp->IsQuestAtState(QuestTag, QuestState);
            }
        }
    }
    return false;
}

FString UIsQuestAtState::GetNodeDisplayText_Implementation() const
{
    FString QuestStateString = StaticEnum<EQuestState>()->GetNameStringByValue(static_cast<int64>(QuestState));
    return FString::Printf(TEXT("Is %s At State :  %s"), *GetShortTag(QuestTag), *QuestStateString);

}
