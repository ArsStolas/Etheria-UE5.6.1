// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderEditor.h"
#include "DialogBuilderNode_Root.h"
#include "Decorator/OrionDecorator.h"
#include "EngineGlobals.h"
#include "Editor/EditorEngine.h"
#include "Editor.h"
#include "UnrealEdGlobals.h"
#include "Event/OrionEvent.h"
#include "DialogCameraShot.h"
#include "BlueprintEditor.h"
#include "UObject/ObjectSaveContext.h"
#include "GraphEditorActions.h"
#include "Framework/Commands/GenericCommands.h"
#include "DialogBuilderEditorToolbar.h"
#include "DialogBuilder_EditorCommands.h"
#include "DialogBuilderEdGraph.h"
#include "DialogBuilderEdNode.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node.h"
#include "GraphEditAction.h"
#include "DialogBuilderEdNode_Edge.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "EdGraphSchema_DialogBuilder.h"
#include "DialogBuilderEditorUtils.h"
#include "DialogBuilderFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SMyDialog.h"
#include "Dialog_System_Editor.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserFrontEndFilterExtension.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include <Kismet2/KismetEditorUtilities.h>

#define LOCTEXT_NAMESPACE "DialogBuilderEditor"

const FName FDialogBuilderEditorTabs::MyDialogDetailID(TEXT("MyDialogDetail"));
const FName FDialogBuilderEditorTabs::DialogBuilderPropertyID(TEXT("DialogBuilderProperty"));
const FName FDialogBuilderEditorTabs::ViewportID(TEXT("Viewport"));
const FName FDialogBuilderEditorTabs::DialogBuilderEditorSettingsID(TEXT("DialogBuilderEditorSettings"));

const FName DialogBuilderEditorAppName = FName(TEXT("DialogBuilderEditorApp"));

//////////////////////////////////////////////////////////////////////////
FDialogBuilderEditor::FDialogBuilderEditor()
{
	EditingDialogGraph = nullptr;

#if ENGINE_MAJOR_VERSION < 5
	OnPackageSavedDelegateHandle = UPackage::PackageSavedEvent.AddRaw(this, &FDialogBuilderEditor::OnPackageSaved);
#else // #if ENGINE_MAJOR_VERSION < 5
	OnPackageSavedDelegateHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FDialogBuilderEditor::OnPackageSavedWithContext);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5
}

FDialogBuilderEditor::~FDialogBuilderEditor()
{
#if ENGINE_MAJOR_VERSION < 5
	UPackage::PackageSavedEvent.Remove(OnPackageSavedDelegateHandle);
#else // #if ENGINE_MAJOR_VERSION < 5
	UPackage::PackageSavedWithContextEvent.Remove(OnPackageSavedDelegateHandle);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5
}

