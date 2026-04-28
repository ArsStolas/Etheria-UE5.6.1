// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdSubNode_Decorator.h"
#include "DialogBuilderSetting.h"
#include "Decorator/OrionDecorator.h"

UDialogBuilderEdSubNode_Decorator::UDialogBuilderEdSubNode_Decorator()
{
}

FText UDialogBuilderEdSubNode_Decorator::GetNodeTitle(ENodeTitleType::Type TitleType) const
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

		return FText::Format(NSLOCTEXT("DialogGraph", "NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
	}

	return Super::GetNodeTitle(TitleType);
}

FName UDialogBuilderEdSubNode_Decorator::GetNameIcon() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Conditional.Icon");
}

FText UDialogBuilderEdSubNode_Decorator::GetDescription() const
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
