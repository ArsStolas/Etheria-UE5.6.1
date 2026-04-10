// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "QuestBuilderEditorUtils.h"
#include "QuestBuilderNode.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderSetting.h"


#define LOCTEXT_NAMESPACE "QuestBuilderEditorUtils"

FQuestBuilderEditorUtils::FQuestBuilderEditorUtils()
{
}

FQuestBuilderEditorUtils::~FQuestBuilderEditorUtils()
{
}

FName FQuestBuilderEditorUtils::FindUniqueQuestName(const FString& InBaseName)
{
	// Start with the base name
	FString UniqueName = InBaseName + FGuid::NewGuid().ToString();
	return FName(*UniqueName);

}

FName FQuestBuilderEditorUtils::FindUniqueQuestGraphName(const FString& InBaseName)
{
	// Start with the base name
	FString UniqueName = InBaseName + FGuid::NewGuid().ToString();
	return FName(*UniqueName);

}


UEdGraph* FQuestBuilderEditorUtils::CreateNewGraph(UObject* ParentScope, const FName& GraphName, TSubclassOf<class UEdGraph> GraphClass, TSubclassOf<class UEdGraphSchema> SchemaClass)
{
	UEdGraph* NewGraph = nullptr;

	
	// Construct a new graph with a default name
	NewGraph = NewObject<UEdGraph>(ParentScope, GraphClass, NAME_None, RF_Transactional);
	
	NewGraph->Schema = SchemaClass;
	NewGraph->GetSchema()->CreateDefaultNodesForGraph(*NewGraph);
	NewGraph->Rename(*(GraphName.ToString()), ParentScope, REN_DoNotDirty | REN_ForceNoResetLoaders);

	if (UQuestBuilderEdGraph* QuestGraph = Cast<UQuestBuilderEdGraph>(NewGraph))
	{
		QuestGraph->Quest = NewObject<UQuest>(ParentScope, UQuest::StaticClass(), NAME_None, RF_Transactional);
		QuestGraph->Quest->QuestGraph = QuestGraph->GetQuestBuilderGraph();
		QuestGraph->Quest->ID = GraphName;
		QuestGraph->Quest->QuestName = FText::FromString("New Quest");

		if (UQuestBuilderGraph* QuestBuilderGraph = Cast<UQuestBuilderGraph>(QuestGraph->GetQuestBuilderGraph()))
		{
			QuestBuilderGraph->QuestList.Add(QuestGraph->Quest);
		}
	}

	
	return NewGraph;
}



void FQuestBuilderEditorUtils::RemoveNode(UQuestBuilderGraph* QuestBuilderGraph, UQuestBuilderEdNode* Node, bool bDontRecompile)
{
	check(Node);

	const UEdGraphSchema* Schema = nullptr;

	// Ensure we mark parent graph modified
	if (UEdGraph* GraphObj = Node->GetGraph())
	{
		GraphObj->Modify();
		Schema = GraphObj->GetSchema();
	}

	Node->Modify();

	// Timelines will be removed from the blueprint if the node is a UK2Node_Timeline
	if (Schema)
	{
		Schema->BreakNodeLinks(*Node);
	}

	Node->DestroyNode();

	/*if (!bDontRecompile && (Blueprint != nullptr))
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}*/
}

void FQuestBuilderEditorUtils::RemoveGraph(UQuestBuilderGraph* QuestBuilderGraph, UEdGraph* GraphToRemove)
{
	GraphToRemove->Modify();

	for (UObject* TestOuter = GraphToRemove->GetOuter(); TestOuter; TestOuter = TestOuter->GetOuter())
	{
		if (TestOuter == QuestBuilderGraph)
		{
			QuestBuilderGraph->QuestGraphPages.Remove(GraphToRemove);
			if (UQuestBuilderEdGraph* QuestGraphToRemove = Cast<UQuestBuilderEdGraph>(GraphToRemove))
			{
				QuestBuilderGraph->QuestList.Remove(QuestGraphToRemove->Quest);
			}

			// Can't just call Remove, the object is wrapped in a struct
			for (int EditedDocIdx = 0; EditedDocIdx < QuestBuilderGraph->LastEditedDocuments.Num(); ++EditedDocIdx)
			{
				if (QuestBuilderGraph->LastEditedDocuments[EditedDocIdx].EditedObjectPath.ResolveObject() == GraphToRemove)
				{
					QuestBuilderGraph->LastEditedDocuments.RemoveAt(EditedDocIdx);
					break;
				}
			}
		}
		else if (UEdGraph* OuterGraph = Cast<UEdGraph>(TestOuter))
		{
			// remove ourselves
			OuterGraph->Modify();
			OuterGraph->SubGraphs.Remove(GraphToRemove);
		}
		else if (!(Cast<UEdGraphNode>(TestOuter) && Cast<UEdGraphNode>(TestOuter)->GetSubGraphs().Num() > 0))
		{
			break;
		}
	}

	

	// Handle subgraphs held in graph
	TArray<UEdGraphNode*> AllNodes;
	GraphToRemove->GetNodesOfClass<UEdGraphNode>(AllNodes);

	for (UEdGraphNode* GraphNode : AllNodes)
	{
		for (UEdGraph* SubGraph : GraphNode->GetSubGraphs())
		{
			if (SubGraph && SubGraph->GetOuter()->IsA(UEdGraphNode::StaticClass()))
			{
				FQuestBuilderEditorUtils::RemoveGraph(QuestBuilderGraph, SubGraph);
			}
		}
	}

	GraphToRemove->GetSchema()->HandleGraphBeingDeleted(*GraphToRemove);

	GraphToRemove->Rename(nullptr, QuestBuilderGraph ? QuestBuilderGraph->GetOuter() : nullptr, REN_DoNotDirty | REN_DontCreateRedirectors);
	GraphToRemove->ClearFlags(RF_Standalone | RF_Public);
	GraphToRemove->RemoveFromRoot();

	
}

void FQuestBuilderEditorUtils::AddQuestGraphPage(UQuestBuilderGraph* QuestBuilderGraph, UEdGraph* Graph)
{
#if WITH_EDITORONLY_DATA
	QuestBuilderGraph->QuestGraphPages.Add(Graph);
	Graph->MarkPackageDirty();
#endif	//#if WITH_EDITORONLY_DATA
}


UEdGraph* FQuestBuilderEditorUtils::FindQuestGraph(const UQuestBuilderGraph* QuestBuilderGraph)
{

	for (UEdGraph* CurrentGraph : QuestBuilderGraph->QuestGraphPages)
	{
		if (CurrentGraph->GetFName().ToString().Contains(TEXT("QuestGraph")))
		{
			UE_LOG(LogTemp, Warning, TEXT("Current graph name: %s"), *CurrentGraph->GetFName().ToString());
			return CurrentGraph;
		}
	}

	return nullptr;
}

TArray<UObject*> FQuestBuilderEditorUtils::GetSelectionForPropertyEditor(const TSet<UObject*>& InSelection)
{
	TArray<UObject*> Selection;

	for (UObject* SelectionEntry : InSelection)
	{
		UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(SelectionEntry);
		if (QuestEdNode)
		{
			Selection.Add(QuestEdNode->NodeInstance);
			continue;
		}
		Selection.Add(SelectionEntry);
	}

	return Selection;
}


#undef LOCTEXT_NAMESPACE
