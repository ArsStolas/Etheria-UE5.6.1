// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "AIGraphTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/NoExportTypes.h"
#include "DialogBuilderEdGraph.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderEdNode.generated.h"

class UEdNode_DialogSystemEdge;
class UDialogBuilderEdGraph;
class SGraphNode_DialogBuilderNode;

UCLASS()
class DIALOG_SYSTEM_EDITOR_API UDialogBuilderEdNode : public UEdGraphNode
{
	GENERATED_BODY()
public:
	UDialogBuilderEdNode();
	virtual ~UDialogBuilderEdNode();

	/** only some of dialog nodes support decorators */
	UPROPERTY()
	TArray<TObjectPtr<UDialogBuilderEdNode>> Decorators;

	/** only some of dialog nodes support events */
	UPROPERTY()
	TArray<TObjectPtr<UDialogBuilderEdNode>> Events;


	/** instance class */
	UPROPERTY()
	struct FGraphNodeClassData ClassData;

	UPROPERTY()
	TObjectPtr<UObject> NodeInstance;
	
	UPROPERTY(transient)
	TObjectPtr<UDialogBuilderEdNode> ParentNode;

	UPROPERTY()
	TArray<TObjectPtr<UDialogBuilderEdNode>> SubNodes;

	UDialogBuilderEdGraph* GetDialogBuilderEdGraph();

	SGraphNode_DialogBuilderNode* SGraphNode;

	/** subnode index assigned during copy operation to connect nodes again on paste */
	UPROPERTY()
	int32 CopySubNodeIndex;

	/** if set, all modifications (including delete/cut) are disabled */
	UPROPERTY()
	uint32 bIsReadOnly : 1;
	
	/** if set, this node will be always considered as subnode */
	UPROPERTY()
	uint32 bIsSubNode : 1;

	/** error message for node */
	UPROPERTY()
	FString ErrorMessage;

	//UedGraphnode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void DestroyNode() override;
	virtual bool CanDuplicateNode() const override;
	virtual void PostPlacedNewNode() override;
	virtual void PostPasteNode() override;
	virtual bool CanUserDeleteNode() const override;
	virtual void PrepareForCopying() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual void FindDiffs(class UEdGraphNode* OtherNode, struct FDiffResults& Results) override;
	//end UedGraphnode interface

	void UpdateSubnodeDependencies();
	void AddSubNode(UDialogBuilderEdNode* SubNode, class UEdGraph* ParentGraph);
	void RemoveSubNode(UDialogBuilderEdNode* SubNode);
	virtual void RemoveAllSubNodes();
	virtual void OnSubNodeRemoved(UDialogBuilderEdNode* SubNode);
	virtual void OnSubNodeAdded(UDialogBuilderEdNode* SubNode);

	virtual int32 FindSubNodeDropIndex(UDialogBuilderEdNode* SubNode) const;
	virtual void InsertSubNodeAt(UDialogBuilderEdNode* SubNode, int32 DropIndex);

	/** check if node is subnode */
	virtual bool IsSubNode() const;

	/** reinitialize node instance */
	virtual bool RefreshNodeClass();

	/** updates ClassData from node instance */
	virtual void UpdateNodeClassData();

	/** gets icon resource name for title bar */
	virtual FName GetNameIcon() const;

	virtual void FindUniqueNodeName(const FString& InBaseName);

	virtual FText GetDescription() const;
	virtual FText GetEventsText() const;
	virtual FText GetConditionsText() const;

	virtual FLinearColor GetBackgroundColor() const;
	virtual UEdGraphPin* GetInputPin() const;
	virtual UEdGraphPin* GetOutputPin() const;

	/** initialize instance object  */
	virtual void InitializeInstance();

	/** Check if node instance uses blueprint for its implementation */
	bool UsesBlueprint() const;

	/**
	 * Checks for any errors in this node and updates ErrorMessage with any resulting message
	 * Called every time the graph is serialized (i.e. loaded, saved, execution index changed, etc)
	 */
	virtual void UpdateErrorMessage();

	/** check if node has any errors, used for assigning colors on graph */
	virtual bool HasErrors() const;

	static void UpdateNodeClassDataFrom(UClass* InstanceClass, FGraphNodeClassData& UpdatedData);

	virtual void PostCopyNode();

	virtual void ResetNodeOwner();
protected:
	/** creates add decorator... submenu */
	void CreateAddDecoratorSubMenu(class UToolMenu* Menu, UEdGraph* Graph) const;

	/** creates add event... submenu */
	void CreateAddEventSubMenu(class UToolMenu* Menu, UEdGraph* Graph) const;

	/** add right click menu to create subnodes: Decorators */
	void AddContextMenuActionsDecorators(class UToolMenu* Menu, const FName SectionName, class UGraphNodeContextMenuContext* Context) const;

	/** add right click menu to create subnodes: Events */
	void AddContextMenuActionsEvents(class UToolMenu* Menu, const FName SectionName, class UGraphNodeContextMenuContext* Context) const;
	



public:
#if WITH_EDITOR
	virtual void PostEditImport() override;
	virtual void PostEditUndo() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

};
