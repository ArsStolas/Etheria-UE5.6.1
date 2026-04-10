// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdSubNode_Event.h"
#include "DialogBuilderSetting.h"
#include "Event/OrionEvent.h"

UDialogBuilderEdSubNode_Event::UDialogBuilderEdSubNode_Event()
{
}

FText UDialogBuilderEdSubNode_Event::GetNodeTitle(ENodeTitleType::Type TitleType) const
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

		return FText::Format(NSLOCTEXT("DialogGraph", "NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
	}

	return Super::GetNodeTitle(TitleType);
}

FName UDialogBuilderEdSubNode_Event::GetNameIcon() const
{
    return FName("GraphEditor.CustomEvent_16x");
}

FText UDialogBuilderEdSubNode_Event::GetDescription() const
{
	UOrionEvent* Event = Cast<UOrionEvent>(NodeInstance);
	
	return Event ? FText::FromString(Event->GetNodeDisplayText()) : FText();
	
}
