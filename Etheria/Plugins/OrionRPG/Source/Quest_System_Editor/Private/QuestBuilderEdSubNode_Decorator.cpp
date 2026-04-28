// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEdSubNode_Decorator.h"
#include "QuestBuilderSetting.h"
#include "Decorator/OrionDecorator.h"

UQuestBuilderEdSubNode_Decorator::UQuestBuilderEdSubNode_Decorator()
{
}

FText UQuestBuilderEdSubNode_Decorator::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UOrionDecorator* Decorator = Cast<UOrionDecorator>(NodeInstance);
	if (Decorator != NULL)
	{
		return FText::FromString(Decorator->GetNodeName());
	}
	else if (!ClassData.GetClassName().IsEmpty())
	{
		FString StoredClassName = ClassData.GetClassName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));

		return FText::Format(NSLOCTEXT("QuestGraph", "NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
	}

	return Super::GetNodeTitle(TitleType);
}

FName UQuestBuilderEdSubNode_Decorator::GetNameIcon() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Conditional.Icon");
}

FText UQuestBuilderEdSubNode_Decorator::GetDescription() const
{
	UOrionDecorator* Decorator = Cast<UOrionDecorator>(NodeInstance);
	if (Decorator)
	{
		FString Desc = Decorator->GetNodeDisplayText();
		if (Decorator->InvertCondition)
		{
            Desc += "\n" + FString::Printf(TEXT("Invert Condition : %s"), Decorator->InvertCondition ? TEXT("True") : TEXT("False"));
		}
		return FText::FromString(Desc);
	}

	return FText();
}
