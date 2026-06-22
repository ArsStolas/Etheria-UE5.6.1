// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "QuestBuilderEditor.h"
#include "QuestBuilderNode_Root.h"
#include "Decorator/OrionDecorator.h"
#include "EngineGlobals.h"
#include "Editor/EditorEngine.h"
#include "Editor.h"
#include "UnrealEdGlobals.h"
#include "Event/OrionEvent.h"
#include "BlueprintEditor.h"
#include "UObject/ObjectSaveContext.h"
#include "GraphEditorActions.h"
#include "Framework/Commands/GenericCommands.h"
#include "QuestBuilderEditorToolbar.h"
#include "QuestBuilder_EditorCommands.h"
#include "QuestBuilderEdGraph.h"
#include "QuestBuilderEdNode.h"
#include "QuestBuilderNode_Objective.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node.h"
#include "GraphEditAction.h"
#include "QuestBuilderEdNode_Edge.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "EdGraphSchema_QuestBuilder.h"
#include "QuestBuilderEditorUtils.h"
#include "QuestBuilderFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SMyQuest.h"
#include "Quest_System_Editor.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserFrontEndFilterExtension.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include <Kismet2/KismetEditorUtilities.h>

#define LOCTEXT_NAMESPACE "QuestBuilderEditor"

const FName FQuestBuilderEditorTabs::MyQuestDetailID(TEXT("MyQuestDetail"));
const FName FQuestBuilderEditorTabs::QuestBuilderPropertyID(TEXT("QuestBuilderProperty"));
const FName FQuestBuilderEditorTabs::ViewportID(TEXT("Viewport"));
const FName FQuestBuilderEditorTabs::QuestBuilderEditorSettingsID(TEXT("QuestBuilderEditorSettings"));

const FName QuestBuilderEditorAppName = FName(TEXT("QuestBuilderEditorApp"));

//////////////////////////////////////////////////////////////////////////
FQuestBuilderEditor::FQuestBuilderEditor()
{
	EditingQuestGraph = nullptr;

#if ENGINE_MAJOR_VERSION < 5
	OnPackageSavedDelegateHandle = UPackage::PackageSavedEvent.AddRaw(this, &FQuestBuilderEditor::OnPackageSaved);
#else // #if ENGINE_MAJOR_VERSION < 5
	OnPackageSavedDelegateHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FQuestBuilderEditor::OnPackageSavedWithContext);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5
}

FQuestBuilderEditor::~FQuestBuilderEditor()
{
#if ENGINE_MAJOR_VERSION < 5
	UPackage::PackageSavedEvent.Remove(OnPackageSavedDelegateHandle);
#else // #if ENGINE_MAJOR_VERSION < 5
	UPackage::PackageSavedWithContextEvent.Remove(OnPackageSavedDelegateHandle);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5
}

void FQuestBuilderEditor::Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InObject)
{
	UQuestBuilderGraph* QuestGraphToEdit = Cast<UQuestBuilderGraph>(InObject);

	if (QuestGraphToEdit != nullptr)
	{
		EditingQuestGraph = QuestGraphToEdit;
	}

	//Binding Functionality
	EditingQuestGraph->GetOutermost()->PackageMarkedDirtyEvent.AddRaw(this, &FQuestBuilderEditor::OnPackageMarkedDirty);

	TSharedPtr<FQuestBuilderEditor> ThisPtr(SharedThis(this));
	if (!DocumentManager.IsValid())
	{
		DocumentManager = MakeShareable(new FDocumentTracker);
		DocumentManager->Initialize(ThisPtr);

		// Register the document factories
		{
			TSharedRef<FDocumentTabFactory> GraphEditorFactory = MakeShareable(new FQuestGraphEditorSummoner(ThisPtr,
				FQuestGraphEditorSummoner::FOnCreateGraphEditorWidget::CreateSP(this, &FQuestBuilderEditor::CreateGraphEditorWidget)
			));

			// Also store off a reference to the grapheditor factory so we can find all the tabs spawned by it later.
			QuestEditorTabFactoryPtr = GraphEditorFactory;
			DocumentManager->RegisterDocumentFactory(GraphEditorFactory);
		}
	}

	TArray<UObject*> ObjectsToEdit;
	if (EditingQuestGraph != nullptr)
	{
		ObjectsToEdit.Add(EditingQuestGraph);
	}

	//Make Toolbar
	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FQuestBuilderEditorToolbar(SharedThis(this)));
	}

	// if we are already editing objects, dont try to recreate the editor from scratch but update the list of objects in edition
	// ex: BehaviorTree may want to reuse an editor already opened for its associated Blackboard asset.
	const TArray<UObject*>* EditedObjects = GetObjectsCurrentlyBeingEdited();
	if (EditedObjects == nullptr || EditedObjects->Num() == 0)
	{
		FGenericCommands::Register();
		FGraphEditorCommands::Register();
		FMyQuestCommands::Register();
		FQuestBuilder_EditorCommands::Register();

		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

		ToolbarBuilder->AddQuestSystemToolbar(ToolbarExtender);

		BindCommands();
		CreateInternalWidgets();

		// Layout
		const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_QuestBuilderEditor_Layout_v1")
			->AddArea
			(
				FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
#if ENGINE_MAJOR_VERSION < 5
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.1f)
					->AddTab(GetToolbarTabId(), ETabState::OpenedTab)->SetHideTabWell(true)
				)
