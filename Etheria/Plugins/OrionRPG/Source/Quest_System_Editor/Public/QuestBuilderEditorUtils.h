// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuestBuilderEdNode.h"
#include "QuestBuilderGraph.h"

/**
 * 
 */
class QUEST_SYSTEM_EDITOR_API FQuestBuilderEditorUtils
{
public:
	FQuestBuilderEditorUtils();
	~FQuestBuilderEditorUtils();


	static FName FindUniqueQuestName(const FString& InBaseName);

	static FName FindUniqueQuestGraphName(const FString& InBaseName);


	/**
	 * Creates a new empty graph.
	 *
	 * @param	ParentScope		The outer of the new graph.
	 * @param	GraphName		Name of the graph to add.
	 * @param	SchemaClass		Schema to use for the new graph.
	 *
	 * @return	null if it fails, else.
	 */
	static class UEdGraph* CreateNewGraph(UObject* ParentScope, const FName& GraphName, TSubclassOf<class UEdGraph> GraphClass, TSubclassOf<class UEdGraphSchema> SchemaClass);


	/** Returns all nodes in all graphs of the specified class */
	template< class T >
	static inline void GetAllNodesOfClass(const UQuestBuilderGraph* QuestBuilderGraph, TArray<T*>& OutNodes)
	{
		TArray<UEdGraph*> AllGraphs;
		QuestBuilderGraph->GetAllGraphs(AllGraphs);
		for (int32 i = 0; i < AllGraphs.Num(); i++)
		{
			check(AllGraphs[i] != NULL);
			TArray<T*> GraphNodes;
			AllGraphs[i]->GetNodesOfClass<T>(GraphNodes);
			OutNodes.Append(GraphNodes);
		}
	}

	/**
	 * Removes the supplied node from the UQuestBuilderGraph.
	 *
	 * @param Node				The node to remove.
	 * @param bDontRecompile	If true, the UQuestBuilderGraph will not be marked as modified, and will not be recompiled.  Useful for if you are removing several node at once, and don't want to recompile each time
	 */
	static void RemoveNode(UQuestBuilderGraph* QuestBuilderGraph, UQuestBuilderEdNode* Node, bool bDontRecompile = false);

	/**
	 * Removes the supplied graph from the UQuestBuilderGraph.
	 *
	 * @param UQuestBuilderGraph	The UQuestBuilderGraph containing the graph
	 * @param GraphToRemove		The graph to remove.
	 */
	static void RemoveGraph(UQuestBuilderGraph* QuestBuilderGraph, class UEdGraph* GraphToRemove);


	/** Adds an questgraph page to this Quest Editor */
	static void AddQuestGraphPage(UQuestBuilderGraph* InQuestSystemGraph, class UEdGraph* Graph);

	/** Returns the event graph, if any */
	static UEdGraph* FindQuestGraph(const UQuestBuilderGraph* QuestBuilderGraph);

	/** Given a selection of nodes, return the instances that should be selected be selected for editing in the property panel */
	static TArray<UObject*> GetSelectionForPropertyEditor(const TSet<UObject*>& InSelection);
};