void FDialogBuilderEditor::Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InObject)
{
	UDialogBuilderGraph* DialogGraphToEdit = Cast<UDialogBuilderGraph>(InObject);

	if (DialogGraphToEdit != nullptr)
	{
		EditingDialogGraph = DialogGraphToEdit;
	}

	//Binding Functionality
	EditingDialogGraph->GetOutermost()->PackageMarkedDirtyEvent.AddRaw(this, &FDialogBuilderEditor::OnPackageMarkedDirty);

	TSharedPtr<FDialogBuilderEditor> ThisPtr(SharedThis(this));
	if (!DocumentManager.IsValid())
	{
		DocumentManager = MakeShareable(new FDocumentTracker);
		DocumentManager->Initialize(ThisPtr);

		// Register the document factories
		{
			TSharedRef<FDocumentTabFactory> GraphEditorFactory = MakeShareable(new FDialogGraphEditorSummoner(ThisPtr,
				FDialogGraphEditorSummoner::FOnCreateGraphEditorWidget::CreateSP(this, &FDialogBuilderEditor::CreateGraphEditorWidget)
			));

			// Also store off a reference to the grapheditor factory so we can find all the tabs spawned by it later.
			DialogEditorTabFactoryPtr = GraphEditorFactory;
			DocumentManager->RegisterDocumentFactory(GraphEditorFactory);
		}
	}

	TArray<UObject*> ObjectsToEdit;
	if (EditingDialogGraph != nullptr)
	{
		ObjectsToEdit.Add(EditingDialogGraph);
	}

	//Make Toolbar
	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FDialogBuilderEditorToolbar(SharedThis(this)));
	}

	// if we are already editing objects, dont try to recreate the editor from scratch but update the list of objects in edition
	// ex: BehaviorTree may want to reuse an editor already opened for its associated Blackboard asset.
	const TArray<UObject*>* EditedObjects = GetObjectsCurrentlyBeingEdited();
	if (EditedObjects == nullptr || EditedObjects->Num() == 0)
	{
		FGenericCommands::Register();
		FGraphEditorCommands::Register();
		FMyDialogCommands::Register();
		FDialogBuilder_EditorCommands::Register();

		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

		ToolbarBuilder->AddDialogSystemToolbar(ToolbarExtender);

		BindCommands();
		CreateInternalWidgets();

		// Layout
		const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_DialogBuilderEditor_Layout_v1")
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
						->AddTab(FDialogBuilderEditorTabs::MyDialogDetailID, ETabState::OpenedTab)
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
							->AddTab(FDialogBuilderEditorTabs::DialogBuilderPropertyID, ETabState::OpenedTab)
						)
					)
				)
			);

		const bool bCreateDefaultStandaloneMenu = true;
		const bool bCreateDefaultToolbar = true;
		InitAssetEditor(Mode, InitToolkitHost, DialogBuilderEditorAppName, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, EditingDialogGraph, false);


		if (EditingDialogGraph && EditingDialogGraph->bIsNewlyCreated)
		{
			NewDocument_OnClicked(CGT_NewDialogGraph);
			EditingDialogGraph->bIsNewlyCreated = false;
			EditingDialogGraph->Modify();
		}
		else
		{
			if (GetDialogBuilderGraph()->DialogGraphPages.Num() > 0)
			{
				OpenDocument(GetDialogBuilderGraph()->DialogGraphPages[0], FDocumentTracker::OpenNewDocument);
				
				//bind on graph changes
				for (UEdGraph* EdGraph : EditingDialogGraph->DialogGraphPages)
				{
					if (EdGraph)
					{
						if (UDialogBuilderEdGraph* DialogGraph = Cast<UDialogBuilderEdGraph>(EdGraph))
						{
							DialogGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FDialogBuilderEditor::OnGraphChanged));
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
	RebuildDialogBuilderGraphPages();
}


void FDialogBuilderEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	DocumentManager->SetTabManager(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_DialogBuilderEditor", "Generic Graph Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::MyDialogDetailID, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_MyDialog))
		.SetDisplayName(LOCTEXT("Dialog Details", "My Dialog"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("GraphCanvasTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::DialogBuilderPropertyID, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Property"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FDialogBuilderEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::MyDialogDetailID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::ViewportID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::DialogBuilderPropertyID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::DialogBuilderEditorSettingsID);
}

void FDialogBuilderEditor::PostUndo(bool bSuccess)
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

void FDialogBuilderEditor::PostRedo(bool bSuccess)
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

void FDialogBuilderEditor::BindCommands()
{
	ToolkitCommands->MapAction(FDialogBuilder_EditorCommands::Get().NewDialogDecorator,
		FExecuteAction::CreateSP(this, &FDialogBuilderEditor::CreateNewDialogDecorator),
		FCanExecuteAction::CreateSP(this, &FDialogBuilderEditor::CanCreateDialogDecorator)
	);

	ToolkitCommands->MapAction(FDialogBuilder_EditorCommands::Get().NewDialogEvent,
		FExecuteAction::CreateSP(this, &FDialogBuilderEditor::CreateNewDialogEvent),
		FCanExecuteAction::CreateSP(this, &FDialogBuilderEditor::CanCreateDialogEvent)
	); 
	
	ToolkitCommands->MapAction(FDialogBuilder_EditorCommands::Get().NewDialogCameraShot,
		FExecuteAction::CreateSP(this, &FDialogBuilderEditor::CreateNewDialogCameraShot),
		FCanExecuteAction::CreateSP(this, &FDialogBuilderEditor::CanCreateDialogCameraShot)
	);

	ToolkitCommands->MapAction(FDialogBuilder_EditorCommands::Get().DialogSetting,
		FExecuteAction::CreateSP(this, &FDialogBuilderEditor::OpenDialogSetting),
		FCanExecuteAction::CreateSP(this, &FDialogBuilderEditor::CanOpenDialogSetting)
	);
	
	ToolkitCommands->MapAction(FDialogBuilder_EditorCommands::Get().AddNewDialogGraph,
		FExecuteAction::CreateSP(this, &FDialogBuilderEditor::NewDocument_OnClicked, CGT_NewDialogGraph),
		FCanExecuteAction::CreateSP(this, &FDialogBuilderEditor::CanAddNewDialogGraph),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FDialogBuilderEditor::NewDocument_IsVisibleForType, CGT_NewDialogGraph)
	);

}

void FDialogBuilderEditor::CreateCommandList()
{
	if (GraphEditorCommands.IsValid()) {
		return;
	}

	GraphEditorCommands = MakeShareable(new FUICommandList);
	// Can't use CreateSP here because derived editor are already implementing TSharedFromThis<FAssetEditorToolkit>
	// however it should be safe, since commands are being used only within this editor
	// if it ever crashes, this function will have to go away and be reimplemented in each derived class

	GraphEditorCommands->MapAction(FDialogBuilder_EditorCommands::Get().AddNewDialogGraph,
		FExecuteAction::CreateSP(this, &FDialogBuilderEditor::NewDocument_OnClicked, CGT_NewDialogGraph),
		FCanExecuteAction::CreateSP(this, &FDialogBuilderEditor::CanAddNewDialogGraph),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateSP(this, &FDialogBuilderEditor::NewDocument_IsVisibleForType, CGT_NewDialogGraph)
	);

	GraphEditorCommands->MapAction(FDialogBuilder_EditorCommands::Get().NewDialogDecorator,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CreateNewDialogDecorator),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanCreateDialogDecorator));
	
	GraphEditorCommands->MapAction(FDialogBuilder_EditorCommands::Get().NewDialogEvent,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CreateNewDialogEvent),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanCreateDialogEvent));

	GraphEditorCommands->MapAction(FDialogBuilder_EditorCommands::Get().DialogSetting,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::OpenDialogSetting),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanOpenDialogSetting));

	GraphEditorCommands->MapAction(FDialogBuilder_EditorCommands::Get().NewDialogCameraShot,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CreateNewDialogCameraShot),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanCreateDialogCameraShot));

	GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::SelectAllNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanSelectAllNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanDeleteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanCopyNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanCutNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanPasteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanDuplicateNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &FDialogBuilderEditor::OnRenameNode),
		FCanExecuteAction::CreateSP(this, &FDialogBuilderEditor::CanRenameNodes)
	);

	GraphEditorCommands->MapAction(
		FGraphEditorCommands::Get().CreateComment,
		FExecuteAction::CreateRaw(this, &FDialogBuilderEditor::OnCreateComment),
		FCanExecuteAction::CreateRaw(this, &FDialogBuilderEditor::CanCreateComment)
	);
}

