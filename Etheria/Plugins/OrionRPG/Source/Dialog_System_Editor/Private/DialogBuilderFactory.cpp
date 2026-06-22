// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilderFactory.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderEditor.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "DialogBuilderEditorUtils.h"
#include "OrionSetting.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformProcess.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "Dialog_Editor"

namespace
{
	constexpr int32 TrialGraphAssetLimit = 1;

	int32 CountDialogGraphAssets()
	{
		TArray<FAssetData> DialogGraphAssets;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UDialogBuilderGraph::StaticClass()), DialogGraphAssets, true);
		return DialogGraphAssets.Num();
	}

	void ShowDialogGraphTrialLimitNotification(const UOrionSetting* Settings)
	{
		FNotificationInfo Info(LOCTEXT("DialogGraphTrialLimitWarning", "Trial Version only allows 1 dialog graph asset."));
		Info.ExpireDuration = 5.0f;
		Info.bUseLargeFont = false;

		const FString PurchaseURL = Settings ? Settings->TrialPurchaseURL : FString();
		if (!PurchaseURL.IsEmpty())
		{
			Info.HyperlinkText = LOCTEXT("DialogGraphTrialLimitPurchaseLink", "Purchase full product");
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

	bool CanCreateDialogGraphAsset()
	{
		const UOrionSetting* Settings = GetDefault<UOrionSetting>();
		if (!Settings || !Settings->bTrialVersion)
		{
			return true;
		}

		if (CountDialogGraphAssets() < TrialGraphAssetLimit)
		{
			return true;
		}

		ShowDialogGraphTrialLimitNotification(Settings);
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

UDialogBuilderFactory::UDialogBuilderFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SupportedClass = UDialogBuilderGraph::StaticClass();
	DialogSystemGraphClass = UDialogBuilderGraph::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}


bool UDialogBuilderFactory::ConfigureProperties()
{
	return true;
}

UObject* UDialogBuilderFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UDialogBuilderGraph::StaticClass()));

	if (!CanCreateDialogGraphAsset())
	{
		return nullptr;
	}

	UDialogBuilderGraph* DialogGraph = nullptr;
	if (DialogSystemGraphClass)
	{
		DialogGraph =  NewObject<UDialogBuilderGraph>(InParent, DialogSystemGraphClass, Name, Flags | RF_Transactional);
		DialogGraph->ID = FDialogBuilderEditorUtils::FindUniqueDialogGraphName("DialogAsset_");
		
	}

	return DialogGraph;
}

FDialogGraphEditorSummoner::FDialogGraphEditorSummoner(TSharedPtr<class FDialogBuilderEditor> InDialogEditorPtr, FOnCreateGraphEditorWidget CreateGraphEditorWidgetCallback) : FDocumentTabFactoryForObjects<UEdGraph>(FDialogBuilderEditorTabs::ViewportID, InDialogEditorPtr)
, DialogEditorPtr(InDialogEditorPtr)
, OnCreateGraphEditorWidget(CreateGraphEditorWidgetCallback)
{
	TabLabel = LOCTEXT("DialogGraphLabel", "Dialog Graph");
}

void FDialogGraphEditorSummoner::OnTabActivated(TSharedPtr<SDockTab> Tab) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	DialogEditorPtr.Pin()->OnGraphEditorFocused(GraphEditor);
}

void FDialogGraphEditorSummoner::OnTabBackgrounded(TSharedPtr<SDockTab> Tab) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	DialogEditorPtr.Pin()->OnGraphEditorBackgrounded(GraphEditor);
}

void FDialogGraphEditorSummoner::OnTabRefreshed(TSharedPtr<SDockTab> Tab) const
{
	TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	GraphEditor->NotifyGraphChanged();
}

void FDialogGraphEditorSummoner::SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const
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

	if (Graph && DialogEditorPtr.Pin()->IsGraphInCurrentDialogGraph(Graph))
	{
		// Don't save references to external graphs.
		DialogEditorPtr.Pin()->GetDialogBuilderGraph()->LastEditedDocuments.Add(FEditedDocumentInfo(Graph, ViewLocation, ZoomAmount));
	}
}

TSharedRef<SWidget> FDialogGraphEditorSummoner::CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const
{
	check(Info.TabInfo.IsValid());
	return OnCreateGraphEditorWidget.Execute(Info.TabInfo.ToSharedRef(), DocumentID);
}

const FSlateBrush* FDialogGraphEditorSummoner::GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const
{
	return FDialogBuilderEditor::GetGlyphForGraph(DocumentID, false);
}


EAssetCommandResult UAssetDefinition_DialogEditor::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	const EToolkitMode::Type Mode = OpenArgs.ToolkitHost.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	for (UDialogBuilderGraph* DialogGraph : OpenArgs.LoadObjects<UDialogBuilderGraph>())
	{
		FDialog_System_EditorModule& DialogSystemEditorModule = FModuleManager::GetModuleChecked<FDialog_System_EditorModule>("Dialog_System_Editor");
		DialogSystemEditorModule.CheckClassCache();

		const TSharedRef<FDialogBuilderEditor> CustomObjectToolkit(new FDialogBuilderEditor());
		CustomObjectToolkit->Initialize(Mode, OpenArgs.ToolkitHost, DialogGraph);
	}

	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE

