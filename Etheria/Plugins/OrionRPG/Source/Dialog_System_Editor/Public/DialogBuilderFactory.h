// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "DialogBuilderGraph.h"
#include "Factories/Factory.h"
#include "UObject/Object.h"
#include "AssetDefinitionDefault.h"
#include "Dialog_System_Editor.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"
#include "DialogBuilder_EditorStyle.h"
#include "DialogBuilderFactory.generated.h"


#define LOCTEXT_NAMESPACE "DialogBuilderFactory"

/////////////////////////////////////////////////////
// FLocalKismetCallbacks

struct FLocalKismetCallbacks
{
	static FText GetObjectName(UObject* Object)
	{
		return (Object != NULL) ? FText::FromString(Object->GetName()) : LOCTEXT("UnknownObjectName", "UNKNOWN");
	}

	static FText GetGraphDisplayName(const UEdGraph* Graph)
	{
		if (Graph)
		{
			if (const UEdGraphSchema* Schema = Graph->GetSchema())
			{
				FGraphDisplayInfo Info;
				Schema->GetGraphDisplayInformation(*Graph, /*out*/ Info);

				return Info.DisplayName;
			}
			else
			{
				// if we don't have a schema, we're dealing with a malformed (or incomplete graph)...
				// possibly in the midst of some transaction - here we return the object's outer path 
				// so we can at least get some context as to which graph we're referring
				return FText::FromString(Graph->GetPathName());
			}
		}

		return LOCTEXT("UnknownGraphName", "UNKNOWN");
	}
};

struct FDialogGraphEditorSummoner : public FDocumentTabFactoryForObjects<UEdGraph>
{
public:
	DECLARE_DELEGATE_RetVal_TwoParams(TSharedRef<SGraphEditor>, FOnCreateGraphEditorWidget, TSharedRef<FTabInfo>, UEdGraph*);
public:
	FDialogGraphEditorSummoner(TSharedPtr<class FDialogBuilderEditor> InDialogEditorPtr, FOnCreateGraphEditorWidget CreateGraphEditorWidgetCallback);

	virtual void OnTabActivated(TSharedPtr<SDockTab> Tab) const override;

	virtual void OnTabBackgrounded(TSharedPtr<SDockTab> Tab) const override;

	virtual void OnTabRefreshed(TSharedPtr<SDockTab> Tab) const override;

	virtual void SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const override;

protected:
	virtual TAttribute<FText> ConstructTabNameForObject(UEdGraph* DocumentID) const override
	{
		// Extract the name of the graph from DocumentID (replace with your logic)
		FString GraphName = DocumentID->GetName();
		// Create FText with the extracted name
		FText TabName = FText::FromString("Dialog Graph");

		// Return TAttribute containing the constructed FText
		return TAttribute<FText>(TabName);
	}

	virtual TSharedRef<SWidget> CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const override;

	virtual const FSlateBrush* GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const override;

	//virtual TSharedRef<FGenericTabHistory> CreateTabHistoryNode(TSharedPtr<FTabPayload> Payload) override;

protected:
	TWeakPtr<class FDialogBuilderEditor> DialogEditorPtr;
	FOnCreateGraphEditorWidget OnCreateGraphEditorWidget;
};


/**
 * 
 */
UCLASS(MinimalAPI, hidecategories = Object)
class UDialogBuilderFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(EditAnywhere, Category = DataAsset)	
		TSubclassOf<UDialogBuilderGraph> DialogSystemGraphClass;

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

};


UCLASS()
class UAssetDefinition_DialogEditor : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:

	virtual FText GetAssetDisplayName() const override { return FText::FromString("Dialog Graph"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(48, 165, 171)); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UDialogBuilderGraph::StaticClass(); }

	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const FAssetCategoryPath Categories[] = { FText::FromString("Orion RPG") };
		return Categories;
	}

	virtual FAssetSupportResponse CanDuplicate(const FAssetData& InAsset) const override
	{
		return FAssetSupportResponse::NotSupported();
	}

	virtual const FSlateBrush* GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const override
	{
		return FDialogBuilder_EditorStyle::Get().GetBrush("ClassThumbnail.Dialog");
	}

	virtual const FSlateBrush* GetIconBrush(const FAssetData& InAssetData, const FName InClassName) const override
	{
		return FDialogBuilder_EditorStyle::Get().GetBrush("ClassIcon.Dialog");
	}

	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};


#undef LOCTEXT_NAMESPACE