TSharedPtr<SGraphEditor> FDialogBuilderEditor::GetCurrGraphEditor() const
{
	return CurrentGraphWidget;
}

FGraphPanelSelectionSet FDialogBuilderEditor::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = GetCurrGraphEditor();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}

	return CurrentSelection;
}

FName FDialogBuilderEditor::GetToolkitFName() const
{
	return FName("FDialogGraphEditor");
}

FText FDialogBuilderEditor::GetBaseToolkitName() const
{
	return LOCTEXT("DialogGraphEditorAppLabel", "Dialog Graph Editor");
}


FText FDialogBuilderEditor::GetToolkitName() const
{
	const bool bDirtyState = EditingDialogGraph->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("DialogGraphName"), FText::FromString(EditingDialogGraph->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("DialogGraphEditorToolkitName", "{DialogGraphName}{DirtyState}"), Args);
}

FText FDialogBuilderEditor::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolTipTextForObject(EditingDialogGraph);
}

FLinearColor FDialogBuilderEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::Blue;
}

FString FDialogBuilderEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("DialogGraphEditor");
}

FString FDialogBuilderEditor::GetDocumentationLink() const
{
	//make documentation from notion add this link
	return TEXT("");
}

void FDialogBuilderEditor::SaveAsset_Execute()
{
	FAssetEditorToolkit::SaveAsset_Execute();

}

void FDialogBuilderEditor::RefreshEditors()
{
	
}

void FDialogBuilderEditor::RefreshMyDialog()
{
}

void FDialogBuilderEditor::RefreshInspector()
{
}

void FDialogBuilderEditor::AddToSelection(UEdGraphNode* InNode)
{
}

void FDialogBuilderEditor::JumpToHyperlink(const UObject* ObjectReference, bool bRedialogRename)
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

void FDialogBuilderEditor::JumpToPin(const UEdGraphPin* Pin)
{
}

void FDialogBuilderEditor::SummonSearchUI(bool bSetFindWithinBlueprint, FString NewSearchTerms, bool bSelectFirstResult)
{
}

void FDialogBuilderEditor::SummonFindAndReplaceUI()
{
}

TSharedPtr<SGraphEditor> FDialogBuilderEditor::OpenGraphAndBringToFront(UEdGraph* Graph, bool bSetFocus)
{
	return TSharedPtr<SGraphEditor>();
}

void FDialogBuilderEditor::UpdateToolbar()
{
}

void FDialogBuilderEditor::RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FDialogBuilderEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	/*if (GetObjectsCurrentlyBeingEdited()->Num() > 0)
	{
		TArray<UObject*>& LocalEditingObjects = const_cast<TArray<UObject*>&>(GetEditingObjects());

		Collector.AddReferencedObjects(LocalEditingObjects);
	}

	Collector.AddReferencedObject(EditingDialogGraph);*/
}

FString FDialogBuilderEditor::GetReferencerName() const
{
	return FString();
}

UDialogBuilderGraph* FDialogBuilderEditor::GetDialogBuilderGraph() const
{
	return EditingDialogGraph;
}

bool FDialogBuilderEditor::NewDocument_IsVisibleForType(ECreatedDialogDocumentType GraphType) const
{
	return false;
}

void FDialogBuilderEditor::NewDocument_OnClicked(ECreatedDialogDocumentType GraphType)
{
	FText DocumentNameText;
	bool bResetMyBlueprintFilter = false;

	switch (GraphType)
	{
	case CGT_NewDialogGraph:
		DocumentNameText = LOCTEXT("NewDocDialogName", "Dialog Graph");
		bResetMyBlueprintFilter = true;
		break;
	
	default:
		DocumentNameText = LOCTEXT("NewDocNewName", "NewDocument");
		break;
	}

	FName DocumentName = FName(*DocumentNameText.ToString());

	

	// Make sure the new name is valid
	DocumentName = FDialogBuilderEditorUtils::FindUniqueDialogName(DocumentNameText.ToString());
		
	//check(IsEditingSingleBlueprint());

	const FScopedTransaction Transaction(LOCTEXT("AddNewDialogGraph", "Add New Dialog Graph"));
	GetDialogBuilderGraph()->Modify();

	UEdGraph* NewGraph = nullptr;

	
	if (GraphType == CGT_NewDialogGraph)
	{
		NewGraph = FDialogBuilderEditorUtils::CreateNewGraph(GetDialogBuilderGraph(), DocumentName, UDialogBuilderEdGraph::StaticClass(), UEdGraphSchema_DialogBuilder::StaticClass());
		NewGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FDialogBuilderEditor::OnGraphChanged));
		FDialogBuilderEditorUtils::AddDialogGraphPage(GetDialogBuilderGraph(), NewGraph);

	}
	else
	{
		ensureMsgf(false, TEXT("GraphType is invalid"));
	}

	// Now open the new graph
	if (NewGraph)
	{
		OpenDocument(NewGraph, FDocumentTracker::OpenNewDocument);

		RenameNewlyAddedAction(DocumentName);
	}
	else
	{
		LogSimpleMessage(LOCTEXT("AddDocument_Error", "Adding new document failed."));
	}
}

bool FDialogBuilderEditor::InEditingMode() const
{
	return true;
}

bool FDialogBuilderEditor::CanAddNewDialogGraph() const
{
	return false;
}

TSharedPtr<SDockTab> FDialogBuilderEditor::OpenDocument(const UObject* DocumentID, FDocumentTracker::EOpenDocumentCause Cause)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);
	return DocumentManager->OpenDocument(Payload, Cause);
}

void FDialogBuilderEditor::CloseDocumentTab(const UObject* DocumentID)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);
	DocumentManager->CloseTab(Payload);
}

void FDialogBuilderEditor::RenameNewlyAddedAction(FName InActionName)
{
	if (MyDialogWidget.IsValid())
	{
		// Force a refresh immediately, the item has to be present in the list for the rename redialogs to be successful.
		MyDialogWidget->Refresh();
		MyDialogWidget->SelectItemByName(InActionName, ESelectInfo::OnMouseClick);
		MyDialogWidget->OnRequestRenameOnActionNode();
	}
}

void FDialogBuilderEditor::LogSimpleMessage(const FText& MessageText)
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

TSharedRef<SWidget> FDialogBuilderEditor::CreateGraphTitleBarWidget(TSharedRef<FTabInfo> InTabInfo, UEdGraph* InGraph)
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


const FSlateBrush* FDialogBuilderEditor::GetGlyphForGraph(const UEdGraph* Graph, bool bInLargeIcon)
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

bool FDialogBuilderEditor::FindOpenTabsContainingDocument(const UObject* DocumentID, TArray<TSharedPtr<SDockTab>>& Results)
{
	int32 StartingCount = Results.Num();

	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(DocumentID);

	DocumentManager->FindMatchingTabs(Payload, /*inout*/ Results);

	// Did we add anything new?
	return (StartingCount != Results.Num());
}