#endif // #if ENGINE_MAJOR_VERSION < 5
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)->SetSizeCoefficient(0.9f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.2f)
						->AddTab(FQuestBuilderEditorTabs::MyQuestDetailID, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.55f)
						->AddTab("Document", ETabState::ClosedTab)
					)
					->Split
					(
						FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)->SetSizeCoefficient(0.25f)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.55f)
							->AddTab(FQuestBuilderEditorTabs::QuestBuilderPropertyID, ETabState::OpenedTab)
						)
					)
				)
			);

		const bool bCreateDefaultStandaloneMenu = true;
		const bool bCreateDefaultToolbar = true;
		InitAssetEditor(Mode, InitToolkitHost, QuestBuilderEditorAppName, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, EditingQuestGraph, false);


		if (EditingQuestGraph && EditingQuestGraph->bIsNewlyCreated)
		{
			NewDocument_OnClicked(CGT_NewQuestGraph);
			EditingQuestGraph->bIsNewlyCreated = false;
			EditingQuestGraph->Modify();
		}
		else
		{
			if (GetQuestBuilderGraph()->QuestGraphPages.Num() > 0)
			{
				OpenDocument(GetQuestBuilderGraph()->QuestGraphPages[0], FDocumentTracker::OpenNewDocument);
				
				//bind on graph changes
				for (UEdGraph* EdGraph : EditingQuestGraph->QuestGraphPages)
				{
					if (EdGraph)
					{
						if (UQuestBuilderEdGraph* QuestGraph = Cast<UQuestBuilderEdGraph>(EdGraph))
						{
							QuestGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FQuestBuilderEditor::OnGraphChanged));
						}
					}
				}
			}
		}
	}
	else
	{
		for (UObject* ObjectToEdit : ObjectsToEdit)
		{
			if (!EditedObjects->Contains(ObjectToEdit))
			{
				AddEditingObject(ObjectToEdit);
			}
		}
	}


	RegenerateMenusAndToolbars();
	RebuildQuestBuilderGraphPages();
}


void FQuestBuilderEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	DocumentManager->SetTabManager(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_QuestBuilderEditor", "Generic Graph Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FQuestBuilderEditorTabs::MyQuestDetailID, FOnSpawnTab::CreateSP(this, &FQuestBuilderEditor::SpawnTab_MyQuest))
		.SetDisplayName(LOCTEXT("Quest Details", "My Quest"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FQuestBuilderEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FQuestBuilderEditor::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("GraphCanvasTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(FQuestBuilderEditorTabs::QuestBuilderPropertyID, FOnSpawnTab::CreateSP(this, &FQuestBuilderEditor::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Property"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FQuestBuilderEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FQuestBuilderEditorTabs::MyQuestDetailID);
	InTabManager->UnregisterTabSpawner(FQuestBuilderEditorTabs::ViewportID);
	InTabManager->UnregisterTabSpawner(FQuestBuilderEditorTabs::QuestBuilderPropertyID);
	InTabManager->UnregisterTabSpawner(FQuestBuilderEditorTabs::QuestBuilderEditorSettingsID);
}

void FQuestBuilderEditor::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
		// Clear selection, to avoid holding refs to nodes that go away
		if (TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor())
		{
			CurrentGraphEditor->ClearSelectionSet();
			CurrentGraphEditor->NotifyGraphChanged();
		}
		FSlateApplication::Get().DismissAllMenus();
	}
}

void FQuestBuilderEditor::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{
		// Clear selection, to avoid holding refs to nodes that go away
		if (TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor())
		{
			CurrentGraphEditor->ClearSelectionSet();
			CurrentGraphEditor->NotifyGraphChanged();
		}
		FSlateApplication::Get().DismissAllMenus();
	}
}

void FQuestBuilderEditor::BindCommands()
{
	ToolkitCommands->MapAction(FQuestBuilder_EditorCommands::Get().NewObjective,
		FExecuteAction::CreateSP(this, &FQuestBuilderEditor::CreateNewObjective),
		FCanExecuteAction::CreateSP(this, &FQuestBuilderEditor::CanCreateNewObjective),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FQuestBuilderEditor::IsNewObjectiveButtonVisible)
	);

	ToolkitCommands->MapAction(FQuestBuilder_EditorCommands::Get().NewQuestDecorator,
		FExecuteAction::CreateSP(this, &FQuestBuilderEditor::CreateNewQuestDecorator),
		FCanExecuteAction::CreateSP(this, &FQuestBuilderEditor::CanCreateQuestDecorator)
	);

	ToolkitCommands->MapAction(FQuestBuilder_EditorCommands::Get().NewQuestEvent,
		FExecuteAction::CreateSP(this, &FQuestBuilderEditor::CreateNewQuestEvent),
		FCanExecuteAction::CreateSP(this, &FQuestBuilderEditor::CanCreateQuestEvent)
	); 
	
	ToolkitCommands->MapAction(FQuestBuilder_EditorCommands::Get().QuestSetting,
		FExecuteAction::CreateSP(this, &FQuestBuilderEditor::OpenQuestSetting),
		FCanExecuteAction::CreateSP(this, &FQuestBuilderEditor::CanOpenQuestSetting)
	);
	
	ToolkitCommands->MapAction(FQuestBuilder_EditorCommands::Get().AddNewQuestGraph,
		FExecuteAction::CreateSP(this, &FQuestBuilderEditor::NewDocument_OnClicked, CGT_NewQuestGraph),
		FCanExecuteAction::CreateSP(this, &FQuestBuilderEditor::InEditingMode),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FQuestBuilderEditor::NewDocument_IsVisibleForType, CGT_NewQuestGraph)
	);

}

void FQuestBuilderEditor::CreateCommandList()
{
	if (GraphEditorCommands.IsValid()) {
		return;
	}

	GraphEditorCommands = MakeShareable(new FUICommandList);
	// Can't use CreateSP here because derived editor are already implementing TSharedFromThis<FAssetEditorToolkit>
	// however it should be safe, since commands are being used only within this editor
	// if it ever crashes, this function will have to go away and be reimplemented in each derived class

	GraphEditorCommands->MapAction(FQuestBuilder_EditorCommands::Get().AddNewQuestGraph,
		FExecuteAction::CreateSP(this, &FQuestBuilderEditor::NewDocument_OnClicked, CGT_NewQuestGraph),
		FCanExecuteAction::CreateSP(this, &FQuestBuilderEditor::InEditingMode),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FQuestBuilderEditor::NewDocument_IsVisibleForType, CGT_NewQuestGraph)
	);

	GraphEditorCommands->MapAction(FQuestBuilder_EditorCommands::Get().NewObjective,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CreateNewObjective),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanCreateNewObjective));

	GraphEditorCommands->MapAction(FQuestBuilder_EditorCommands::Get().NewQuestDecorator,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CreateNewQuestDecorator),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanCreateQuestDecorator));
	
	GraphEditorCommands->MapAction(FQuestBuilder_EditorCommands::Get().NewQuestEvent,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CreateNewQuestEvent),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanCreateQuestEvent));

	GraphEditorCommands->MapAction(FQuestBuilder_EditorCommands::Get().QuestSetting,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::OpenQuestSetting),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanOpenQuestSetting));


	GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::SelectAllNodes),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanSelectAllNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanDeleteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanCopyNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanCutNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanPasteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanDuplicateNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &FQuestBuilderEditor::OnRenameNode),
		FCanExecuteAction::CreateSP(this, &FQuestBuilderEditor::CanRenameNodes)
	);

	GraphEditorCommands->MapAction(
		FGraphEditorCommands::Get().CreateComment,
		FExecuteAction::CreateRaw(this, &FQuestBuilderEditor::OnCreateComment),
		FCanExecuteAction::CreateRaw(this, &FQuestBuilderEditor::CanCreateComment)
	);
}

TSharedPtr<SGraphEditor> FQuestBuilderEditor::GetCurrGraphEditor() const
{
	return CurrentGraphWidget;
}

FGraphPanelSelectionSet FQuestBuilderEditor::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = GetCurrGraphEditor();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}

	return CurrentSelection;
}

FName FQuestBuilderEditor::GetToolkitFName() const
{
	return FName("FQuestGraphEditor");
}

FText FQuestBuilderEditor::GetBaseToolkitName() const
{
	return LOCTEXT("QuestGraphEditorAppLabel", "Quest Graph Editor");
}


FText FQuestBuilderEditor::GetToolkitName() const
{
	const bool bDirtyState = EditingQuestGraph->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("QuestGraphName"), FText::FromString(EditingQuestGraph->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("QuestGraphEditorToolkitName", "{QuestGraphName}{DirtyState}"), Args);
}

FText FQuestBuilderEditor::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolTipTextForObject(EditingQuestGraph);
}

FLinearColor FQuestBuilderEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::Blue;
}

FString FQuestBuilderEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("QuestGraphEditor");
}

FString FQuestBuilderEditor::GetDocumentationLink() const
{
	//make documentation from notion add this link
	return TEXT("");
}

void FQuestBuilderEditor::SaveAsset_Execute()
{
	FAssetEditorToolkit::SaveAsset_Execute();

}

void FQuestBuilderEditor::RefreshEditors()
{
	
}

void FQuestBuilderEditor::RefreshMyQuest()
{
}

void FQuestBuilderEditor::RefreshInspector()
{
}

void FQuestBuilderEditor::AddToSelection(UEdGraphNode* InNode)
{
}

void FQuestBuilderEditor::JumpToHyperlink(const UObject* ObjectReference, bool bRequestRename)
{
	//SetCurrentMode(FBlueprintEditorApplicationModes::StandardBlueprintEditorMode);
	if (const UEdGraph* Graph = Cast<const UEdGraph>(ObjectReference))
	{
		// Navigating into things should re-use the current tab when it makes sense
		FDocumentTracker::EOpenDocumentCause OpenMode = FDocumentTracker::OpenNewDocument;
		if ((Graph->GetSchema()->GetGraphType(Graph) == GT_Ubergraph) /*|| Cast<UK2Node>(Graph->GetOuter())*/ || Cast<UEdGraph>(Graph-> GetOuter()))
		{
			// Ubergraphs directly reuse the current graph
			OpenMode = FDocumentTracker::NavigatingCurrentDocument;
		}
		else
		{
			// Walk up the outer chain to see if any tabs have a parent of this document open for edit, and if so
			// we should reuse that one and drill in deeper instead
			for (UObject* WalkPtr = const_cast<UEdGraph*>(Graph); WalkPtr != nullptr; WalkPtr = WalkPtr->GetOuter())
			{
				TArray< TSharedPtr<SDockTab> > TabResults;
				if (FindOpenTabsContainingDocument(WalkPtr, /*out*/ TabResults))
				{
					// See if the parent was active
					bool bIsActive = false;
					for (TSharedPtr<SDockTab> Tab : TabResults)
					{
						if (Tab->IsActive())
						{
							bIsActive = true;
							break;
						}
					}

					if (bIsActive)
					{
						OpenMode = FDocumentTracker::NavigatingCurrentDocument;
						break;
					}
				}
			}
		}

		// Force it to open in a new document if shift is pressed
		const bool bIsShiftPressed = FSlateApplication::Get().GetModifierKeys().IsShiftDown();
		if (bIsShiftPressed)
		{
			OpenMode = FDocumentTracker::ForceOpenNewDocument;
		}

		// Open the document
		OpenDocument(Graph, OpenMode);
	}

	else
	{
		UE_LOG(LogBlueprint, Warning, TEXT("Unknown type of hyperlinked object (%s), cannot focus it"), *GetNameSafe(ObjectReference));
	}

	//@TODO: Hacky way to ensure a message is seen when hitting an exception and doing intraframe debugging
	const FText ExceptionMessage = FKismetDebugUtilities::GetAndClearLastExceptionMessage();
	if (!ExceptionMessage.IsEmpty())
	{
		LogSimpleMessage(ExceptionMessage);
	}
}

void FQuestBuilderEditor::JumpToPin(const UEdGraphPin* Pin)
{
}

void FQuestBuilderEditor::SummonSearchUI(bool bSetFindWithinBlueprint, FString NewSearchTerms, bool bSelectFirstResult)
{
}

void FQuestBuilderEditor::SummonFindAndReplaceUI()
{
}

TSharedPtr<SGraphEditor> FQuestBuilderEditor::OpenGraphAndBringToFront(UEdGraph* Graph, bool bSetFocus)
{
	return TSharedPtr<SGraphEditor>();
}

void FQuestBuilderEditor::UpdateToolbar()
{
}

void FQuestBuilderEditor::RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FQuestBuilderEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	/*if (GetObjectsCurrentlyBeingEdited()->Num() > 0)
	{
		TArray<UObject*>& LocalEditingObjects = const_cast<TArray<UObject*>&>(GetEditingObjects());

		Collector.AddReferencedObjects(LocalEditingObjects);
	}

	Collector.AddReferencedObject(EditingQuestGraph);*/
}

FString FQuestBuilderEditor::GetReferencerName() const
{
	return FString();
}

UQuestBuilderGraph* FQuestBuilderEditor::GetQuestBuilderGraph() const
{
	return EditingQuestGraph;
}

