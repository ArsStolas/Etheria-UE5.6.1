// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "QuestBuilderFactory.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderEditor.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "Quest.h"
#include "QuestBuilderEditorUtils.h"
#include "QuestBuilderEdGraph.h"
#include "OrionSetting.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformProcess.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "Quest_Editor"

namespace
{
	constexpr int32 TrialGraphAssetLimit = 1;

	int32 CountQuestGraphAssets()
	{
		TArray<FAssetData> QuestGraphAssets;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UQuestBuilderGraph::StaticClass()), QuestGraphAssets, true);
		return QuestGraphAssets.Num();
	}

	void ShowQuestGraphTrialLimitNotification(const UOrionSetting* Settings)
	{
		FNotificationInfo Info(LOCTEXT("QuestGraphTrialLimitWarning", "Trial Version only allows 1 quest graph asset."));
		Info.ExpireDuration = 5.0f;
		Info.bUseLargeFont = false;

		const FString PurchaseURL = Settings ? Settings->TrialPurchaseURL : FString();
		if (!PurchaseURL.IsEmpty())
		{
			Info.HyperlinkText = LOCTEXT("QuestGraphTrialLimitPurchaseLink", "Purchase full product");
			Info.Hyperlink = FSimpleDelegate::CreateLambda([PurchaseURL]()
			{
				FPlatformProcess::LaunchURL(*PurchaseURL, nullptr, nullptr);
			});
		}

		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(SNotificationItem::CS_Fail);
		}
	}

	bool CanCreateQuestGraphAsset()
	{
		const UOrionSetting* Settings = GetDefault<UOrionSetting>();
		if (!Settings || !Settings->bTrialVersion)
		{
			return true;
		}

		if (CountQuestGraphAssets() < TrialGraphAssetLimit)
		{
			return true;
		}

		ShowQuestGraphTrialLimitNotification(Settings);
		return false;
	}
}

class FAssetClassParentFilter : public IClassViewerFilter
{
public:
	FAssetClassParentFilter()
		: DisallowedClassFlags(CLASS_None), bDisallowBlueprintBase(false)
	{}

	/** All children of these classes will be included unless filtered out by another setting. */
	TSet< const UClass* > AllowedChildrenOfClasses;

	/** Disallowed class flags. */
	EClassFlags DisallowedClassFlags;

	/** Disallow blueprint base classes. */
	bool bDisallowBlueprintBase;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		bool bAllowed = !InClass->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;

		if (bAllowed && bDisallowBlueprintBase)
		{
			if (FKismetEditorUtilities::CanCreateBlueprintOfClass(InClass))
			{
				return false;
			}
		}

		return bAllowed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		if (bDisallowBlueprintBase)
		{
			return false;
		}

		return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
	}
};

UQuestBuilderFactory::UQuestBuilderFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SupportedClass = UQuestBuilderGraph::StaticClass();
	QuestSystemGraphClass = UQuestBuilderGraph::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}


bool UQuestBuilderFactory::ConfigureProperties()
{
	return true;
}

UObject* UQuestBuilderFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UQuestBuilderGraph::StaticClass()));

	if (!CanCreateQuestGraphAsset())
	{
		return nullptr;
	}

	UQuestBuilderGraph* QuestGraph = nullptr;
	if (QuestSystemGraphClass)
	{
		QuestGraph =  NewObject<UQuestBuilderGraph>(InParent, QuestSystemGraphClass, Name, Flags | RF_Transactional);
		QuestGraph->ID = FQuestBuilderEditorUtils::FindUniqueQuestGraphName("QuestAsset_");
		
	}

	return QuestGraph;
}

FQuestGraphEditorSummoner::FQuestGraphEditorSummoner(TSharedPtr<class FQuestBuilderEditor> InQuestEditorPtr, FOnCreateGraphEditorWidget CreateGraphEditorWidgetCallback) : FDocumentTabFactoryForObjects<UEdGraph>(FQuestBuilderEditorTabs::ViewportID, InQuestEditorPtr)
, QuestEditorPtr(InQuestEditorPtr)
, OnCreateGraphEditorWidget(CreateGraphEditorWidgetCallback)
{
}

void FQuestGraphEditorSummoner::OnTabActivated(TSharedPtr<SDockTab> Tab) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	QuestEditorPtr.Pin()->OnGraphEditorFocused(GraphEditor);
}

void FQuestGraphEditorSummoner::OnTabBackgrounded(TSharedPtr<SDockTab> Tab) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	QuestEditorPtr.Pin()->OnGraphEditorBackgrounded(GraphEditor);
}

void FQuestGraphEditorSummoner::OnTabRefreshed(TSharedPtr<SDockTab> Tab) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	GraphEditor->NotifyGraphChanged();
}

void FQuestGraphEditorSummoner::SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
	FVector2f ViewLocation;
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5)
	FVector2D ViewLocation;
#endif
	float ZoomAmount;
	GraphEditor->GetViewLocation(ViewLocation, ZoomAmount);

	UEdGraph* Graph = Payload->IsValid() ? FTabPayload_UObject::CastChecked<UEdGraph>(Payload) : nullptr;

	if (Graph && QuestEditorPtr.Pin()->IsGraphInCurrentQuestGraph(Graph))
	{
		// Don't save references to external graphs.
		QuestEditorPtr.Pin()->GetQuestBuilderGraph()->LastEditedDocuments.Add(FEditedDocumentInfo(Graph, ViewLocation, ZoomAmount));
	}
}

TAttribute<FText> FQuestGraphEditorSummoner::ConstructTabNameForObject(UEdGraph* DocumentID) const
{
	UQuestBuilderEdGraph* QuestGraph = Cast<UQuestBuilderEdGraph>(DocumentID);
	UQuest* Quest = QuestGraph ? QuestGraph->Quest : nullptr;
	if (Quest)
	{
		return Quest->QuestName;
	}
	FString GraphName = DocumentID->GetName();
	// Create FText with the extracted name
	FText TabName = FText::FromString(GraphName);

	// Return TAttribute containing the constructed FText
	return TAttribute<FText>(TabName);
}

TSharedRef<SWidget> FQuestGraphEditorSummoner::CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const
{
	check(Info.TabInfo.IsValid());
	return OnCreateGraphEditorWidget.Execute(Info.TabInfo.ToSharedRef(), DocumentID);
}

const FSlateBrush* FQuestGraphEditorSummoner::GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const
{
	return FQuestBuilderEditor::GetGlyphForGraph(DocumentID, false);
}


EAssetCommandResult UAssetDefinition_QuestEditor::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	const EToolkitMode::Type Mode = OpenArgs.ToolkitHost.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	for (UQuestBuilderGraph* QuestGraph : OpenArgs.LoadObjects<UQuestBuilderGraph>())
	{
		FQuest_System_EditorModule& QuestSystemEditorModule = FModuleManager::GetModuleChecked<FQuest_System_EditorModule>("Quest_System_Editor");
		QuestSystemEditorModule.CheckClassCache();

		const TSharedRef<FQuestBuilderEditor> CustomObjectToolkit(new FQuestBuilderEditor());
		CustomObjectToolkit->Initialize(Mode, OpenArgs.ToolkitHost, QuestGraph);
	}

	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE

