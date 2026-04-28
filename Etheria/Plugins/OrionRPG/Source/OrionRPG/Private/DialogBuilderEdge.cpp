// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEdge.h"

UDialogBuilderEdge::UDialogBuilderEdge()
{
}

UDialogBuilderEdge::~UDialogBuilderEdge()
{
}

UDialogBuilderGraph* UDialogBuilderEdge::GetGraph() const
{
	return Graph;
}


#if WITH_EDITOR
void UDialogBuilderEdge::SetNodeTitle(const FText& NewTitle)
{
	NodeTitle = NewTitle;
}
#endif // #if WITH_EDITOR