bool FQuestBuilderEditor::NewDocument_IsVisibleForType(ECreatedQuestDocumentType GraphType) const
{
	return true;
}

void FQuestBuilderEditor::NewDocument_OnClicked(ECreatedQuestDocumentType GraphType)
{
	FText DocumentNameText;
	bool bResetMyBlueprintFilter = false;

	switch (GraphType)
	{
	case CGT_NewQuestGraph:
		DocumentNameText = LOCTEXT("NewDocQuestName", "QUEST_");
		bResetMyBlueprintFilter = true;
		break;
	
	default:
		DocumentNameText = LOCTEXT("NewDocNewName", "NewDocument");
		break;
	}

	FName DocumentName = FName(*DocumentNameText.ToString());

	

	// Make sure the new name is valid
	DocumentName = FQuestBuilderEditorUtils::FindUniqueQuestName(DocumentNameText.ToString());
		
	//check(IsEditingSingleBlueprint());

	const FScopedTransaction Transaction(LOCTEXT("AddNewQuestGraph", "Add New Quest Graph"));
	GetQuestBuilderGraph()->Modify();

	UEdGraph* NewGraph = nullptr;

	
	if (GraphType == CGT_NewQuestGraph)
	{
		NewGraph = FQuestBuilderEditorUtils::CreateNewGraph(GetQuestBuilderGraph(), DocumentName, UQuestBuilderEdGraph::StaticClass(), UEdGraphSchema_QuestBuilder::StaticClass());
		NewGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FQuestBuilderEditor::OnGraphChanged));
		FQuestBuilderEditorUtils::AddQuestGraphPage(GetQuestBuilderGraph(), NewGraph);

	}
	else
	{
		ensureMsgf(false, TEXT("GraphType is invalid"));
	}

	// Now open the new graph
	if (NewGraph)	
	{
		OpenDocument(NewGraph, FDocumentTracker::OpenNewDocument);

		//RenameNewlyAddedAction(DocumentName);
	}
	else
	{
		LogSimpleMessage(LOCTEXT("AddDocument_Error", "Adding new document failed."));
	}
}

bool FQuestBuilderEditor::InEditingMode() const
{
	return true;
}

TSharedPtr<SDockTab> FQuestBuilderEditor::OpenDocument(const UObject* DocumentID, FDocumentTracker::EOpenDocumentCause Cause)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);
	return DocumentManager->OpenDocument(Payload, Cause);
}

void FQuestBuilderEditor::CloseDocumentTab(const UObject* DocumentID)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);
	DocumentManager->CloseTab(Payload);
}

void FQuestBuilderEditor::RenameNewlyAddedAction(FName InActionName)
{
	if (MyQuestWidget.IsValid())
	{
		// Force a refresh immediately, the item has to be present in the list for the rename requests to be successful.
		MyQuestWidget->Refresh();
		MyQuestWidget->SelectItemByName(InActionName, ESelectInfo::OnMouseClick);
		MyQuestWidget->OnRequestRenameOnActionNode();
	}
}

void FQuestBuilderEditor::LogSimpleMessage(const FText& MessageText)
{
	FNotificationInfo Info(MessageText);
	Info.ExpireDuration = 3.0f;
	Info.bUseLargeFont = false;
	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Fail);
	}
}

TSharedRef<SWidget> FQuestBuilderEditor::CreateGraphTitleBarWidget(TSharedRef<FTabInfo> InTabInfo, UEdGraph* InGraph)
{
   // Create a horizontal box to serve as the title bar
	return SNew(SBox);
		
		//SNew(SHorizontalBox)
		//+ SHorizontalBox::Slot()
		//.FillWidth(1.0f)
		//.VAlign(VAlign_Center)
		//.Padding(FMargin(5.0f, 0.0f))
		//[
		//	// Add the graph name as a text block
		//	SNew(STextBlock)
		//		.Text(FText::FromString(InGraph->GetName()))
		//		.TextStyle(FAppStyle::Get(), "GraphBreadcrumbButtonText")
		//];
}


const FSlateBrush* FQuestBuilderEditor::GetGlyphForGraph(const UEdGraph* Graph, bool bInLargeIcon)
{
	const FSlateBrush* ReturnValue = FAppStyle::GetBrush(bInLargeIcon ? TEXT("GraphEditor.Function_24x") : TEXT("GraphEditor.Function_16x"));

	check(Graph != nullptr);
	const UEdGraphSchema* Schema = Graph->GetSchema();
	if (Schema != nullptr)
	{
		const EGraphType GraphType = Schema->GetGraphType(Graph);
		switch (GraphType)
		{
		default:
		case GT_Ubergraph:
		{
			ReturnValue = FAppStyle::GetBrush(bInLargeIcon ? TEXT("GraphEditor.EventGraph_24x") : TEXT("GraphEditor.EventGraph_16x"));
		}
		break;
		}
	}

	return ReturnValue;
}

bool FQuestBuilderEditor::FindOpenTabsContainingDocument(const UObject* DocumentID, TArray<TSharedPtr<SDockTab>>& Results)
{
	int32 StartingCount = Results.Num();

	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);

	DocumentManager->FindMatchingTabs(Payload, /*inout*/ Results);

	// Did we add anything new?
	return (StartingCount != Results.Num());
}


