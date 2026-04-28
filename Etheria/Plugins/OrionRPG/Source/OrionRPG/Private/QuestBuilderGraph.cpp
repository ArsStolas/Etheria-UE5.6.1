// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderGraph.h"
#include "Quest.h"
#include "QuestComponent.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderEdge.h"

#define LOCTEXT_NAMESPACE "QuestBuilderGraph"

UQuestBuilderGraph::UQuestBuilderGraph()
{
}

UQuestBuilderGraph::~UQuestBuilderGraph()
{
	
}

void UQuestBuilderGraph::Initialize()
{
}
void UQuestBuilderGraph::Deinitialize()
{
	ID = NAME_None;
}

#if WITH_EDITORONLY_DATA

void UQuestBuilderGraph::GetAllGraphs(TArray<UEdGraph*>& Graphs) const
{
	for (int32 i = 0; i < QuestGraphPages.Num(); ++i)
	{
		UEdGraph* Graph = QuestGraphPages[i];
		if (Graph)
		{
			Graphs.Add(Graph);
			Graph->GetAllChildrenGraphs(Graphs);
		}
	}
	
}
#endif // WITH_EDITORONLY_DATA

#undef LOCTEXT_NAMESPACE