void FDialogBuilderEditor::InitializeDocumentTab()
{
	check(IsEditingSingleDialogGraph());

	UDialogBuilderGraph* DialogBuilderGraph = GetDialogBuilderGraph();
	if (DialogBuilderGraph->LastEditedDocuments.Num() == 0)
	{
			DialogBuilderGraph->LastEditedDocuments.Add(FDialogBuilderEditorUtils::FindDialogGraph(DialogBuilderGraph));
	}

	for (int32 i = 0; i < DialogBuilderGraph->LastEditedDocuments.Num(); i++)
	{
		if (UObject* Obj = DialogBuilderGraph->LastEditedDocuments[i].EditedObjectPath.ResolveObject())
		{
			if (UEdGraph* Graph = Cast<UEdGraph>(Obj))
			{
				struct LocalStruct
				{
					static TSharedPtr<SDockTab> OpenGraphTree(FDialogBuilderEditor* InDialogSystemGraphEditor, UEdGraph* InGraph)
					{
						FDocumentTracker::EOpenDocumentCause OpenCause = FDocumentTracker::QuickNavigateCurrentDocument;

						for (UObject* OuterObject = InGraph->GetOuter(); OuterObject; OuterObject = OuterObject->GetOuter())
						{
							if (OuterObject->IsA<UDialogBuilderGraph>())
							{
								// reached up to the DialogBuilderGraph for the graph, we are done climbing the tree
								OpenCause = FDocumentTracker::RestorePreviousDocument;
								break;
							}
							else if (UEdGraph* OuterGraph = Cast<UEdGraph>(OuterObject))
							{
								// Found another graph, open it up
								OpenGraphTree(InDialogSystemGraphEditor, OuterGraph);
								break;
							}
						}

						return InDialogSystemGraphEditor->OpenDocument(InGraph, OpenCause);
					}
				};
				TSharedPtr<SDockTab> TabWithGraph = LocalStruct::OpenGraphTree(this, Graph);
				if (TabWithGraph.IsValid())
				{
					TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(TabWithGraph->GetContent());
					GraphEditor->SetViewLocation(DialogBuilderGraph->LastEditedDocuments[i].SavedViewOffset, DialogBuilderGraph->LastEditedDocuments[i].SavedZoomAmount);
				}
			}
			else
			{
				TSharedPtr<SDockTab> TabWithGraph = OpenDocument(Obj, FDocumentTracker::RestorePreviousDocument);
			}
		}
	}
}



bool FDialogBuilderEditor::IsEditingSingleDialogGraph() const
{
	return GetDialogBuilderGraph() != nullptr;
}

UEdGraph* FDialogBuilderEditor::GetFocusedGraph() const
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

UEdGraphNode* FDialogBuilderEditor::GetSingleSelectedNode() const
{
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	return (SelectedNodes.Num() == 1) ? Cast<UEdGraphNode>(*SelectedNodes.CreateConstIterator()) : nullptr;
}

void FDialogBuilderEditor::OnGraphEditorFocused(const TSharedRef<class SGraphEditor>& InGraphEditor)
{
	// Update the graph editor that is currently focused
	CurrentGraphWidget = InGraphEditor;
	InGraphEditor->SetPinVisibility(SGraphEditor::EPinVisibility::Pin_Show);

	// Update the inspector as well, to show selection from the focused graph editor
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	//FocusInspectorOnGraphSelection(SelectedNodes, /*bForceRefresh=*/ true);

	// During undo, garbage graphs can be temporarily brought into focus, ensure that before a refresh of the MyBlueprint window that the graph is owned by a Blueprint
	if (CurrentGraphWidget.IsValid() && MyDialogWidget.IsValid())
	{
		// The focused graph can be garbage as well
		TWeakObjectPtr< UEdGraph > FocusedGraphPtr = CurrentGraphWidget->GetCurrentGraph();
		UEdGraph* FocusedGraph = FocusedGraphPtr.Get();
		
		if (FocusedGraph != nullptr)
		{
			if (UDialogBuilderEdGraph* DialogEdGraph = Cast<UDialogBuilderEdGraph>(FocusedGraph))
			{
				DialogEdGraph->SEditorGraph = CurrentGraphWidget.Get();
			}
			MyDialogWidget->Refresh();
		}
	}

	
}

void FDialogBuilderEditor::OnGraphEditorBackgrounded(const TSharedRef<SGraphEditor>& InGraphEditor)
{
}

bool FDialogBuilderEditor::IsGraphInCurrentDialogGraph(const UEdGraph* InGraph) const
{
	bool bEditable = true;

	UDialogBuilderGraph* EditingBP = GetDialogBuilderGraph();
	if (EditingBP)
	{
		TArray<UEdGraph*> Graphs;
		EditingBP->GetAllGraphs(Graphs);
		bEditable &= Graphs.Contains(InGraph);
	}

	return bEditable;
}