void FQuestBuilderEditor::InitializeDocumentTab()
{
	check(IsEditingSingleQuestGraph());

	UQuestBuilderGraph* QuestBuilderGraph = GetQuestBuilderGraph();
	if (QuestBuilderGraph->LastEditedDocuments.Num() == 0)
	{
			QuestBuilderGraph->LastEditedDocuments.Add(FQuestBuilderEditorUtils::FindQuestGraph(QuestBuilderGraph));
	}

	for (int32 i = 0; i < QuestBuilderGraph->LastEditedDocuments.Num(); i++)
	{
		if (UObject* Obj = QuestBuilderGraph->LastEditedDocuments[i].EditedObjectPath.ResolveObject())
		{
			if (UEdGraph* Graph = Cast<UEdGraph>(Obj))
			{
				struct LocalStruct
				{
					static TSharedPtr<SDockTab> OpenGraphTree(FQuestBuilderEditor* InQuestSystemGraphEditor, UEdGraph* InGraph)
					{
						FDocumentTracker::EOpenDocumentCause OpenCause = FDocumentTracker::QuickNavigateCurrentDocument;

						for (UObject* OuterObject = InGraph->GetOuter(); OuterObject; OuterObject = OuterObject->GetOuter())
						{
							if (OuterObject->IsA<UQuestBuilderGraph>())
							{
								// reached up to the QuestBuilderGraph for the graph, we are done climbing the tree
								OpenCause = FDocumentTracker::RestorePreviousDocument;
								break;
							}
							else if (UEdGraph* OuterGraph = Cast<UEdGraph>(OuterObject))
							{
								// Found another graph, open it up
								OpenGraphTree(InQuestSystemGraphEditor, OuterGraph);
								break;
							}
						}

						return InQuestSystemGraphEditor->OpenDocument(InGraph, OpenCause);
					}
				};
				TSharedPtr<SDockTab> TabWithGraph = LocalStruct::OpenGraphTree(this, Graph);
				if (TabWithGraph.IsValid())
				{
					TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(TabWithGraph->GetContent());
					GraphEditor->SetViewLocation(QuestBuilderGraph->LastEditedDocuments[i].SavedViewOffset, QuestBuilderGraph->LastEditedDocuments[i].SavedZoomAmount);
				}
			}
			else
			{
				TSharedPtr<SDockTab> TabWithGraph = OpenDocument(Obj, FDocumentTracker::RestorePreviousDocument);
			}
		}
	}
}



bool FQuestBuilderEditor::IsEditingSingleQuestGraph() const
{
	return GetQuestBuilderGraph() != nullptr;
}

UEdGraph* FQuestBuilderEditor::GetFocusedGraph() const
{
	if (GetCurrGraphEditor().IsValid())
	{
		if (UEdGraph* Graph = GetCurrGraphEditor()->GetCurrentGraph())
		{
			if (IsValid(Graph))
			{
				return Graph;
			}
		}
	}
	return nullptr;
}

UEdGraphNode* FQuestBuilderEditor::GetSingleSelectedNode() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	return (SelectedNodes.Num() == 1) ? Cast<UEdGraphNode>(*SelectedNodes.CreateConstIterator()) : nullptr;
}

void FQuestBuilderEditor::OnGraphEditorFocused(const TSharedRef<class SGraphEditor>& InGraphEditor)
{
	// Update the graph editor that is currently focused
	CurrentGraphWidget = InGraphEditor;
	InGraphEditor->SetPinVisibility(SGraphEditor::EPinVisibility::Pin_Show);

	// Update the inspector as well, to show selection from the focused graph editor
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	//FocusInspectorOnGraphSelection(SelectedNodes, /*bForceRefresh=*/ true);

	// During undo, garbage graphs can be temporarily brought into focus, ensure that before a refresh of the MyBlueprint window that the graph is owned by a Blueprint
	if (CurrentGraphWidget.IsValid() && MyQuestWidget.IsValid())
	{
		// The focused graph can be garbage as well
		TWeakObjectPtr< UEdGraph > FocusedGraphPtr = CurrentGraphWidget->GetCurrentGraph();
		UEdGraph* FocusedGraph = FocusedGraphPtr.Get();
		
		if (FocusedGraph != nullptr)
		{
			if (UQuestBuilderEdGraph* QuestEdGraph = Cast<UQuestBuilderEdGraph>(FocusedGraph))
			{
				QuestEdGraph->SEditorGraph = CurrentGraphWidget.Get();
			}
			MyQuestWidget->Refresh();
		}
	}

	
}

void FQuestBuilderEditor::OnGraphEditorBackgrounded(const TSharedRef<SGraphEditor>& InGraphEditor)
{
}

bool FQuestBuilderEditor::IsGraphInCurrentQuestGraph(const UEdGraph* InGraph) const
{
	bool bEditable = true;

	UQuestBuilderGraph* EditingBP = GetQuestBuilderGraph();
	if (EditingBP)
	{
		TArray<UEdGraph*> Graphs;
		EditingBP->GetAllGraphs(Graphs);
		bEditable &= Graphs.Contains(InGraph);
	}

	return bEditable;
}

FGraphAppearanceInfo FQuestBuilderEditor::GetGraphAppearance() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "Quest Editor");

	if (FQuestBuilderEditor::IsPIESimulating())
	{
		if (GetQuestBuilderGraph()->QuestComponent)
		{
			AppearanceInfo.PIENotifyText = LOCTEXT("ActiveLabel", "ACTIVE");
		}
		else
		{
			AppearanceInfo.PIENotifyText = LOCTEXT("InactiveLabel", "INACTIVE");
		}
	}
	
	
	return AppearanceInfo;
}

bool FQuestBuilderEditor::InEditingMode(bool bGraphIsEditable) const
{
	return bGraphIsEditable && FQuestBuilderEditor::IsPIENotSimulating();
}


bool FQuestBuilderEditor::IsPIESimulating()
{
	return GEditor->IsSimulateInEditorInProgress() || GEditor->PlayWorld;
}

bool FQuestBuilderEditor::IsPIENotSimulating()
{
	return !GEditor->IsSimulateInEditorInProgress() && (GEditor->PlayWorld == NULL);
}

void FQuestBuilderEditor::OnChangeBreadCrumbGraph(UEdGraph* InGraph)
{
}

TSharedRef<SDockTab> FQuestBuilderEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FQuestBuilderEditorTabs::ViewportID);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"));

	if (CurrentGraphWidget.IsValid())
	{
		SpawnedTab->SetContent(CurrentGraphWidget.ToSharedRef());
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FQuestBuilderEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FQuestBuilderEditorTabs::QuestBuilderPropertyID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("Details_Title", "Property"))
		[
			PropertyWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FQuestBuilderEditor::SpawnTab_EditorSettings(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FQuestBuilderEditorTabs::QuestBuilderEditorSettingsID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("EditorSettings_Title", "Generic Graph Editor Setttings"))
		[
			EditorSettingsWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FQuestBuilderEditor::SpawnTab_MyQuest(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FQuestBuilderEditorTabs::MyQuestDetailID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("MyQuest_Title", "My Quest"))
		[
			MyQuestWidget.ToSharedRef()
		];
}

void FQuestBuilderEditor::CreateInternalWidgets()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;

	PropertyWidget = PropertyModule.CreateDetailView(DetailsViewArgs);
	PropertyWidget->SetObject( NULL );
	PropertyWidget->OnFinishedChangingProperties().AddSP(this, &FQuestBuilderEditor::OnFinishedChangingProperties);


	this->MyQuestWidget = SNew(SMyQuest, SharedThis(this));
}

