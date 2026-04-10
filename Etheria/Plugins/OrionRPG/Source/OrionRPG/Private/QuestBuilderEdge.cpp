// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEdge.h"

UQuestBuilderEdge::UQuestBuilderEdge()
{
}

UQuestBuilderEdge::~UQuestBuilderEdge()
{
}

UQuestBuilderGraph* UQuestBuilderEdge::GetGraph() const
{
	return Graph;
}


#if WITH_EDITOR
void UQuestBuilderEdge::SetNodeTitle(const FText& NewTitle)
{
	NodeTitle = NewTitle;
}
#endif // #if WITH_EDITOR

