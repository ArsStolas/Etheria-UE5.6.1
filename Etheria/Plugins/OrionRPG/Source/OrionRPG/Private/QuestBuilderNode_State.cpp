// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderNode_State.h"
#include "QuestComponent.h"
#include "Quest.h"
#include "Event/OrionEvent.h"


void UQuestBuilderNode_State::Initialize(bool bLaunchEventOnLoad)
{
    Super::Initialize(bLaunchEventOnLoad);
}

void UQuestBuilderNode_State::BeginNode()
{
    Super::BeginNode();
	BeginState();
}

void UQuestBuilderNode_State::BeginState()
{
    switch (QuestState)
    {
    case EQuestState::E_Complete:
        GetQuestComponent()->CompleteQuest(Quest);
        break;
    case EQuestState::E_Fail:
        GetQuestComponent()->FailQuest(Quest);
        break;
    }
	Deinitialize();
    
}

#if WITH_EDITOR
FText UQuestBuilderNode_State::GetNodeTitle() const
{
    if (NodeName.Len())
        return FText::FromString(NodeName);

    switch (QuestState)
    {
        case EQuestState::E_Complete:
            return FText::FromString("Complete Quest");
            break;
        case EQuestState::E_Fail:
            return FText::FromString("Fail Quest");
            break;
        default:
            return FText::FromString("Default");
            break;
    }
}
void UQuestBuilderNode_State::SetNodeTitle(const FText& NewTitle)
{
    ID = FName(NewTitle.ToString());
}
FText UQuestBuilderNode_State::GetNodeDescription() const
{
    return Description;
}


FLinearColor UQuestBuilderNode_State::GetBackgroundColor() const
{
    FLinearColor FailQuestColor(.34f, .024f, .024f);
    FLinearColor CompleteQuestColor(0.20f, 0.32f, 0.15f);
    FLinearColor Default(0.05f, 0.05f, 0.05f);

    switch (QuestState)
    {
        case EQuestState::E_Complete:
            return CompleteQuestColor;
        case EQuestState::E_Fail:
            return FailQuestColor;
        default:
            return Default;
    }
}
#endif