TSharedRef<SGraphEditor> FQuestBuilderEditor::CreateGraphEditorWidget(TSharedRef<class FTabInfo> InTabInfo, UEdGraph* InGraph)
{

	// Create the title bar widget
	TSharedPtr<SWidget> TitleBarWidget = CreateGraphTitleBarWidget(InTabInfo, InGraph);

	CreateCommandList();

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FQuestBuilderEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FQuestBuilderEditor::OnNodeDoubleClicked);

	// Make full graph editor
	const bool bGraphIsEditable = InGraph->bEditable;
	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(this, &FQuestBuilderEditor::InEditingMode, bGraphIsEditable)
		.TitleBar(TitleBarWidget)
		.Appearance(this, &FQuestBuilderEditor::GetGraphAppearance)
		.GraphToEdit(InGraph)
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true);
}


void FQuestBuilderEditor::RebuildQuestBuilderGraphPages()
{
	if (EditingQuestGraph == nullptr)
	{
		return;
	}

	for (UEdGraph* EdGraph : EditingQuestGraph->QuestGraphPages)
	{
		if (EdGraph)
		{
			if (UQuestBuilderEdGraph* QuestGraph = Cast<UQuestBuilderEdGraph>(EdGraph))
			{
				QuestGraph->UpdateAsset();
			}
		}
	}
}

	



void FQuestBuilderEditor::OnGraphChanged(const FEdGraphEditAction& Action)
{
}
void FQuestBuilderEditor::SelectAllNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		CurrentGraphEditor->SelectAllNodes();
	}
}
bool FQuestBuilderEditor::CanSelectAllNodes()
{
	return true;
}
void FQuestBuilderEditor::DeleteSelectedNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

	CurrentGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* EdNode = Cast<UEdGraphNode>(*NodeIt);
		if (EdNode == nullptr || !EdNode->CanUserDeleteNode())
			continue;;

		if (UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(EdNode))
		{
			QuestEdNode->Modify();

			const UEdGraphSchema* Schema = QuestEdNode->GetSchema();
			if (Schema != nullptr)
			{
				Schema->BreakNodeLinks(*QuestEdNode);
			}

			QuestEdNode->DestroyNode();
		}
		else
		{
			EdNode->Modify();
			EdNode->DestroyNode();
		}

	}
}
bool FQuestBuilderEditor::CanDeleteNodes()
{
	if (IsPIESimulating())
		return false;

	// If any of the nodes can be deleted then we should allow deleting
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node != nullptr && Node->CanUserDeleteNode())
		{
			return true;
		}
	}
	return false;
}
void FQuestBuilderEditor::DeleteSelectedDuplicatableNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}
}
void FQuestBuilderEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();
	ShouldGetNewID = false;
}
bool FQuestBuilderEditor::CanCutNodes()
{
	if (IsPIESimulating())
		return false;
	return CanCopyNodes() && CanDeleteNodes();
}

void FQuestBuilderEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	TArray<UQuestBuilderEdNode*> SubNodes;

	FString ExportedText;

	ShouldGetNewID = true;

	int32 CopySubNodeIndex = 0;
	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(Node);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		if (UQuestBuilderEdNode_Edge* EdNode_Edge = Cast<UQuestBuilderEdNode_Edge>(*SelectedIter))
		{
			UQuestBuilderEdNode* StartNode = EdNode_Edge->GetStartNode();
			UQuestBuilderEdNode* EndNode = EdNode_Edge->GetEndNode();

			if (!SelectedNodes.Contains(StartNode) || !SelectedNodes.Contains(EndNode))
			{
				SelectedIter.RemoveCurrent();
				continue;
			}
		}

		Node->PrepareForCopying();

		if (QuestEdNode)
		{
			QuestEdNode->CopySubNodeIndex = CopySubNodeIndex;

			// append all subnodes for selection
			for (int32 Idx = 0; Idx < QuestEdNode->SubNodes.Num(); Idx++)
			{
				QuestEdNode->SubNodes[Idx]->CopySubNodeIndex = CopySubNodeIndex;
				SubNodes.Add(QuestEdNode->SubNodes[Idx]);
			}

			CopySubNodeIndex++;
		}

	}

	for (int32 Idx = 0; Idx < SubNodes.Num(); Idx++)
	{
		SelectedNodes.Add(SubNodes[Idx]);
		SubNodes[Idx]->PrepareForCopying();
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UQuestBuilderEdNode* Node = Cast<UQuestBuilderEdNode>(*SelectedIter);
		if (Node)
		{
			Node->PostCopyNode();
		}
	}
}

bool FQuestBuilderEditor::CanCopyNodes()
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}

	return false;
}
void FQuestBuilderEditor::PasteNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
		FVector2D PasteLocation2D = FVector2D(CurrentGraphEditor->GetPasteLocation2f().X, CurrentGraphEditor->GetPasteLocation2f().Y);
		PasteNodesHere(CurrentGraphEditor->GetCurrentGraph(), PasteLocation2D);
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5)
		PasteNodesHere(CurrentGraphEditor->GetCurrentGraph(), CurrentGraphEditor->GetPasteLocation());
