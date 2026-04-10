// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "DialogBuilderEdGraph.generated.h"

class UDialogBuilderGraph;
class UDialogBuilderNode;
class UDialogBuilderEdge;
class UDialogBuilderEdNode;
class UDialogBuilderEdNode_Edge;
class UDialogBuilderEdNode_Edge;

UCLASS()
class DIALOG_SYSTEM_EDITOR_API UDialogBuilderEdGraph : public UEdGraph
{
	GENERATED_BODY()
public:
	UDialogBuilderEdGraph();
	virtual ~UDialogBuilderEdGraph();

	virtual void UpdateAsset(bool bForce = false);
	virtual void SortAllChildrenNodes(bool bForce = false);
	virtual void UpdateAllSubnodeDependencies();
	virtual void RebuildChildOrder(UEdGraphNode* ParentNode);
	virtual void OnSubNodeDropped();

	bool UpdateUnknownNodeClasses();
	void UpdateDeprecatedClasses();
	void UpdateClassData();
	void RemoveUnknownSubNodes();

	bool IsLocked() const;
	void LockUpdates();
	void UnlockUpdates();

	UDialogBuilderGraph* GetDialogBuilderGraph() const;

	virtual bool Modify(bool bAlwaysMarkDirty = true) override;

	class SGraphEditor* SEditorGraph;
	
	/** Dialog linked with this UEdGraph */
	UPROPERTY(VisibleAnywhere, Instanced, Category = "DialogBuilderGraph")
	TObjectPtr<UDialogBuilderGraph> OwningDialog;

	UPROPERTY(Transient)
		TMap< TObjectPtr<UDialogBuilderNode>, TObjectPtr<UDialogBuilderEdNode>> NodeMap;

	UPROPERTY(Transient)
		TMap< TObjectPtr<UDialogBuilderEdge>, TObjectPtr<UDialogBuilderEdNode_Edge>> EdgeMap;

	void Clear();

	void OnPackageMarkedDirty(UPackage* ModifiedPackage, bool bWasDirty);
protected:
	/** if set, graph modifications won't cause updates in internal tree structure
	 *  flag allows freezing update during heavy changes like pasting new nodes
	 */
	uint32 bLockUpdates : 1;

	void SortNodes(UDialogBuilderNode* RootNode);
	double LastRebuildTime = 0.f;
	double LastSortTime = 0.f;

#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif
protected:
	virtual void CollectAllNodeInstances(TSet<UObject*>& NodeInstances);
	void UpdateVersion_UnifiedSubNodes();
};