FGraphAppearanceInfo FDialogBuilderEditor::GetGraphAppearance() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "Dialog Editor");

	if (FDialogBuilderEditor::IsPIESimulating())
	{
		if (GetDialogBuilderGraph()->DialogComponent)
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

bool FDialogBuilderEditor::InEditingMode(bool bGraphIsEditable) const
{
	return bGraphIsEditable && FDialogBuilderEditor::IsPIENotSimulating();
}


bool FDialogBuilderEditor::IsPIESimulating()
{
	return GEditor->bIsSimulatingInEditor || GEditor->PlayWorld;
}

bool FDialogBuilderEditor::IsPIENotSimulating()
{
	return !GEditor->bIsSimulatingInEditor && (GEditor->PlayWorld == NULL);
}

void FDialogBuilderEditor::OnChangeBreadCrumbGraph(UEdGraph* InGraph)
{
}

TSharedRef<SDockTab> FDialogBuilderEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDialogBuilderEditorTabs::ViewportID);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"));

	if (CurrentGraphWidget.IsValid())
	{
		SpawnedTab->SetContent(CurrentGraphWidget.ToSharedRef());
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FDialogBuilderEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDialogBuilderEditorTabs::DialogBuilderPropertyID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("Details_Title", "Property"))
		[
			PropertyWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FDialogBuilderEditor::SpawnTab_EditorSettings(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDialogBuilderEditorTabs::DialogBuilderEditorSettingsID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("EditorSettings_Title", "Generic Graph Editor Setttings"))
		[
			EditorSettingsWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FDialogBuilderEditor::SpawnTab_MyDialog(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDialogBuilderEditorTabs::MyDialogDetailID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("MyDialog_Title", "My Dialog"))
		[
			MyDialogWidget.ToSharedRef()
		];
}

void FDialogBuilderEditor::CreateInternalWidgets()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;	

	PropertyWidget = PropertyModule.CreateDetailView(DetailsViewArgs);
	PropertyWidget->SetObject( NULL );
	PropertyWidget->OnFinishedChangingProperties().AddSP(this, &FDialogBuilderEditor::OnFinishedChangingProperties);


	this->MyDialogWidget = SNew(SMyDialog, SharedThis(this));
}

TSharedRef<SGraphEditor> FDialogBuilderEditor::CreateGraphEditorWidget(TSharedRef<class FTabInfo> InTabInfo, UEdGraph* InGraph)
{

	// Create the title bar widget
	TSharedPtr<SWidget> TitleBarWidget = CreateGraphTitleBarWidget(InTabInfo, InGraph);

	CreateCommandList();

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FDialogBuilderEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FDialogBuilderEditor::OnNodeDoubleClicked);

	// Make full graph editor
	const bool bGraphIsEditable = InGraph->bEditable;
	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(this, &FDialogBuilderEditor::InEditingMode, bGraphIsEditable)
		.TitleBar(TitleBarWidget)
		.Appearance(this, &FDialogBuilderEditor::GetGraphAppearance)
		.GraphToEdit(InGraph)
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true);
}


void FDialogBuilderEditor::RebuildDialogBuilderGraphPages()
{
	if (EditingDialogGraph == nullptr)
	{
		return;
	}

	for (UEdGraph* EdGraph : EditingDialogGraph->DialogGraphPages)
	{
		if (EdGraph)
		{
			if (UDialogBuilderEdGraph* DialogGraph = Cast<UDialogBuilderEdGraph>(EdGraph))
			{
				DialogGraph->UpdateAsset();
			}
		}
	}
}

	



void FDialogBuilderEditor::OnGraphChanged(const FEdGraphEditAction& Action)
{
}
void FDialogBuilderEditor::SelectAllNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		CurrentGraphEditor->SelectAllNodes();
	}
}
bool FDialogBuilderEditor::CanSelectAllNodes()
{
	return true;
}
void FDialogBuilderEditor::DeleteSelectedNodes()
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

		if (UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(EdNode))
		{
			DialogEdNode->Modify();

			const UEdGraphSchema* Schema = DialogEdNode->GetSchema();
			if (Schema != nullptr)
			{
				Schema->BreakNodeLinks(*DialogEdNode);
			}

			DialogEdNode->DestroyNode();
		}
		else
		{
			EdNode->Modify();
			EdNode->DestroyNode();
		}

	}
}
bool FDialogBuilderEditor::CanDeleteNodes()
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
void FDialogBuilderEditor::DeleteSelectedDuplicatableNodes()
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
void FDialogBuilderEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();
	ShouldGetNewID = false;
}
bool FDialogBuilderEditor::CanCutNodes()
{
	if (IsPIESimulating())
		return false;
	return CanCopyNodes() && CanDeleteNodes();
}

void FDialogBuilderEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	TArray<UDialogBuilderEdNode*> SubNodes;

	FString ExportedText;

	ShouldGetNewID = true;

	int32 CopySubNodeIndex = 0;
	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(Node);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		if (UDialogBuilderEdNode_Edge* EdNode_Edge = Cast<UDialogBuilderEdNode_Edge>(*SelectedIter))
		{
			UDialogBuilderEdNode* StartNode = EdNode_Edge->GetStartNode();
			UDialogBuilderEdNode* EndNode = EdNode_Edge->GetEndNode();

			if (!SelectedNodes.Contains(StartNode) || !SelectedNodes.Contains(EndNode))
			{
				SelectedIter.RemoveCurrent();
				continue;
			}
		}

		Node->PrepareForCopying();

		if (DialogEdNode)
		{
			DialogEdNode->CopySubNodeIndex = CopySubNodeIndex;

			// append all subnodes for selection
			for (int32 Idx = 0; Idx < DialogEdNode->SubNodes.Num(); Idx++)
			{
				DialogEdNode->SubNodes[Idx]->CopySubNodeIndex = CopySubNodeIndex;
				SubNodes.Add(DialogEdNode->SubNodes[Idx]);
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
		UDialogBuilderEdNode* Node = Cast<UDialogBuilderEdNode>(*SelectedIter);
		if (Node)
		{
			Node->PostCopyNode();
		}
	}
}