#endif
	}
}
void FQuestBuilderEditor::PasteNodesHere(UEdGraph* DestinationGraph, const FVector2D& Location)
{
	// Find the graph editor with focus
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}
	// Select the newly pasted stuff
	UEdGraph* EdGraph = DestinationGraph;
	{
		// Undo/Redo support
		const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
		UQuestBuilderEdGraph* QuestEdGraph = Cast<UQuestBuilderEdGraph>(EdGraph);

		EdGraph->Modify();

		if (QuestEdGraph)
		{
			QuestEdGraph->LockUpdates();
		}

		UQuestBuilderEdNode* SelectedParent = NULL;
		bool bHasMultipleNodesSelected = false;

		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
		{
			UQuestBuilderEdNode* Node = Cast<UQuestBuilderEdNode>(*SelectedIter);
			if (Node && Node->IsSubNode())
			{
				Node = Node->ParentNode;
			}

			if (Node)
			{
				if (SelectedParent == nullptr)
				{
					SelectedParent = Node;
				}
				else
				{
					bHasMultipleNodesSelected = true;
					break;
				}
			}
		}

		// Clear the selection set (newly pasted stuff will be selected)
		CurrentGraphEditor->ClearSelectionSet();

		// Grab the text to paste from the clipboard.
		FString TextToImport;
		FPlatformApplicationMisc::ClipboardPaste(TextToImport);

		// Import the nodes
		TSet<UEdGraphNode*> PastedNodes;
		FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, PastedNodes);

		//Average position of nodes so we can move them while still maintaining relative distances to each other
		FVector2D AvgNodePosition(0.0f, 0.0f);

		// Number of nodes used to calculate AvgNodePosition
		int32 AvgCount = 0;

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* EdNode = *It;
			UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(EdNode);
			if (EdNode && (QuestEdNode == nullptr || !QuestEdNode->IsSubNode()))
			{
				AvgNodePosition.X += EdNode->NodePosX;
				AvgNodePosition.Y += EdNode->NodePosY;
				++AvgCount;
			}
			
			if (ShouldGetNewID && QuestEdNode && !QuestEdNode->IsSubNode())
			{
				UQuestBuilderNode* QuestNode = QuestEdNode ? Cast<UQuestBuilderNode>(QuestEdNode->NodeInstance) : nullptr;
				if (QuestNode)
				{
					QuestEdNode->FindUniqueNodeName(QuestNode->ID.ToString());
				}
			}
			else
			{
				ShouldGetNewID = true;
			}
		}

		if (AvgCount > 0)
		{
			float InvNumNodes = 1.0f / float(AvgCount);
			AvgNodePosition.X *= InvNumNodes;
			AvgNodePosition.Y *= InvNumNodes;
		}
		
		bool bPastedParentNode = false;

		TMap<FGuid/*New*/, FGuid/*Old*/> NewToOldNodeMapping;

		TMap<int32, UQuestBuilderEdNode*> ParentMap;
		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* PasteNode = *It;
			UQuestBuilderEdNode* PasteQuestEdNode = Cast<UQuestBuilderEdNode>(PasteNode);

			if (PasteNode && (PasteQuestEdNode == nullptr || !PasteQuestEdNode->IsSubNode()))
			{
				bPastedParentNode = true;

				// Select the newly pasted stuff
				CurrentGraphEditor->SetNodeSelection(PasteNode, true);

				const FVector::FReal NodePosX = (PasteNode->NodePosX - AvgNodePosition.X) + Location.X;
				const FVector::FReal NodePosY = (PasteNode->NodePosY - AvgNodePosition.Y) + Location.Y;

				PasteNode->NodePosX = static_cast<int32>(NodePosX);
				PasteNode->NodePosY = static_cast<int32>(NodePosY);

				PasteNode->SnapToGrid(16);

				const FGuid OldGuid = PasteNode->NodeGuid;

				// Give new node a different Guid from the old one
				PasteNode->CreateNewGuid();

				const FGuid NewGuid = PasteNode->NodeGuid;

				NewToOldNodeMapping.Add(NewGuid, OldGuid);

				if (PasteQuestEdNode)
				{
					PasteQuestEdNode->RemoveAllSubNodes();
					ParentMap.Add(PasteQuestEdNode->CopySubNodeIndex, PasteQuestEdNode);
				}
			}
		}

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UQuestBuilderEdNode* PasteNode = Cast<UQuestBuilderEdNode>(*It);
			if (PasteNode && PasteNode->IsSubNode())
			{
				PasteNode->NodePosX = 0;
				PasteNode->NodePosY = 0;

				// remove subnode from graph, it will be referenced from parent node
				PasteNode->DestroyNode();

				PasteNode->ParentNode = ParentMap.FindRef(PasteNode->CopySubNodeIndex);
				if (PasteNode->ParentNode)
				{
					PasteNode->ParentNode->AddSubNode(PasteNode, EdGraph);
				}
				else if (!bHasMultipleNodesSelected && !bPastedParentNode && SelectedParent)
				{
					PasteNode->ParentNode = SelectedParent;
					SelectedParent->AddSubNode(PasteNode, EdGraph);
				}
			}
		}

		FixupPastedNodes(PastedNodes, NewToOldNodeMapping);

		if (QuestEdGraph)
		{
			QuestEdGraph->UpdateClassData();
			QuestEdGraph->UnlockUpdates();
		}

		// Update UI
		CurrentGraphEditor->NotifyGraphChanged();

		UObject* GraphOwner = EdGraph->GetOuter();
		if (GraphOwner)
		{
			GraphOwner->PostEditChange();
			GraphOwner->MarkPackageDirty();
		}

	}

	
}


void FQuestBuilderEditor::FixupPastedNodes(const TSet<UEdGraphNode*>& NewPastedGraphNodes, const TMap<FGuid, FGuid>& NewToOldNodeMapping)
{
}

bool FQuestBuilderEditor::CanPasteNodes() const
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}
void FQuestBuilderEditor::DuplicateNodes()
{
	ShouldGetNewID = true;
	CopySelectedNodes();
	PasteNodes();
}
bool FQuestBuilderEditor::CanDuplicateNodes()
{
	return CanCopyNodes();
}

void FQuestBuilderEditor::CreateNewObjective()
{
	HandleNewNodeClassPicked(UQuestBuilderNode_Objective::StaticClass());
}

