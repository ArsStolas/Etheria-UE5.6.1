// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEdSubNode_Event.h"
#include "QuestBuilderSetting.h"
#include "Event/OrionEvent.h"

UQuestBuilderEdSubNode_Event::UQuestBuilderEdSubNode_Event()
{
}

FText UQuestBuilderEdSubNode_Event::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UOrionEvent* Event = Cast<UOrionEvent>(NodeInstance);
	if (Event != NULL)
	{
		return FText::FromString(Event->GetNodeName());
	}
	else if (!ClassData.GetClassName().IsEmpty())
	{
		FString StoredClassName = ClassData.GetClassName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));

		return FText::Format(NSLOCTEXT("QuestGraph", "NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
	}

	return Super::GetNodeTitle(TitleType);
}

FName UQuestBuilderEdSubNode_Event::GetNameIcon() const
{
    return FName("GraphEditor.CustomEvent_16x");
}

FText UQuestBuilderEdSubNode_Event::GetDescription() const
{
	UOrionEvent* Event = Cast<UOrionEvent>(NodeInstance);
	
	return Event ? FText::FromString(Event->GetNodeDisplayText()) : FText();
	
}