bool FDialogBuilderEditor::CanCopyNodes()
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
void FDialogBuilderEditor::PasteNodes()
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
void FDialogBuilderEditor::PasteNodesHere(UEdGraph* DestinationGraph, const FVector2D& Location)
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
		UDialogBuilderEdGraph* DialogEdGraph = Cast<UDialogBuilderEdGraph>(EdGraph);

		EdGraph->Modify();

		if (DialogEdGraph)
		{
			DialogEdGraph->LockUpdates();
		}

		UDialogBuilderEdNode* SelectedParent = NULL;
		bool bHasMultipleNodesSelected = false;

		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
		{
			UDialogBuilderEdNode* Node = Cast<UDialogBuilderEdNode>(*SelectedIter);
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
			UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(EdNode);
			if (EdNode && (DialogEdNode == nullptr || !DialogEdNode->IsSubNode()))
			{
				AvgNodePosition.X += EdNode->NodePosX;
				AvgNodePosition.Y += EdNode->NodePosY;
				++AvgCount;
			}
			
			if (ShouldGetNewID && DialogEdNode && !DialogEdNode->IsSubNode())
			{
				UDialogBuilderNode* DialogNode = DialogEdNode ? Cast<UDialogBuilderNode>(DialogEdNode->NodeInstance) : nullptr;
				if (DialogNode)
				{
					DialogEdNode->FindUniqueNodeName(DialogNode->ID.ToString());
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

		TMap<int32, UDialogBuilderEdNode*> ParentMap;
		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* PasteNode = *It;
			UDialogBuilderEdNode* PasteDialogEdNode = Cast<UDialogBuilderEdNode>(PasteNode);

			if (PasteNode && (PasteDialogEdNode == nullptr || !PasteDialogEdNode->IsSubNode()))
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

				if (PasteDialogEdNode)
				{
					PasteDialogEdNode->RemoveAllSubNodes();
					ParentMap.Add(PasteDialogEdNode->CopySubNodeIndex, PasteDialogEdNode);
				}
			}
		}

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UDialogBuilderEdNode* PasteNode = Cast<UDialogBuilderEdNode>(*It);
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

		if (DialogEdGraph)
		{
			DialogEdGraph->UpdateClassData();
			DialogEdGraph->UnlockUpdates();
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


void FDialogBuilderEditor::FixupPastedNodes(const TSet<UEdGraphNode*>& NewPastedGraphNodes, const TMap<FGuid, FGuid>& NewToOldNodeMapping)
{
}

bool FDialogBuilderEditor::CanPasteNodes() const
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
void FDialogBuilderEditor::DuplicateNodes()
{
	ShouldGetNewID = true;
	CopySelectedNodes();
	PasteNodes();
}
bool FDialogBuilderEditor::CanDuplicateNodes()
{
	return CanCopyNodes();
}

void FDialogBuilderEditor::HandleNewNodeClassPicked(UClass* InClass) const
{

	if (EditingDialogGraph != nullptr && InClass != nullptr && EditingDialogGraph->GetOutermost())
	{
		const FString ClassName = FBlueprintEditorUtils::GetClassNameWithoutSuffix(InClass);

		FString PathName = EditingDialogGraph->GetOutermost()->GetPathName();
		PathName = FPaths::GetPath(PathName);

		// Now that we've generated some reasonable default locations/names for the package, allow the user to have the final say
		// before we create the package and initialize the blueprint inside of it.
		FSaveAssetDialogConfig SaveAssetDialogConfig;
		SaveAssetDialogConfig.DialogTitleOverride = LOCTEXT("SaveAssetDialogTitle", "Save Asset As");
		SaveAssetDialogConfig.DefaultPath = PathName;
		SaveAssetDialogConfig.DefaultAssetName = ClassName + TEXT("_New");
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

void FDialogBuilderEditor::CreateNewDialogDecorator()
{
	HandleNewNodeClassPicked(UOrionDecorator::StaticClass());
}
bool FDialogBuilderEditor::CanCreateDialogDecorator() const
{
	return true;
}
void FDialogBuilderEditor::CreateNewDialogEvent()
{
	HandleNewNodeClassPicked(UOrionEvent::StaticClass());
}
bool FDialogBuilderEditor::CanCreateDialogEvent() const
{
	return true;
}
void FDialogBuilderEditor::CreateNewDialogCameraShot()
{
	HandleNewNodeClassPicked(UDialogCameraShot::StaticClass());
}
bool FDialogBuilderEditor::CanCreateDialogCameraShot() const
{
	return true;
}
void FDialogBuilderEditor::OpenDialogSetting()
{
	PropertyWidget->SetObject(GetDialogBuilderGraph());
}
bool FDialogBuilderEditor::CanOpenDialogSetting() const
{
	return true;
}
void FDialogBuilderEditor::OnRenameNode()
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
bool FDialogBuilderEditor::CanRenameNodes() const
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

bool FDialogBuilderEditor::CanCreateComment() const
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	return CurrentGraphEditor.IsValid();
}

void FDialogBuilderEditor::OnCreateComment()
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

void FDialogBuilderEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	TArray<UObject*> Selection = FDialogBuilderEditorUtils::GetSelectionForPropertyEditor(NewSelection);

	if (Selection.Num() == 0)
	{
		if (UDialogBuilderEdGraph* DialogGraph = Cast<UDialogBuilderEdGraph>(CurrentGraphWidget->GetCurrentGraph()))
		{
			PropertyWidget->SetObject(DialogGraph->OwningDialog);
		}
	}
	else if (Selection.Num() == 1)
	{
		if (UDialogBuilderNode_Root* RootNode = Cast<UDialogBuilderNode_Root>(Selection[0]))
		{
			if (UDialogBuilderEdGraph* DialogGraph = Cast<UDialogBuilderEdGraph>(CurrentGraphWidget->GetCurrentGraph()))
			{
				PropertyWidget->SetObject(DialogGraph->OwningDialog);
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

void FDialogBuilderEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	UDialogBuilderEdNode* DialogEdNode = Cast<UDialogBuilderEdNode>(Node);

	if (DialogEdNode && 
		DialogEdNode->NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		UClass* NodeClass = DialogEdNode->NodeInstance->GetClass();
		UPackage* Pkg = NodeClass->GetOuterUPackage();
		FString ClassName = NodeClass->GetName().LeftChop(2);
		UBlueprint* BlueprintOb = FindObject<UBlueprint>(Pkg, *ClassName);
		if (BlueprintOb)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(BlueprintOb);
		}
	}
}

void FDialogBuilderEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (EditingDialogGraph == nullptr)
		return;

	for (UEdGraph* EdGraph : EditingDialogGraph->DialogGraphPages)
	{
		EdGraph->GetSchema()->ForceVisualizationCacheClear();
	}
	DocumentManager->RefreshAllTabs();

}
#include "Async/Async.h"

void FDialogBuilderEditor::OnPackageMarkedDirty(UPackage* ModifiedPackage, bool bWasDirty)
{   
}
//Called when saving our file graph
#if ENGINE_MAJOR_VERSION < 5

void FDialogBuilderEditor::OnPackageSaved(const FString& PackageFileName, UObject* Outer)
{
	RebuildDialogBuilderGraphPages();
}
#else // #if ENGINE_MAJOR_VERSION < 5
void FDialogBuilderEditor::OnPackageSavedWithContext(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext ObjectSaveContext)
{
	RebuildDialogBuilderGraphPages();
	
}


#endif // #else // #if ENGINE_MAJOR_VERSION < 5

#undef LOCTEXT_NAMESPACE