void FQuestBuilderEditor::HandleNewNodeClassPicked(UClass* InClass) const
{

	if (EditingQuestGraph != nullptr && InClass != nullptr && EditingQuestGraph->GetOutermost())
	{
		const FString ClassName = FBlueprintEditorUtils::GetClassNameWithoutSuffix(InClass);

		FString PathName = EditingQuestGraph->GetOutermost()->GetPathName();
		PathName = FPaths::GetPath(PathName);

		// Now that we've generated some reasonable default locations/names for the package, allow the user to have the final say
		// before we create the package and initialize the blueprint inside of it.
		FSaveAssetDialogConfig SaveAssetDialogConfig;
		SaveAssetDialogConfig.DialogTitleOverride = LOCTEXT("SaveAssetDialogTitle", "Save Asset As");
		SaveAssetDialogConfig.DefaultPath = PathName;
		if (InClass == UQuestBuilderNode_Objective::StaticClass())
		{
			SaveAssetDialogConfig.DefaultAssetName = TEXT("Objective_New");
		}
		else
		{
			SaveAssetDialogConfig.DefaultAssetName = ClassName + TEXT("_New");
		}
		SaveAssetDialogConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::Disallow;

		const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		const FString SaveObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveAssetDialogConfig);
		if (!SaveObjectPath.IsEmpty())
		{
			const FString SavePackageName = FPackageName::ObjectPathToPackageName(SaveObjectPath);
			const FString SavePackagePath = FPaths::GetPath(SavePackageName);
			const FString SaveAssetName = FPaths::GetBaseFilename(SavePackageName);

			UPackage* Package = CreatePackage(*SavePackageName);
			if (ensure(Package))
			{
				// Create and init a new Blueprint
				if (UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(InClass, Package, FName(*SaveAssetName), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass()))
				{
					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(NewBP);

					// Notify the asset registry
					FAssetRegistryModule::AssetCreated(NewBP);

					// Mark the package dirty...
					Package->MarkPackageDirty();
				}
			}
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

bool FQuestBuilderEditor::CanCreateNewObjective() const
{
	return true;
}
bool FQuestBuilderEditor::IsNewObjectiveButtonVisible() const
{
	return true;
}

void FQuestBuilderEditor::CreateNewQuestDecorator()
{
	HandleNewNodeClassPicked(UOrionDecorator::StaticClass());
}
bool FQuestBuilderEditor::CanCreateQuestDecorator() const
{
	return true;
}
void FQuestBuilderEditor::CreateNewQuestEvent()
{
	HandleNewNodeClassPicked(UOrionEvent::StaticClass());
}
bool FQuestBuilderEditor::CanCreateQuestEvent() const
{
	return true;
}
void FQuestBuilderEditor::OpenQuestSetting()
{
	PropertyWidget->SetObject(GetQuestBuilderGraph());
}
bool FQuestBuilderEditor::CanOpenQuestSetting() const
{
	return true;
}
void FQuestBuilderEditor::OnRenameNode()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt);
			if (SelectedNode != NULL && SelectedNode->bCanRenameNode)
			{
				CurrentGraphEditor->IsNodeTitleVisible(SelectedNode, true);
				break;
			}
		}
	}
}
bool FQuestBuilderEditor::CanRenameNodes() const
{
	if (GetFocusedGraph())
	{
		if (const UEdGraphNode* SelectedNode = GetSingleSelectedNode())
		{
			return SelectedNode->GetCanRenameNode();
		}
	}
	return false;

}

bool FQuestBuilderEditor::CanCreateComment() const
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	return CurrentGraphEditor.IsValid();
}

void FQuestBuilderEditor::OnCreateComment()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (UEdGraph* EdGraph = CurrentGraphEditor.IsValid() ? CurrentGraphEditor->GetCurrentGraph() : nullptr)
	{
		TSharedPtr<FEdGraphSchemaAction> Action = EdGraph->GetSchema()->GetCreateCommentAction();
		if (Action.IsValid())
		{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
			Action->PerformAction(EdGraph, nullptr, FVector2f());
#elif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5)
			Action->PerformAction(EdGraph, nullptr, FVector2D());
#endif
		}
	}
}

void FQuestBuilderEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	TArray<UObject*> Selection = FQuestBuilderEditorUtils::GetSelectionForPropertyEditor(NewSelection);

	if (Selection.Num() == 0)
	{
		if (UQuestBuilderEdGraph* QuestGraph = Cast<UQuestBuilderEdGraph>(CurrentGraphWidget->GetCurrentGraph()))
		{
			PropertyWidget->SetObject(QuestGraph->Quest);
		}
	}
	else if (Selection.Num() == 1)
	{
		if (UQuestBuilderNode_Root* RootNode = Cast<UQuestBuilderNode_Root>(Selection[0]))
		{
			if (UQuestBuilderEdGraph* QuestGraph = Cast<UQuestBuilderEdGraph>(CurrentGraphWidget->GetCurrentGraph()))
			{
				PropertyWidget->SetObject(QuestGraph->Quest);
			}
		}
		else
		{
			PropertyWidget->SetObjects(Selection);
		}
	}
	else
	{
		PropertyWidget->SetObjects(Selection);
	}
}

void FQuestBuilderEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	UQuestBuilderEdNode* QuestEdNode = Cast<UQuestBuilderEdNode>(Node);

	if (QuestEdNode && 
		QuestEdNode->NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		UClass* NodeClass = QuestEdNode->NodeInstance->GetClass();
		UPackage* Pkg = NodeClass->GetOuterUPackage();
		FString ClassName = NodeClass->GetName().LeftChop(2);
		UBlueprint* BlueprintOb = FindObject<UBlueprint>(Pkg, *ClassName);
		if (BlueprintOb)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(BlueprintOb);
		}
	}
}

void FQuestBuilderEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (EditingQuestGraph == nullptr)
		return;

	for (UEdGraph* EdGraph : EditingQuestGraph->QuestGraphPages)
	{
		EdGraph->GetSchema()->ForceVisualizationCacheClear();
	}
	DocumentManager->RefreshAllTabs();

}
#include "Async/Async.h"

void FQuestBuilderEditor::OnPackageMarkedDirty(UPackage* ModifiedPackage, bool bWasDirty)
{   
}
//Called when saving our file graph
#if ENGINE_MAJOR_VERSION < 5

void FQuestBuilderEditor::OnPackageSaved(const FString& PackageFileName, UObject* Outer)
{
	RebuildQuestBuilderGraphPages();
}
#else // #if ENGINE_MAJOR_VERSION < 5
void FQuestBuilderEditor::OnPackageSavedWithContext(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext ObjectSaveContext)
{
	RebuildQuestBuilderGraphPages();
	
}


#endif // #else // #if ENGINE_MAJOR_VERSION < 5

#undef LOCTEXT_NAMESPACE
