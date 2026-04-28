// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderNode_PlayerChoice.h"
#include "DialogComponent.h"
#include "Event/OrionEvent.h"


UDialogBuilderNode_PlayerChoice::UDialogBuilderNode_PlayerChoice()
{
}

void UDialogBuilderNode_PlayerChoice::BeginNode()
{
	Super::BeginNode();
	EvaluateNextNode();
}

#if WITH_EDITOR

FText UDialogBuilderNode_PlayerChoice::GetNodeTitle() const
{
	return ChoiceText;
}
void UDialogBuilderNode_PlayerChoice::SetNodeTitle(const FText& NewTitle)
{
    ID = FName(NewTitle.ToString());
}

FText UDialogBuilderNode_PlayerChoice::GetNodeDescription() const
{
	return ChoiceText.IsEmpty() ? FText::FromString("Choice Text") : ChoiceText;
}

#endif