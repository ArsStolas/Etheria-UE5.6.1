// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilderEditor.h"
#include "DialogBuilderNode_Root.h"
#include "DialogBuilderEditorToolbar.h"
#include "DialogBuilder_EditorCommands.h"
#include "DialogBuilderEdGraph.h"
#include "DialogBuilderEdNode.h"
#include "DialogBuilderEditorModes.h"
#include "MovieSceneDialogSection.h"
#include "DialogSequenceShot.h"
#include "Components/StaticMeshComponent.h"
#include "SDialogStageManager.h"
#include "SDialogCameraPresets.h"
#include "DialogSequencePlaybackContext.h"
#include "DialogDefinition.h"
#include "DialogSequence.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "MovieSceneObjectBindingID.h"
#include "DialogStage.h"
#include "DialogBuilderEdNode.h"
#include "DialogBuilderViewportClient.h"
#include "DialogBuilderNode_DialogSequence.h"
#include "Decorator/OrionDecorator.h"
#include "EngineGlobals.h"
#include "Editor/EditorEngine.h"
#include "Editor.h"
#include "DrawDebugHelpers.h"
#include "UnrealEdGlobals.h"
#include "Event/OrionEvent.h"
#include "DialogCameraShot.h"
#include "BlueprintEditor.h"
#include "UObject/ObjectSaveContext.h"
#include "GraphEditorActions.h"
#include "Framework/Commands/GenericCommands.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "MovieScenePossessable.h"
#include "K2Node.h"
#include "Components/ArrowComponent.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/DirectionalLight.h"
#include "Containers/Ticker.h"
#include "GraphEditAction.h"
#include "DialogBuilderEdNode_Edge.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "EdGraphSchema_DialogBuilder.h"
#include "DialogBuilderEditorUtils.h"
#include "DialogBuilderFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SDialogDefinitions.h"
#include "Dialog_System_Editor.h"
#include "ContentBrowserModule.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "ContentBrowserFrontEndFilterExtension.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/LevelSequenceEditorSpawnRegister.h"
#include "ISequencer.h"
#include "ISequencerModule.h"
#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneSignedObject.h"
#include "MovieSceneDialogTrack.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "LevelSequence.h"
#include "GameFramework/Character.h"
#include "CineCameraComponent.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SNullWidget.h"
#include "MovieSceneSpawnRegister.h"
#include "PreviewScene.h"
#include "SDialogPreviewViewport.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "UObject/SoftObjectPath.h"
#include "CineCameraActor.h"
#include "SequencerUtilities.h"
#include "MovieSceneToolHelpers.h"
#include "Components/LocalLightComponent.h"
#include "Components/LightComponent.h"
#include "Tracks/MovieSceneObjectPropertyTrack.h"
#include "Tracks/MovieSceneActorReferenceTrack.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Sections/MovieSceneActorReferenceSection.h"
#include "Bindings/MovieSceneCustomBinding.h"
#include "Bindings/MovieSceneSpawnableBinding.h"
#include "Bindings/MovieSceneReplaceableBinding.h"
#include "Bindings/MovieSceneSpawnableActorBinding.h"
#include "Bindings/MovieSceneReplaceableActorBinding.h"
#include "MVVM/ViewModels/ObjectBindingModel.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "MVVM/ViewModels/ViewModel.h"
#include "MVVM/ViewModels/TrackModel.h"
#include "MVVM/ViewModels/SequenceModel.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "Engine/AssetManager.h"
#include <Kismet2/KismetEditorUtilities.h>
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Channels/MovieSceneDoubleChannel.h"
#include "DialogGenerateCameraSettings.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Editor/TransBuffer.h"
#include "Elements/Framework/TypedElementList.h"
#include "Elements/Framework/TypedElementSelectionSet.h"
#include "Selection.h"

#define LOCTEXT_NAMESPACE "DialogBuilderEditor"

namespace
{
	void ResetSelectionWithoutResolvingElements(USelection* Selection)
	{
		if (!Selection)
		{
			return;
		}

		if (UTypedElementSelectionSet* SelectionSet = Selection->GetElementSelectionSet())
		{
			FTypedElementListConstRef ElementList = SelectionSet->GetElementList();
			const_cast<FTypedElementList&>(ElementList.Get()).Reset();
			Selection->NoteSelectionChanged();
		}
	}

	void ResetEditorSelectionWithoutResolvingElements()
	{
		if (!GEditor)
		{
			return;
		}

		ResetSelectionWithoutResolvingElements(GEditor->GetSelectedActors());
		ResetSelectionWithoutResolvingElements(GEditor->GetSelectedComponents());
	}

	void DestroyPreviewActor(UWorld* PreviewWorld, AActor* Actor)
	{
		if (!PreviewWorld || !IsValid(Actor))
		{
			return;
		}

		Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Actor->SetActorTickEnabled(false);
		Actor->SetTickableWhenPaused(false);

		TArray<UPrimitiveComponent*> PrimitiveComponents;
		Actor->GetComponents(PrimitiveComponents);
		for (UPrimitiveComponent* Component : PrimitiveComponents)
		{
			if (!Component)
			{
				continue;
			}

			Component->SetSimulatePhysics(false);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Component->Deactivate();
			Component->UnregisterComponent();
		}

		TArray<UActorComponent*> ActorComponents;
		Actor->GetComponents(ActorComponents);
		for (UActorComponent* Component : ActorComponents)
		{
			if (Component && Component->IsRegistered())
			{
				Component->UnregisterComponent();
			}
		}

		PreviewWorld->EditorDestroyActor(Actor, false);
	}

	void DisablePreviewActorPhysics(AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return;
		}

		Actor->SetActorTickEnabled(false);
		Actor->SetTickableWhenPaused(false);
		Actor->PrimaryActorTick.bStartWithTickEnabled = false;

		TArray<UPrimitiveComponent*> PrimitiveComponents;
		Actor->GetComponents(PrimitiveComponents);
		for (UPrimitiveComponent* Component : PrimitiveComponents)
		{
			if (!Component)
			{
				continue;
			}

			Component->SetSimulatePhysics(false);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

const FName FDialogBuilderEditor::DialogEditorMode(TEXT("DialogEditor"));
const FName FDialogBuilderEditor::DialogSequencerMode(TEXT("DialogSequencer"));

FText FDialogBuilderEditor::DialogEditorModeText(LOCTEXT("DialogEditorMode", "Dialog Editor"));
FText FDialogBuilderEditor::DialogSequencerModeText(LOCTEXT("DialogSequencerMode", "Sequencer"));

const FName FDialogBuilderEditorTabs::DialogDefinitionsID(TEXT("DialogDefinitions"));
const FName FDialogBuilderEditorTabs::DialogBuilderPropertyID(TEXT("DialogBuilderProperty"));
const FName FDialogBuilderEditorTabs::ViewportID(TEXT("Viewport"));
const FName FDialogBuilderEditorTabs::DialogBuilderEditorSettingsID(TEXT("DialogBuilderEditorSettings"));
const FName FDialogBuilderEditorTabs::DialogSequencerTabID(TEXT("DialogSequencerTab"));
const FName FDialogBuilderEditorTabs::DialogStageSettingsID(TEXT("DialogStageSettingsID"));
const FName FDialogBuilderEditorTabs::DialogSequencerViewportID(TEXT("DialogSequencerViewport"));
const FName FDialogBuilderEditorTabs::DialogCameraPresetsID(TEXT("DialogCameraPresets"));
const FName FDialogBuilderEditorTabs::SequencerGraphEditor(TEXT("SequencerGraphEditor"));

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
	bIsClosing = true;

	// Unbind raw package dirty callback added in Initialize()
	if (EditingDialogGraph && EditingDialogGraph->GetOutermost())
	{
		EditingDialogGraph->GetOutermost()->PackageMarkedDirtyEvent.RemoveAll(this);
	}
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);

		if (TransactionStateChangedHandle.IsValid())
		{
			if (UTransBuffer* TransBuffer = Cast<UTransBuffer>(GEditor->Trans))
			{
				TransBuffer->OnTransactionStateChanged().Remove(TransactionStateChangedHandle);
			}
			TransactionStateChangedHandle.Reset();
		}
	}

	UnbindSequencerDelegates();
	UnbindDelegates();

	if (Sequencer.IsValid())
	{
		ResetEditorSelectionWithoutResolvingElements();
		Sequencer->Close();
	}

	Sequencer.Reset();
	PlaybackContext.Reset();


	UToolMenus::UnregisterOwner(this);
	if (DialogViewportWidget.IsValid())
	{
		DialogViewportWidget->OnActorUnlock();
		DialogViewportWidget->RemoveDialogOverlayWidget();
	}
	DestroyDialogStage();
	
	if (UWorld* World = PreviewScene.GetWorld())
	{
		
		World->CleanupWorld(true, true);
	}
	
	
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
	

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);

		if (UTransBuffer* TransBuffer = Cast<UTransBuffer>(GEditor->Trans))
		{
			TransactionStateChangedHandle = TransBuffer->OnTransactionStateChanged().AddRaw(
				this,
				&FDialogBuilderEditor::OnTransactionStateChanged);
		}
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

	const TArray<UObject*>* EditedObjects = GetObjectsCurrentlyBeingEdited();
	if (EditedObjects == nullptr || EditedObjects->Num() == 0)
	{
		FGenericCommands::Register();
		FGraphEditorCommands::Register();
		FDialogDefinitionsCommands::Register();
		FDialogBuilder_EditorCommands::Register();

		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

		//ToolbarBuilder->AddDialogSystemToolbar(ToolbarExtender);

		BindCommands();
		CreateInternalWidgets();
		const bool bCreateDefaultStandaloneMenu = true;
		const bool bCreateDefaultToolbar = true;
		InitAssetEditor(Mode, InitToolkitHost, DialogBuilderEditorAppName, FTabManager::FLayout::NullLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectsToEdit);

		EnsureSequencerCreated();
		FDialog_System_EditorModule& DialogBuilderEditorModule = FModuleManager::LoadModuleChecked<FDialog_System_EditorModule>("Dialog_System_Editor");
		AddMenuExtender(DialogBuilderEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
		AddToolbarExtender(DialogBuilderEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

		AddApplicationMode(DialogEditorMode, MakeShareable(new FDialogBuilderEditorApplicationMode(SharedThis(this))));
		AddApplicationMode(DialogSequencerMode, MakeShareable(new FDialogBuilderSequencerApplicationMode(SharedThis(this))));


		if (DialogGraphToEdit != nullptr)
		{
			SetCurrentMode(DialogSequencerMode);
		}

		if (EditingDialogGraph)
		{
			EditingDialogGraph->EnsurePlayerDefinition();
			if (EditingDialogGraph->bIsNewlyCreated)
			{
				NewDocument_OnClicked(CGT_NewDialogGraph);
				EditingDialogGraph->bIsNewlyCreated = false;
				EditingDialogGraph->Modify();
			}
			else if (EditingDialogGraph->DialogGraphPages.Num() > 0)
			{
				OpenDocument(EditingDialogGraph->DialogGraphPages[0], FDocumentTracker::OpenNewDocument);

				//bind on graph changes
				for (UEdGraph* EdGraph : EditingDialogGraph->DialogGraphPages)
				{
					if (EdGraph)
					{
						if (UDialogBuilderEdGraph* DialogEdGraph = Cast<UDialogBuilderEdGraph>(EdGraph))
						{
							DialogEdGraph->DialogEditorPtr = SharedThis(this);
							DialogEdGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FDialogBuilderEditor::OnGraphChanged));
						}
					}
				}

			}

			if (EditingDialogGraph->CurrentEditingSequenceNode)
			{
				OnOpenDialogSequenceNode(EditingDialogGraph->CurrentEditingSequenceNode);
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


	UpdateDialogStage();
	ApplyViewportCameraMode();

	BindDelegates();
}


void FDialogBuilderEditor::BindDelegates()
{
	FEditorDelegates::MapChange.AddRaw(this, &FDialogBuilderEditor::OnMapChange);

	FEditorDelegates::PostPIEStarted.AddRaw(this, &FDialogBuilderEditor::OnPieEvent);
	FEditorDelegates::PrePIEEnded.AddRaw(this, &FDialogBuilderEditor::OnPieEvent);
	FWorldDelegates::OnWorldCleanup.AddRaw(this, &FDialogBuilderEditor::OnWorldCleanup);

	if (GEngine)
	{
		GEngine->OnWorldAdded().AddRaw(this, &FDialogBuilderEditor::OnWorldAdded);
		GEngine->OnWorldDestroyed().AddRaw(this, &FDialogBuilderEditor::OnWorldDestroyed);
	}
}

void FDialogBuilderEditor::UnbindDelegates()
{
	FEditorDelegates::MapChange.RemoveAll(this);
	FEditorDelegates::PostPIEStarted.RemoveAll(this);
	FEditorDelegates::PrePIEEnded.RemoveAll(this);
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);
	if (GEngine)
	{
		GEngine->OnWorldAdded().RemoveAll(this);
		GEngine->OnWorldDestroyed().RemoveAll(this);
	}
}

void FDialogBuilderEditor::OnPieEvent(bool)
{
	if (!Sequencer.IsValid())
	{
		return;
	}

	const TSharedRef<SWidget> SequencerWidget = Sequencer->GetSequencerWidget();

	if (SequencerWidget->IsEnabled())
	{
		Sequencer->Pause();
		SequencerWidget->SetEnabled(false);
	}
	else
	{
		SequencerWidget->SetEnabled(true);
		Sequencer->ForceEvaluate();
		RefreshSequencerCameraLock();
	}

}

void FDialogBuilderEditor::OnMapChange(uint32)
{
}


void FDialogBuilderEditor::OnWorldAdded(UWorld*)
{
	//For some reason, sequencer won't immediately resolve the binding, add a delay instead
	const TWeakPtr<FDialogBuilderEditor> EditorWeak = SharedThis(this);

	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([EditorWeak](float)
			{
				if (const TSharedPtr<FDialogBuilderEditor> EditorPinned = EditorWeak.Pin())
				{
					EditorPinned->UpdateDialogStage();
					EditorPinned->ApplyViewportCameraMode();
				}
				return false;
			}),
		1.0f);
}

void FDialogBuilderEditor::OnWorldDestroyed(UWorld*)
{
}


void FDialogBuilderEditor::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (!World)
	{
		return;
	}

	if (DialogViewportWidget.IsValid())
	{
		DialogViewportWidget->RemoveDialogOverlayWidget();
	}

}

void FDialogBuilderEditor::OnTransactionStateChanged(const FTransactionContext& /*TransactionContext*/, ETransactionStateEventType TransactionState)
{
	if (TransactionState == ETransactionStateEventType::UndoRedoStarted)
	{
		CloseSequencerForUndoRedo();
	}
}

void FDialogBuilderEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	DocumentManager->SetTabManager(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_DialogBuilderEditor", "Generic Graph Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::DialogDefinitionsID, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_DialogDefinitions))
		.SetDisplayName(LOCTEXT("Dialog Details", "My Dialog"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("GraphCanvasTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));


	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::DialogStageSettingsID, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_DialogStageSettings))
		.SetDisplayName(LOCTEXT("DetailsTab", "Dialog Stage"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::DialogBuilderPropertyID, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Property"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::DialogSequencerTabID, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_Sequencer))
		.SetDisplayName(LOCTEXT("DialogSequencerTab", "Sequencer"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Cinematics"));

	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::DialogSequencerViewportID, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_SequencerViewport))
		.SetDisplayName(LOCTEXT("DialogSequencerViewportTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
	

	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::DialogCameraPresetsID, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_CameraPresets))
		.SetDisplayName(LOCTEXT("DialogCameraPresetsTab", "Camera Presets"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.LockCamera"));


	const FSlateIcon SequencerGraphIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCurveEditor.TabIcon");
	InTabManager->RegisterTabSpawner(FDialogBuilderEditorTabs::SequencerGraphEditor, FOnSpawnTab::CreateSP(this, &FDialogBuilderEditor::SpawnTab_CurveEditor))
		.SetMenuType(ETabSpawnerMenuType::Type::Hidden)
		.SetIcon(SequencerGraphIcon);
}

void FDialogBuilderEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::DialogDefinitionsID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::ViewportID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::DialogBuilderPropertyID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::DialogStageSettingsID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::DialogBuilderEditorSettingsID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::DialogSequencerTabID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::DialogSequencerViewportID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::DialogCameraPresetsID);
	InTabManager->UnregisterTabSpawner(FDialogBuilderEditorTabs::SequencerGraphEditor);
}

void FDialogBuilderEditor::Tick(float DeltaTime)
{
	if (bIsClosing)
	{
		return;
	}

	if (IsPIESimulating())
	{
		return;
	}
	PreviewScene.UpdateCaptureContents();
	if (bPendingRefreshDialogEditor && !bIsRefreshDialogEditorInProgress)
	{
		bPendingRefreshDialogEditor = false;

		TGuardValue<bool> ReentryGuard(bIsRefreshDialogEditorInProgress, true);
		UpdateDialogStage();
		if (EditingDialogSequence) EditingDialogSequence->RefreshSequence();
	}

}

TStatId FDialogBuilderEditor::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FDialogBuilderEditor, STATGROUP_Tickables);
}

void FDialogBuilderEditor::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
		if (TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor())
		{
			CurrentGraphEditor->ClearSelectionSet();
			CurrentGraphEditor->NotifyGraphChanged();
		}

		if (DialogDefinitionsWidget.IsValid())
		{
			DialogDefinitionsWidget->Refresh();
		}

		if (PropertyWidget.IsValid())
		{
			PropertyWidget->ForceRefresh();
		}

		if (DialogStageManagerWidget.IsValid())
		{
			DialogStageManagerWidget->Refresh();
		}

		bPendingRefreshDialogEditor = true;

		FSlateApplication::Get().DismissAllMenus();

		UDialogBuilderNode_DialogSequence* SequenceNode = CurrentSequenceNode.Get();
		if (!SequenceNode || !SequenceNode->DialogSequence)
		{
			DestroyDialogStage();
			CurrentSequenceNode.Reset();
			DialogStageTemplate.Reset();
			if (DialogStageWidget.IsValid())
			{
				DialogStageWidget->SetObject(nullptr);
			}
			if (EditingDialogGraph)
			{
				EditingDialogGraph->CurrentEditingSequenceNode = nullptr;
			}
			if (DialogViewportWidget.IsValid())
			{
				DialogViewportWidget->RemoveDialogOverlayWidget();
			}
			EditingDialogSequence = UDialogSequence::GetNullDialogSequence();
			OpenDialogSequence(EditingDialogSequence);
			if (Sequencer.IsValid())
			{
				Sequencer->GetSequencerWidget()->SetEnabled(false);
				Sequencer->SetAutoChangeMode(EAutoChangeMode::None);
			}
		}
		else
		{
			EditingDialogSequence = SequenceNode->DialogSequence;
			if (!Sequencer.IsValid())
			{
				OpenDialogSequence(EditingDialogSequence);
				
			}
			else
			{
				Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::RefreshTree);
				Sequencer->ForceEvaluate();
			}
			UpdateDialogStage();
		}
	}
}

void FDialogBuilderEditor::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{
		if (TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor())
		{
			CurrentGraphEditor->ClearSelectionSet();
			CurrentGraphEditor->NotifyGraphChanged();
		}

		if (DialogDefinitionsWidget.IsValid())
		{
			DialogDefinitionsWidget->Refresh();
		}

		if (PropertyWidget.IsValid())
		{
			PropertyWidget->ForceRefresh();
		}

		if (DialogStageManagerWidget.IsValid())
		{
			DialogStageManagerWidget->Refresh();
		}

		bPendingRefreshDialogEditor = true;

		FSlateApplication::Get().DismissAllMenus();

		UDialogBuilderNode_DialogSequence* SequenceNode = CurrentSequenceNode.Get();
		if (!SequenceNode || !SequenceNode->DialogSequence)
		{
			DestroyDialogStage();
			CurrentSequenceNode.Reset();
			DialogStageTemplate.Reset();
			if (DialogStageWidget.IsValid())
			{
				DialogStageWidget->SetObject(nullptr);
			}
			if (EditingDialogGraph)
			{
				EditingDialogGraph->CurrentEditingSequenceNode = nullptr;
			}
			if (DialogViewportWidget.IsValid())
			{
				DialogViewportWidget->RemoveDialogOverlayWidget();
			}
			EditingDialogSequence = UDialogSequence::GetNullDialogSequence();
			OpenDialogSequence(EditingDialogSequence);
			if (Sequencer.IsValid())
			{
				Sequencer->GetSequencerWidget()->SetEnabled(false);
				Sequencer->SetAutoChangeMode(EAutoChangeMode::None);
			}
		}
		else
		{
			EditingDialogSequence = SequenceNode->DialogSequence;
			if (!Sequencer.IsValid())
			{
				OpenDialogSequence(EditingDialogSequence);
				
			}
			else
			{
				Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::RefreshTree);
				Sequencer->ForceEvaluate();
			}
			UpdateDialogStage();
		}

	}
}




void FDialogBuilderEditor::OnDialogDefinitionAdded(UDialogDefinition* InDialogDefinition)
{

}

void FDialogBuilderEditor::OnDialogDefinitionRemoved(UDialogDefinition* InDialogDefinition)
{
	
}

namespace
{
	template<typename TrackType>
	static void AddTrackIfMissing(UMovieScene* MovieScene, const FGuid& BindingId)
	{
		if (!MovieScene || !BindingId.IsValid())
		{
			return;
		}

		const FMovieSceneBinding* Binding = MovieScene->FindBinding(BindingId);
		if (Binding)
		{
			for (UMovieSceneTrack* Track : Binding->GetTracks())
			{
				if (Track && Track->IsA(TrackType::StaticClass()))
				{
					return;
				}
			}
		}

		MovieScene->AddTrack<TrackType>(BindingId);
	}

	static void AddFloatPropertyTrackIfMissing(
		UMovieScene* MovieScene,
		const FGuid& BindingId,
		const FName PropertyName,
		const TCHAR* PropertyPath)
	{
		if (!MovieScene || !BindingId.IsValid() || !PropertyPath)
		{
			return;
		}

		if (const FMovieSceneBinding* Binding = MovieScene->FindBinding(BindingId))
		{
			for (UMovieSceneTrack* Track : Binding->GetTracks())
			{
				if (const UMovieSceneFloatTrack* FloatTrack = Cast<UMovieSceneFloatTrack>(Track))
				{
					if (FloatTrack->GetPropertyPath() == PropertyPath)
					{
						return;
					}
				}
			}
		}

		if (UMovieSceneFloatTrack* NewTrack = MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingId))
		{
			NewTrack->SetPropertyNameAndPath(PropertyName, PropertyPath);
		}
	}


	static void AddActorToTrackPropertyTrackIfMissing(UMovieScene* MovieScene, const FGuid& ComponentBindingId)
	{
		if (!MovieScene || !ComponentBindingId.IsValid())
		{
			return;
		}

		const FName PropertyName(TEXT("ActorToTrack"));
		const FString PropertyPath(TEXT("FocusSettings.TrackingFocusSettings.ActorToTrack"));

		UMovieSceneActorReferenceTrack* TrackToUse = nullptr;

		if (const FMovieSceneBinding* Binding = MovieScene->FindBinding(ComponentBindingId))
		{
			for (UMovieSceneTrack* Track : Binding->GetTracks())
			{
				if (UMovieSceneActorReferenceTrack* Existing = Cast<UMovieSceneActorReferenceTrack>(Track))
				{
					if (Existing->GetPropertyPath().ToString() == PropertyPath)
					{
						TrackToUse = Existing;
						break;
					}
				}
			}
		}

		if (!TrackToUse)
		{
			TrackToUse = MovieScene->AddTrack<UMovieSceneActorReferenceTrack>(ComponentBindingId);
			if (!TrackToUse)
			{
				return;
			}

			TrackToUse->SetPropertyNameAndPath(PropertyName, PropertyPath);
		}

		// Ensure section exists so Sequencer shows key area/actions immediately.
		if (TrackToUse->GetAllSections().Num() == 0)
		{
			if (UMovieSceneSection* NewSection = TrackToUse->CreateNewSection())
			{
				TrackToUse->AddSection(*NewSection);
				NewSection->SetRange(TRange<FFrameNumber>::All());
			}
		}
	}

	static void AddDefaultTracksForBinding(
		UMovieScene* MovieScene,
		UDialogSequence* OwningSequence,
		ISequencer* InSequencer,
		const FGuid& BindingId,
		AActor* BoundActor,
		bool bIsCamera)
	{
		if (!MovieScene || !BoundActor || !BindingId.IsValid())
		{
			return;
		}

		MovieScene->Modify();

		if (bIsCamera || BoundActor->IsA(ACineCameraActor::StaticClass()))
		{
			AddTrackIfMissing<UMovieScene3DTransformTrack>(MovieScene, BindingId);

			if (ACineCameraActor* CineCam = Cast<ACineCameraActor>(BoundActor))
			{
				if (UCineCameraComponent* CineCameraComponent = CineCam->GetCineCameraComponent())
				{
					FGuid CineComponentBindingId;

					if (InSequencer)
					{
						CineComponentBindingId = InSequencer->GetHandleToObject(CineCameraComponent, true);
					}

					if (CineComponentBindingId.IsValid())
					{
						// Force component binding to be a child of camera actor binding.
						if (FMovieScenePossessable* ComponentPossessable = MovieScene->FindPossessable(CineComponentBindingId))
						{
							if (ComponentPossessable->GetParent() != BindingId)
							{
								ComponentPossessable->SetParent(BindingId, MovieScene);
							}
						}

						// Ensure object binding exists in sequence context.
						if (OwningSequence && InSequencer)
						{
							OwningSequence->BindPossessableObject(
								CineComponentBindingId,
								*CineCameraComponent,
								InSequencer->GetPlaybackContext());
						}

						// Focus settings property track on CineCameraComponent binding.
						AddActorToTrackPropertyTrackIfMissing(
							MovieScene,
							CineComponentBindingId);

						/*AddFloatPropertyTrackIfMissing(
							MovieScene,
							CineComponentBindingId,
							FName(TEXT("Actor to Track (Tracking Focus Settings)")),
							TEXT("FocusSettings.TrackingFocusSettings.ActorToTrack"));*/
					}
				}
			}

			return;
		}

		AddTrackIfMissing<UMovieScene3DTransformTrack>(MovieScene, BindingId);
	}

	static UMovieSceneActorReferenceTrack* FindActorToTrackPropertyTrack(UMovieScene* MovieScene, const FGuid& ComponentBindingId)
	{
		if (!MovieScene || !ComponentBindingId.IsValid())
		{
			return nullptr;
		}

		const FMovieSceneBinding* Binding = MovieScene->FindBinding(ComponentBindingId);
		if (!Binding)
		{
			return nullptr;
		}

		const FString PropertyPath(TEXT("FocusSettings.TrackingFocusSettings.ActorToTrack"));

		for (UMovieSceneTrack* Track : Binding->GetTracks())
		{
			if (UMovieSceneActorReferenceTrack* ActorRefTrack = Cast<UMovieSceneActorReferenceTrack>(Track))
			{
				if (ActorRefTrack->GetPropertyPath().ToString() == PropertyPath)
				{
					return ActorRefTrack;
				}
			}
		}

		return nullptr;
	}

	static void AddActorToTrackKeyIfValid(
		UMovieScene* MovieScene,
		ISequencer* InSequencer,
		UCineCameraComponent* CinecamComp,
		const FFrameNumber InFrame)
	{
		if (!MovieScene || !InSequencer || !CinecamComp)
		{
			return;
		}

		AActor* ActorToTrack = CinecamComp->FocusSettings.TrackingFocusSettings.ActorToTrack.Get();
		if (!IsValid(ActorToTrack))
		{
			return;
		}

		const FGuid ComponentBindingId = InSequencer->GetHandleToObject(CinecamComp, true);
		if (!ComponentBindingId.IsValid())
		{
			return;
		}

		AddActorToTrackPropertyTrackIfMissing(MovieScene, ComponentBindingId);

		UMovieSceneActorReferenceTrack* ActorRefTrack = FindActorToTrackPropertyTrack(MovieScene, ComponentBindingId);
		if (!ActorRefTrack)
		{
			return;
		}

		UMovieSceneActorReferenceSection* ActorRefSection = nullptr;
		if (ActorRefTrack->GetAllSections().Num() == 0)
		{
			ActorRefSection = Cast<UMovieSceneActorReferenceSection>(ActorRefTrack->CreateNewSection());
			if (!ActorRefSection)
			{
				return;
			}

			ActorRefTrack->AddSection(*ActorRefSection);
			ActorRefSection->SetRange(TRange<FFrameNumber>::All());
		}
		else
		{
			ActorRefSection = Cast<UMovieSceneActorReferenceSection>(ActorRefTrack->GetAllSections()[0]);
			if (!ActorRefSection)
			{
				return;
			}
		}

		const FGuid ActorBindingId = InSequencer->GetHandleToObject(ActorToTrack, true);
		if (!ActorBindingId.IsValid())
		{
			return;
		}

		TArrayView<FMovieSceneActorReferenceData*> Channels =
			ActorRefSection->GetChannelProxy().GetChannels<FMovieSceneActorReferenceData>();

		if (!Channels.IsValidIndex(0) || !Channels[0])
		{
			return;
		}

		ActorRefTrack->Modify();
		ActorRefSection->Modify();

		FMovieSceneActorReferenceKey NewKey;
		NewKey.Object = UE::MovieScene::FRelativeObjectBindingID(ActorBindingId);

		Channels[0]->GetData().UpdateOrAddKey(InFrame, NewKey);
	}
}

void FDialogBuilderEditor::CreateDialogTrackFromSlot(UDialogSequenceSlot* InSlot, bool bIsCamera)
{
	if (!InSlot)
	{
		return;
	}

	if (!InSlot->DialogDefinition)
	{
		return;
	}

	UWorld* PreviewWorld = GetPreviewWorld();
	check(PreviewWorld);

	UDialogDefinition* InDialogDefinition = InSlot->DialogDefinition;

	if(InDialogDefinition->IsA<UDialogLight>())
	{
		// no need tp create track for light,
		return;
	}

	UMovieScene* MovieScene = EditingDialogSequence ? EditingDialogSequence->GetMovieScene() : nullptr;
	if (!MovieScene || !InDialogDefinition)
	{
		return;
	}

	AActor* BoundActor = DialogSlotActors.FindRef(InSlot).Get();
	const FString BindingName = InDialogDefinition->DisplayName.IsEmpty() ? BoundActor->GetActorLabel() : InDialogDefinition->DisplayName.ToString();

	if(!BoundActor)
	{
		return;
	}

	FGuid ObjectBinding = InSlot->ID;
	FMovieScenePossessable* BoundDialogDefinition = MovieScene->FindPossessable(ObjectBinding);
	//TArrayView<TWeakObjectPtr<>> BoundObjects = Sequencer->FindBoundObjects(ObjectBinding, Sequencer->GetRootTemplateID());

	if (BoundDialogDefinition)
	{
		if (BoundDialogDefinition->GetName() != BindingName)
		{
			BoundDialogDefinition->SetName(BindingName);
		}
		return;
	}

	if (Sequencer.IsValid())
	{
		FGuid NewBindingId = MovieScene->AddPossessable(BindingName, BoundActor->GetClass());
		FMovieScenePossessable* Possessable = MovieScene->FindPossessable(NewBindingId);

		if (Possessable)
		{
			if (!Possessable->BindSpawnableObject(Sequencer->GetFocusedTemplateID(), BoundActor, Sequencer->GetSharedPlaybackState()))
			{
				EditingDialogSequence->BindPossessableObject(NewBindingId, *BoundActor, Sequencer->GetPlaybackContext());
			}
		}
		// inside FDialogBuilderEditor::CreateDialogTrackFromSlot(...)
		AddDefaultTracksForBinding(MovieScene, EditingDialogSequence, Sequencer.Get(), NewBindingId, BoundActor, bIsCamera);

		InSlot->ID = NewBindingId;

		Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
		Sequencer->ForceEvaluate();
		return;
		
	}
}



void FDialogBuilderEditor::ResolveDialogBoundObjects()
{
	UMovieScene* MovieScene = EditingDialogSequence ? EditingDialogSequence->GetMovieScene() : nullptr;
	if (!MovieScene || !Sequencer.IsValid())
	{
		return;
	}
	TArray<UDialogSequenceSlot*> DialogSlots;
	DialogSlotActors.GenerateKeyArray(DialogSlots);

	bool bAnythingChanged = false;
	for (int32 i = 0; i < DialogSlots.Num(); i++)
	{
		UDialogSequenceSlot* DialogSlot = DialogSlots[i];
		AActor* DialogActor = DialogSlotActors.FindRef(DialogSlot).Get();

		if (DialogSlot && DialogActor)
		{
			FGuid ObjectBinding = DialogSlot->ID;
			FMovieSceneBinding* BoundDialogSlot = MovieScene->FindBinding(ObjectBinding);
			TArrayView<TWeakObjectPtr<>> BoundObjects = Sequencer->FindBoundObjects(ObjectBinding, Sequencer->GetRootTemplateID());

			if (BoundDialogSlot)
			{
				if (!BoundObjects.Contains(DialogActor))
				{
					bAnythingChanged = true;
					EditingDialogSequence->UnbindPossessableObjects(DialogSlot->ID);
					FMovieSceneBindingProxy BindingProxy(DialogSlot->ID, Sequencer->GetFocusedMovieSceneSequence());
					TArray<AActor*> Actors;
					Actors.Add(DialogActor);

					EditingDialogSequence->BindPossessableObject(DialogSlot->ID, *DialogActor, GetPreviewWorld());

					//FSequencerUtilities::ReplaceBindingWithActors(Sequencer.ToSharedRef(), Actors, BindingProxy);
				}
			}
		}
	}

	if (bAnythingChanged)
	{
		Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemsChanged);
		Sequencer->ForceEvaluate();
	}
}

void FDialogBuilderEditor::UpdateDialogStage()
{
	if (bIsClosing)
	{
		return;
	}

	UDialogBuilderGraph* DialogGraph = GetDialogBuilderGraph();
	check(DialogGraph);

	

	if (!GetPreviewScene())
	{
		return;
	}

	UWorld* PreviewWorld = GetPreviewWorld();
	check(PreviewWorld);

	AActor* PivotActor = SequencePivot.Get();
	
	if (!IsValid(PivotActor))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags = RF_Transient | RF_Transactional;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		PivotActor = PreviewWorld->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
		if (PivotActor)
		{
			DisablePreviewActorPhysics(PivotActor);

			USceneComponent* PivotRoot = NewObject<USceneComponent>(PivotActor, TEXT("PivotRoot"));
			PivotActor->SetRootComponent(PivotRoot);
			PivotRoot->RegisterComponent();

			UBillboardComponent* BillboardComponent = NewObject<UBillboardComponent>(PivotActor, TEXT("PivotBillboard"));
			BillboardComponent->SetupAttachment(PivotRoot);
			BillboardComponent->SetMobility(EComponentMobility::Movable);
			BillboardComponent->SetHiddenInGame(false);
			BillboardComponent->SetVisibility(true);
			BillboardComponent->SetUsingAbsoluteScale(true);
			BillboardComponent->bIsScreenSizeScaled = true;
			BillboardComponent->bUseInEditorScaling = true;

			BillboardComponent->SetEditorScale(1.5f);
			BillboardComponent->ScreenSize = 0.025f;
			BillboardComponent->SpriteInfo.Category = TEXT("Dialog");
			BillboardComponent->SpriteInfo.DisplayName = LOCTEXT("DialogSequencePivotSprite", "Dialog Sequence Pivot");

			if (UTexture2D* PivotSprite = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorResources/S_LevelSequence.S_LevelSequence")))
			{
				BillboardComponent->SetSprite(PivotSprite);
			}

			BillboardComponent->RegisterComponent();

			PivotActor->SetActorHiddenInGame(false);

			SequencePivot = PivotActor;
		}
	}
	
	if (!EditingDialogGraph || !CurrentSequenceNode.IsValid()) return;

	DialogStageTemplate = CurrentSequenceNode.Get()->DialogStageToUse;
	UDialogStage* DialogStage = CurrentSequenceNode.Get()->DialogStage;
	if (!DialogStage || !DialogStageTemplate.IsValid())
	{
		DestroyDialogStage();
		return;
	}

	CurrentSequenceNode.Get()->UpdateDialogStageData();
	PivotActor->SetActorLocation(DialogStage->Location);
	PivotActor->SetActorRotation(DialogStage->Rotation);

	for (auto& DialogSlot : DialogStage->Slots)
	{
		AActor* SpawnedActor = nullptr;
		RetrieveDialogSlotActor(DialogSlot, SpawnedActor);
	}

	if (DialogStage->CameraSlots.Num() == 0)
	{
		UDialogSequenceSlot* NewCameraSlot = NewObject<UDialogSequenceSlot>(DialogStage, NAME_None, RF_Transactional);
		DialogStage->CameraSlots.Add(NewCameraSlot);

	}
	for (auto& CameraSlot : DialogStage->CameraSlots)
	{
		if(!CameraSlot) continue;

		if(!CameraSlot->DialogDefinition)
		{
			CameraSlot->DialogDefinition = NewObject<UDialogCamera>(CameraSlot, NAME_None, RF_Transactional);
		}
		AActor* SpawnedCamera = nullptr;
		RetrieveDialogSlotActor(CameraSlot, SpawnedCamera, /*bIsCamera*/true);
	}

	for (auto& LightSlot : DialogStage->LightSlots)
	{
		AActor* SpawnedActor = nullptr;
		RetrieveLightSlotActor(LightSlot, SpawnedActor);
	}

	

	

	// Cleanup: remove entries for participants that no longer exist in the dialog graph.
	for (auto It = DialogSlotActors.CreateIterator(); It; ++It)
	{
		const UDialogSequenceSlot* Key = It.Key();
		if (!Key) continue;
		bool bSlotNotFound = (!DialogStage->Slots.Contains(Key) && !DialogStage->CameraSlots.Contains(Key) && !DialogStage->LightSlots.Contains(Key));

		if (bSlotNotFound)
		{
			if (AActor* Actor = It.Value().Get())
			{
				DestroyPreviewActor(PreviewWorld, Actor);
				if (EditingDialogSequence)
				{
					EditingDialogSequence->UnbindInvalidObjects(Key->ID, PreviewWorld);
				}
			}
			It.RemoveCurrent();
		}
	}


	ResolveDialogBoundObjects();
	ApplyViewportCameraMode();

	//Ensure
	EnsureDialogCameraCutSection();

	//update track model rule
	UpdateTrackModelRule();

	//update section rule
	UpdateSectionRule();

}




void FDialogBuilderEditor::RetrieveDialogSlotActor(UDialogSequenceSlot* InSlot, AActor*& OutActor, bool bIsCamera)
{
	if (bIsClosing)
	{
		return;
	}

	if (!InSlot)
	{
		return;
	}

	

	UWorld* PreviewWorld = GetPreviewWorld();
	check(PreviewWorld);

	UDialogDefinition* InDialogDefinition = InSlot->DialogDefinition;

	const TSoftClassPtr<AActor> ActorSoftClass = InDialogDefinition ? InDialogDefinition->GetActorSoftClass() : nullptr;

	AActor* ExistingActor = nullptr;
	if (TWeakObjectPtr<AActor>* ExistingActorPtr = DialogSlotActors.Find(InSlot))
	{
		ExistingActor = ExistingActorPtr->Get();
	}
	
	if ((!InDialogDefinition || !ActorSoftClass.IsValid()) && ExistingActor)
	{
		DestroyPreviewActor(PreviewWorld, ExistingActor);
		DialogSlotActors.Remove(InSlot);
		return;
	}



	if (!ActorSoftClass.IsNull())
	{
		const TSoftObjectPtr<UDialogSequenceSlot> SlotWeak = InSlot;
		const TWeakPtr<FDialogBuilderEditor> EditorWeak = SharedThis(this);
		const FSoftObjectPath ClassPath = ActorSoftClass.ToSoftObjectPath();

		auto SpawnWithClass = [EditorWeak, SlotWeak, bIsCamera](TSubclassOf<AActor> LoadedActorClass)
			{
				const TSharedPtr<FDialogBuilderEditor> EditorPinned = EditorWeak.Pin();
				UDialogSequenceSlot* Slot = SlotWeak.Get();

				if (!EditorPinned.IsValid() || EditorPinned->bIsClosing || !Slot || !LoadedActorClass)
				{
					return;
				}

				UWorld* PreviewWorld = EditorPinned->GetPreviewWorld();
				if (!PreviewWorld)
				{
					return;
				}

				AActor* ExistingActor = nullptr;
				if (TWeakObjectPtr<AActor>* ExistingActorPtr = EditorPinned->DialogSlotActors.Find(Slot))
				{
					ExistingActor = ExistingActorPtr->Get();
				}

				if (ExistingActor && ExistingActor->IsA(LoadedActorClass))
				{
					if (bIsCamera)
					{
						EditorPinned->ApplyDefaultCameraSetting();
					}
					else
					{
						ExistingActor->SetActorRelativeLocation(Slot->SlotLocation);
						ExistingActor->SetActorRelativeRotation(Slot->SlotRotation);
					}

					EditorPinned->CreateDialogTrackFromSlot(Slot, bIsCamera);
					EditorPinned->ResolveDialogBoundObjects();
					return;
				}

				if (ExistingActor)
				{
					DestroyPreviewActor(PreviewWorld, ExistingActor);
					EditorPinned->DialogSlotActors.Remove(Slot);
				}

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParams.bNoFail = true;
				SpawnParams.ObjectFlags = RF_Transient | RF_Transactional;

				AActor* NewActor = PreviewWorld->SpawnActor<AActor>(LoadedActorClass, FTransform::Identity, SpawnParams);
				if (!NewActor)
				{
					return;
				}
				
				DisablePreviewActorPhysics(NewActor);
				TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
				NewActor->GetComponents(SkeletalMeshComponents);

				for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
				{
					if (!SkeletalMeshComponent)
					{
						continue;
					}

					/*SkeletalMeshComponent->SetComponentTickEnabled(true);
					SkeletalMeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
					SkeletalMeshComponent->SetUpdateAnimationInEditor(true);*/
				}
			
				
				// Preserve the actor's world transform when attaching to the pivot.
				NewActor->AttachToActor(EditorPinned->SequencePivot.Get(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

				if (NewActor->IsA(ALight::StaticClass()))
				{
					EditorPinned->InitializeLightActor(NewActor);
				}

				if (bIsCamera)
				{
					EditorPinned->DialogCamera = NewActor;
					if (ACineCameraActor* CineCam = Cast<ACineCameraActor>(NewActor))
					{
						if (UCineCameraComponent* CineCameraComponent = CineCam->GetCineCameraComponent())
						{
							UStaticMeshComponent* ProxyMeshComponent = NewObject<UCameraProxyMeshComponent>(CineCam, NAME_None, RF_Transactional | RF_TextExportTransient);
							ProxyMeshComponent->SetupAttachment(CineCameraComponent);
							ProxyMeshComponent->SetIsVisualizationComponent(true);
							ProxyMeshComponent->SetCanEverAffectNavigation(false);

							if (UStaticMesh* CameraMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EditorMeshes/Camera/SM_CineCam.SM_CineCam")))
							{
								ProxyMeshComponent->SetStaticMesh(CameraMesh);
							}
							ProxyMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
							ProxyMeshComponent->bHiddenInGame = false;
							ProxyMeshComponent->CastShadow = false;
							ProxyMeshComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
							ProxyMeshComponent->SetRelativeLocation(FVector(-50.0f, 0.0f, -15.0f));
							ProxyMeshComponent->RegisterComponent();
						}
					}
					EditorPinned->ApplyDefaultCameraSetting();

					if (UDialogBuilderGraph* DialogGraph = EditorPinned->GetDialogBuilderGraph())
					{
						NewActor->SetActorTransform(DialogGraph->CachedCameraTransform);
					}

				}
				else
				{
					NewActor->SetActorRelativeLocation(Slot->SlotLocation);
					NewActor->SetActorRelativeRotation(Slot->SlotRotation);
				}

				EditorPinned->DialogSlotActors.Emplace(Slot, NewActor);
				
				EditorPinned->CreateDialogTrackFromSlot(Slot, bIsCamera);
				
			};

		if (UClass* LoadedClass = ActorSoftClass.Get())
		{
			SpawnWithClass(LoadedClass);
			return;
		}

		UAssetManager::GetStreamableManager().RequestAsyncLoad(
			ClassPath,
			FStreamableDelegate::CreateLambda([EditorWeak, SpawnWithClass, ActorSoftClass]()
				{
					UClass* LoadedClass = ActorSoftClass.Get();

					// local variable holds the loaded class here
					TSubclassOf<AActor> LoadedActorClass = LoadedClass;

					SpawnWithClass(LoadedActorClass);

				}));
		return;
	}
}

void FDialogBuilderEditor::RetrieveLightSlotActor(UDialogSequenceSlot_Light* InLightSlot, AActor*& OutActor)
{
	if (bIsClosing)
	{
		return;
	}

	if(!InLightSlot)
	{
		return;
	}

	UWorld* PreviewWorld = GetPreviewWorld();
	check(PreviewWorld);
	const TSubclassOf<AActor> ActorClass = InLightSlot->LightClass;

	AActor* ExistingActor = nullptr;
	if (TWeakObjectPtr<AActor>* ExistingActorPtr = DialogSlotActors.Find(InLightSlot))
	{
		ExistingActor = ExistingActorPtr->Get();
	}

	if ((!InLightSlot || !ActorClass) && ExistingActor)
	{
		DestroyPreviewActor(PreviewWorld, ExistingActor);
		DialogSlotActors.Remove(InLightSlot);
		return;
	}

	if (ExistingActor && ExistingActor->IsA(ActorClass))
	{
		ExistingActor->SetActorRelativeLocation(InLightSlot->SlotLocation);
		ExistingActor->SetActorRelativeRotation(InLightSlot->SlotRotation);
		UpdateLightSlotProperty(InLightSlot, ExistingActor);

		return;
	}

	if (ExistingActor)
	{
		DestroyPreviewActor(PreviewWorld, ExistingActor);
		DialogSlotActors.Remove(InLightSlot);
	}


	if (ActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.bNoFail = true;
		SpawnParams.ObjectFlags = RF_Transient | RF_Transactional;
		AActor* NewActor = PreviewWorld->SpawnActor<AActor>(ActorClass, FTransform::Identity, SpawnParams);
		if (!NewActor)
		{
			return;
		}
		DisablePreviewActorPhysics(NewActor);
		NewActor->AttachToActor(SequencePivot.Get(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		NewActor->SetActorRelativeLocation(InLightSlot->SlotLocation);
		NewActor->SetActorRelativeRotation(InLightSlot->SlotRotation);
		DialogSlotActors.Emplace(InLightSlot, NewActor);
		
		UpdateLightSlotProperty(InLightSlot, NewActor);
		if (NewActor->IsA(ALight::StaticClass()))
		{
			InitializeLightActor(NewActor);
		}
	}


}

void FDialogBuilderEditor::UpdateLightSlotProperty(class UDialogSequenceSlot_Light* InLightSlot, AActor* InActor)
{
	
	if (!InLightSlot || !InActor) return;
	
	if (ULightComponent* LightComponent =  Cast<ULightComponent>(InActor->GetComponentByClass(ULightComponent::StaticClass())))
	{
		LightComponent->SetIntensity(InLightSlot->Intensity);
		LightComponent->SetLightColor(InLightSlot->LightColor);
		LightComponent->SetUseTemperature(InLightSlot->bUseTemperature);
		LightComponent->SetTemperature(InLightSlot->Temperature);
		LightComponent->bAffectsWorld = InLightSlot->bAffectsWorld;
		LightComponent->SetCastShadows(InLightSlot->CastShadows);
		LightComponent->SetIndirectLightingIntensity(InLightSlot->IndirectLightingIntensity);
		LightComponent->SetVolumetricScatteringIntensity(InLightSlot->VolumetricScatteringIntensity);
		LightComponent->CastStaticShadows = InLightSlot->CastStaticShadows;
		LightComponent->CastDynamicShadows = InLightSlot->CastDynamicShadows;
		LightComponent->SetAffectTranslucentLighting(InLightSlot->bAffectTranslucentLighting);
		LightComponent->SetCastVolumetricShadow(InLightSlot->bCastVolumetricShadow);
		LightComponent->SetCastDeepShadow(InLightSlot->bCastDeepShadow);
		LightComponent->CastRaytracedShadow = InLightSlot->CastRaytracedShadow;
		LightComponent->SetAffectReflection(InLightSlot->bAffectReflection);
		LightComponent->SetAffectGlobalIllumination(InLightSlot->bAffectGlobalIllumination);
		LightComponent->DeepShadowLayerDistribution = InLightSlot->DeepShadowLayerDistribution;
		
	}
	if (ULocalLightComponent* LocalLightComponent = Cast<ULocalLightComponent>(InActor->GetComponentByClass(ULocalLightComponent::StaticClass())))
	{
		LocalLightComponent->SetAttenuationRadius(InLightSlot->AttenuationRadius);
		LocalLightComponent->SetIntensityUnits(InLightSlot->IntensityUnits); 
	}
	
}

void FDialogBuilderEditor::InitializeLightActor(AActor* InLightActor)
{
	UBillboardComponent* BillboardComponent = NewObject<UBillboardComponent>(InLightActor, TEXT("LightBillboard"));
	BillboardComponent->SetupAttachment(InLightActor->GetRootComponent());
	BillboardComponent->SetMobility(EComponentMobility::Movable);
	BillboardComponent->SetHiddenInGame(false);
	BillboardComponent->SetVisibility(true);
	BillboardComponent->SetUsingAbsoluteScale(true);
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->bUseInEditorScaling = true;

	BillboardComponent->ScreenSize = 0.005f;
	BillboardComponent->SpriteInfo.Category = TEXT("Dialog");
	BillboardComponent->SetEditorScale(.3f);
	BillboardComponent->SpriteInfo.DisplayName = LOCTEXT("DialogSequencePivotSprite", "Light Prop");
	BillboardComponent->RegisterComponent();
	InLightActor->SetActorHiddenInGame(false);
	if (InLightActor->IsA(APointLight::StaticClass()))
	{
		if (UTexture2D* Sprite = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorResources/LightIcons/S_LightPoint")))
		{
			BillboardComponent->SetSprite(Sprite);
		}
	}
	else if (InLightActor->IsA(ADirectionalLight::StaticClass()))
	{
		if (UTexture2D* Sprite = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorResources/LightIcons/S_LightDirectional")))
		{
			BillboardComponent->SetSprite(Sprite);
		}
	}
	else if (InLightActor->IsA(ASpotLight::StaticClass()))
	{

		UArrowComponent* ArrowComponent = NewObject<UArrowComponent>(InLightActor, TEXT("CameraArrow"));
		ArrowComponent->SetupAttachment(InLightActor->GetRootComponent());
		ArrowComponent->bHiddenInGame = false;
		ArrowComponent->ArrowColor = FColor::Cyan;
		ArrowComponent->ArrowSize = .5f;
		BillboardComponent->SetVisibility(true);
		ArrowComponent->RegisterComponent();
		if (UTexture2D* Sprite = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EditorResources/LightIcons/S_LightSpot")))
		{
			BillboardComponent->SetSprite(Sprite);
		}
	}


	
}

void FDialogBuilderEditor::DestroyDialogStage()
{
	UDialogBuilderGraph* DialogGraph = GetDialogBuilderGraph();
	UWorld* PreviewWorld = GetPreviewWorld();

	if (DialogViewportWidget.IsValid())
	{
		if (FDialogBuilderViewportClient* ViewportClient = DialogViewportWidget->GetDialogViewportClientPtr())
		{
			ViewportClient->SelectActor(nullptr);
		}
	}

	ResetEditorSelectionWithoutResolvingElements();

	if (DialogGraph)
	{
		if (AActor* CameraActor = DialogCamera.Get())
		{
			DialogGraph->CachedCameraTransform = CameraActor->GetActorTransform();
		}
	}

	if (PreviewWorld)
	{
		for (auto& Pair : DialogSlotActors)
		{
			if (AActor* Actor = Pair.Value.Get())
			{
				DestroyPreviewActor(PreviewWorld, Actor);

				if (EditingDialogSequence && Pair.Key)
				{
					EditingDialogSequence->UnbindPossessableObjects(Pair.Key->ID);
				}
			}

		}

		if (AActor* PivotActor = SequencePivot.Get())
		{
			DestroyPreviewActor(PreviewWorld, PivotActor);
		}
	}

	DialogCamera.Reset();
	SequencePivot.Reset();
	DialogSlotActors.Empty();
}


TArray<AActor*> FDialogBuilderEditor::GetDialogSlotActors() const
{
	TArray<AActor*> Result;
	Result.Reserve(DialogSlotActors.Num());

	for (const TPair<UDialogSequenceSlot*, TWeakObjectPtr<AActor>>& Pair : DialogSlotActors)
	{
		if (AActor* Actor = Pair.Value.Get())
		{
			Result.Add(Actor);
		}
	}

	return Result;
}

AActor* FDialogBuilderEditor::GetDialogDefinitionActor(UDialogDefinition* InDialogDefinition)
{
	AActor* OutActor = nullptr;
	for (const TPair<UDialogSequenceSlot*, TWeakObjectPtr<AActor>>& Pair : DialogSlotActors)
	{
		if (UDialogSequenceSlot* Slot = Pair.Key)
		{
			if (Slot->DialogDefinition == InDialogDefinition)
			{
				OutActor = Pair.Value.Get();
				break;
			}
		}
	}
	return OutActor;
}

void FDialogBuilderEditor::SelectDialogDefinitionActor(UDialogDefinition* InDialogDefinition)
{
	AActor* ActorToSelect = GetDialogDefinitionActor(InDialogDefinition);


	SelectActor(ActorToSelect);
}

void FDialogBuilderEditor::SelectDialogDefinition(UDialogDefinition* InDialogDefinition)
{
	if (!DialogDefinitionsWidget.IsValid())
	{
		return;
	}


	int32 SectionId = INDEX_NONE;
	if (Cast<UDialogParticipant>(InDialogDefinition))
	{
		SectionId = DialogSectionID::PARTICIPANTS;
	}
	else if (Cast<UDialogProp>(InDialogDefinition))
	{
		SectionId = DialogSectionID::PROPS;
	}

	DialogDefinitionsWidget->SelectItemByName(
		InDialogDefinition ? InDialogDefinition->GetFName() : NAME_None,
		ESelectInfo::Direct,
		SectionId,
		false);

	SetDetailsObject(InDialogDefinition);
	SelectDialogDefinitionActor(InDialogDefinition);
}

void FDialogBuilderEditor::SelectDialogSlotActor(int32 index)
{
	if (!EditingDialogGraph || !CurrentSequenceNode.IsValid()) return;

	UDialogStage* DialogStage = CurrentSequenceNode.Get()->DialogStage;
	if (!DialogStage)
	{
		return;
	}

	if (!DialogStage->Slots.IsValidIndex(index))
	{
		return;
	}
	UDialogSequenceSlot* Slot = DialogStage->Slots[index];


	AActor* ActorToSelect = DialogSlotActors.FindRef(Slot).Get();


	SelectActor(ActorToSelect);
}

void FDialogBuilderEditor::SelectLightSlotActor(int32 index)
{
	if (!EditingDialogGraph || !CurrentSequenceNode.IsValid()) return;

	UDialogStage* DialogStage = CurrentSequenceNode.Get()->DialogStage;
	if (!DialogStage)
	{
		return;
	}

	if (!DialogStage->LightSlots.IsValidIndex(index))
	{
		return;
	}
	UDialogSequenceSlot* Slot = DialogStage->LightSlots[index];


	AActor* ActorToSelect = DialogSlotActors.FindRef(Slot).Get();

	SelectActor(ActorToSelect);
}

void FDialogBuilderEditor::SelectActor(AActor* ActorToSelect)
{
	if (ActorToSelect && IsValid(ActorToSelect))
	{
		USceneComponent* RootComp = ActorToSelect->GetRootComponent();
		if (!IsValid(RootComp))
		{
			// No valid root component — cannot select
			ActorToSelect = nullptr;
		}
	}
	else
	{
		ActorToSelect = nullptr;
	}
	FDialogBuilderViewportClient* ViewportClient = DialogViewportWidget ? DialogViewportWidget->GetDialogViewportClientPtr() : nullptr;
	if (!ViewportClient)
	{
		return;
	}

	ViewportClient->SelectActor(ActorToSelect);
}

UDialogDefinition* FDialogBuilderEditor::FindDialogDefinitionByActor(const AActor* InActor) const
{
	if (!InActor || !CurrentSequenceNode.IsValid())
	{
		return nullptr;
	}

	UDialogStage* DialogStage = CurrentSequenceNode.Get()->DialogStage;
	if (!DialogStage)
	{
		return nullptr;
	}


	for (auto& Slot : DialogStage->Slots)
	{
		if (Slot)
		{
			AActor* Actor = DialogSlotActors.FindRef(Slot).Get();
			if(Actor == InActor)
			{
				return Slot->DialogDefinition;
			}
		}
	}

	return nullptr;
}

UWorld* FDialogBuilderEditor::GetPreviewWorld()
{
	if (ViewportWorldMode == EDialogViewportWorldMode::CurrentLevel)
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	return PreviewScene.GetWorld();
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


	const TWeakPtr<FDialogBuilderEditor> EditorWeak = SharedThis(this);

	ToolkitCommands->MapAction(
		FDialogBuilder_EditorCommands::Get().SetViewportCameraPerspective,
		FExecuteAction::CreateLambda([EditorWeak]()
			{
				if (const TSharedPtr<FDialogBuilderEditor> EditorPinned = EditorWeak.Pin())
				{
					EditorPinned->SetViewportCameraMode(EDialogViewportCameraMode::Perspective);
					
				}
			}),
		FCanExecuteAction()
	);


	ToolkitCommands->MapAction(
		FDialogBuilder_EditorCommands::Get().SetViewportCameraDialogCamera,
		FExecuteAction::CreateLambda([EditorWeak]()
			{
				if (const TSharedPtr<FDialogBuilderEditor> EditorPinned = EditorWeak.Pin())
				{
					EditorPinned->SetViewportCameraMode(EDialogViewportCameraMode::DialogCameraLock);
					
				}
			}),
		FCanExecuteAction()
	);

	ToolkitCommands->MapAction(
		FDialogBuilder_EditorCommands::Get().SetViewportCameraSequencerCuts,
		FExecuteAction::CreateLambda([EditorWeak]()
			{
				if (const TSharedPtr<FDialogBuilderEditor> EditorPinned = EditorWeak.Pin())
				{
					EditorPinned->SetViewportCameraMode(EDialogViewportCameraMode::SequencerCameraCuts);
					
				}
			}),
		FCanExecuteAction()
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

const FSlateBrush* FDialogBuilderEditor::GetDefaultTabIcon() const
{
	return FDialogBuilder_EditorStyle::Get().GetBrush("ClassIcon.DialogNode");
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
	return FText::Format(LOCTEXT("DialogGraphEditorToolkitName", "{DialogGraphName}"), Args);
}

FText FDialogBuilderEditor::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolTipTextForObject(EditingDialogGraph);
}

FLinearColor FDialogBuilderEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
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

void FDialogBuilderEditor::RefreshDialogDefinitions()
{
}

void FDialogBuilderEditor::RefreshInspector()
{
}

void FDialogBuilderEditor::AddToSelection(UEdGraphNode* InNode)
{
}

void FDialogBuilderEditor::OnOpenDialogSequenceNode(UDialogBuilderNode_DialogSequence* InSequenceNode)
{
	// invoke sequencer tab
	TSharedPtr<FTabManager> HostTabManager;
	HostTabManager = GetToolkitHost()->GetTabManager();

	if (!InSequenceNode)
	{
		return;
	}

	InSequenceNode->EnsureSequenceCreated();

	const FTabId SequencerTabId(FDialogBuilderEditorTabs::DialogSequencerTabID);

	if (HostTabManager.IsValid())
	{
		if (CurrentSequenceNode == InSequenceNode)
		{
			HostTabManager->TryInvokeTab(SequencerTabId);
			return;
		}

		DestroyDialogStage();
		CurrentSequenceNode = InSequenceNode;
		GetDialogBuilderGraph()->CurrentEditingSequenceNode = CurrentSequenceNode.Get();
		DialogStageWidget->SetObject(InSequenceNode->DialogStage);
		DialogStageTemplate = InSequenceNode->DialogStageToUse;
		OpenDialogSequence(InSequenceNode->DialogSequence);
		UpdateDialogStage();

		TSharedPtr<SDockTab> ExistingTab = HostTabManager->FindExistingLiveTab(SequencerTabId);
		if (ExistingTab.IsValid())
		{
			ExistingTab->SetContent(Sequencer.IsValid() ? Sequencer->GetSequencerWidget() : SNullWidget::NullWidget);
			ExistingTab->Invalidate(EInvalidateWidget::Layout);
		}

		HostTabManager->TryInvokeTab(SequencerTabId);

		FDialogBuilderViewportClient* ViewportClient = DialogViewportWidget->GetDialogViewportClientPtr();
		if (!ViewportClient)
		{
			return;
		}
		SetViewportCameraMode(EDialogViewportCameraMode::Perspective);

		ViewportClient->ResetCamera();
	}
}

//Sequencer
void FDialogBuilderEditor::OpenDialogSequence(UDialogSequence* InDialogSequence)
{
	if (!InDialogSequence)
	{
		return;
	}

	EditingDialogSequence = InDialogSequence;
	if (DialogViewportWidget.IsValid())
	{
		DialogViewportWidget->RemoveDialogOverlayWidget();
	}


	if (Sequencer.IsValid())
	{
		Sequencer->ResetToNewRootSequence(*InDialogSequence);
		
		UpdateTrackModelRule();
		UpdateSectionRule();
		Sequencer->GetSequencerWidget()->SetEnabled(true);
		Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::RefreshAllImmediately);
		Sequencer->ForceEvaluate();
		RefreshSequencerCameraLock();
	}
	else
	{
		EnsureSequencerCreated();
		if (Sequencer.IsValid())
		{
			Sequencer->GetSequencerWidget()->SetEnabled(true);
			Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::RefreshAllImmediately);
			Sequencer->ForceEvaluate();
			RefreshSequencerCameraLock();
		}
	}

	if (Sequencer.IsValid())
	{
		if (TSharedPtr<IToolkitHost> LocalToolkitHost = GetToolkitHost())
		{
			if (TSharedPtr<FTabManager> HostTabManager = LocalToolkitHost->GetTabManager())
			{
				const FTabId SequencerTabId(FDialogBuilderEditorTabs::DialogSequencerTabID);
				if (TSharedPtr<SDockTab> ExistingTab = HostTabManager->FindExistingLiveTab(SequencerTabId))
				{
					ExistingTab->SetContent(Sequencer->GetSequencerWidget());
				}
			}
		}
	}

	ApplyViewportCameraMode();


}


void FDialogBuilderEditor::OnSequencerReceivedFocus()
{
	if (!Sequencer.IsValid())
	{
		return;
	}

	RefreshSequencerCameraLock();
}

void FDialogBuilderEditor::OnInitToolMenuContext(FToolMenuContext& MenuContext)
{
	UDialogSequenceEditorMenuContext* DialogSequenceEditorMenuContext = NewObject<UDialogSequenceEditorMenuContext>();
	DialogSequenceEditorMenuContext->DialogEditor = SharedThis(this);
	MenuContext.AddObject(DialogSequenceEditorMenuContext);
}

void FDialogBuilderEditor::EnsureSequencerCreated()
{
	if (Sequencer.IsValid())
	{
		return;
	}

	if (!EditingDialogSequence)
	{
		EditingDialogSequence = UDialogSequence::GetNullDialogSequence();
	}

	PlaybackContext = MakeShared<FDialogSequencePlaybackContext>(EditingDialogSequence);
	PlaybackContext->SetDialogViewport(DialogViewportWidget);


	TSharedRef<FLevelSequenceEditorSpawnRegister> SpawnRegister = MakeShareable(new FLevelSequenceEditorSpawnRegister);

	ISequencerModule& SequencerModule = FModuleManager::LoadModuleChecked<ISequencerModule>("Sequencer");

	FSequencerViewParams ViewParams(TEXT("DialogSequenceSetting"));
	{
		ViewParams.UniqueName = "DialogSequenceEditor";
		ViewParams.ScrubberStyle = ESequencerScrubberStyle::FrameBlock;
		ViewParams.OnReceivedFocus.BindRaw(this, &FDialogBuilderEditor::OnSequencerReceivedFocus);
		ViewParams.OnInitToolMenuContext.BindRaw(this, &FDialogBuilderEditor::OnInitToolMenuContext);
		ViewParams.ToolbarExtender = MakeShared<FExtender>();
		ViewParams.AddMenuExtender = MakeShared<FExtender>();

		ViewParams.ToolbarExtender->AddToolBarExtension(
			"CurveEditor",
			EExtensionHook::After,
			nullptr,
			FToolBarExtensionDelegate::CreateSP(this, &FDialogBuilderEditor::ExtendSequencerToolbar));
	}

	FSequencerInitParams InitParams;
	{
		InitParams.RootSequence = EditingDialogSequence;
		InitParams.bEditWithinLevelEditor = false;
		InitParams.ToolkitHost = GetToolkitHost();
		InitParams.SpawnRegister = SpawnRegister;

		InitParams.EventContexts.Bind(PlaybackContext.ToSharedRef(), &FDialogSequencePlaybackContext::GetEventContexts);
		InitParams.PlaybackContext.Bind(PlaybackContext.ToSharedRef(), &FDialogSequencePlaybackContext::GetPlaybackContextAsObject);
		InitParams.PlaybackClient.Bind(PlaybackContext.ToSharedRef(), &FDialogSequencePlaybackContext::GetPlaybackClientAsInterface);

		InitParams.ViewParams = ViewParams;

		InitParams.HostCapabilities.bSupportsCurveEditor = true;
		InitParams.HostCapabilities.bSupportsAddFromContentBrowser = true;
		InitParams.HostCapabilities.bSupportsSidebar = true;
		InitParams.HostCapabilities.bSupportsViewportSelectability = true;

	}


	Sequencer = SequencerModule.CreateSequencer(InitParams);
	SpawnRegister->SetSequencer(Sequencer);

	if (Sequencer.IsValid())
	{
		SequencerGlobalTimeChangedHandle = Sequencer->OnGlobalTimeChanged().AddRaw(this, &FDialogBuilderEditor::OnSequencerGlobalTimeChanged);
		SequencerMovieSceneDataChangedHandle = Sequencer->OnMovieSceneDataChanged().AddRaw(this, &FDialogBuilderEditor::OnSequencerMovieSceneDataChanged);

		Sequencer->ForceEvaluate();

		const TSharedRef<SWidget> SequencerWidget = Sequencer->GetSequencerWidget();

		//trigger sequencer property widget docked tab
		/*FSlateApplication::Get().SetKeyboardFocus(SequencerWidget, EFocusCause::SetDirectly);

		const FModifierKeysState AltModifiers(
			false, false, false, false,
			true, false,
			false, false,
			false);

		const FKeyEvent KeyDownEvent(EKeys::D, AltModifiers, 0, false, 0, 0);
		FSlateApplication::Get().ProcessKeyDownEvent(KeyDownEvent);
		FSlateApplication::Get().ProcessKeyUpEvent(KeyDownEvent);*/

		Sequencer->GetSelectionChangedObjectGuids().AddRaw(
			this,
			&FDialogBuilderEditor::OnSequencerSelectionChangedObjectGuids);
		
		Sequencer->GetSequencerWidget()->SetEnabled(false);
		


		RefreshSequencerCameraLock();
	}

}

void FDialogBuilderEditor::CloseSequencerForUndoRedo()
{
	if (!Sequencer.IsValid())
	{
		return;
	}

	UnbindSequencerDelegates();

	if (TSharedPtr<IToolkitHost> LocalToolkitHost = GetToolkitHost())
	{
		if (TSharedPtr<FTabManager> HostTabManager = LocalToolkitHost->GetTabManager())
		{
			const FTabId SequencerTabId(FDialogBuilderEditorTabs::DialogSequencerTabID);
			if (TSharedPtr<SDockTab> ExistingTab = HostTabManager->FindExistingLiveTab(SequencerTabId))
			{
				ExistingTab->SetContent(SNullWidget::NullWidget);
			}
		}
	}

	// Clear typed-element selections before closing to avoid pivot updates on invalid preview actors.
	ResetEditorSelectionWithoutResolvingElements();
	Sequencer->Close();
	Sequencer.Reset();
	PlaybackContext.Reset();
}

void FDialogBuilderEditor::ExtendSequencerToolbar(FToolBarBuilder& InToolbarBuilder)
{
	InToolbarBuilder.BeginSection("DialogStagePicker");
	{
		InToolbarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateLambda([this]() -> TSharedRef<SWidget>
				{
					FMenuBuilder MenuBuilder(true, nullptr);

					MenuBuilder.BeginSection("DialogStageListSection", LOCTEXT("ToolbarDialogStageList", "Dialog Stage List"));

					UDialogBuilderGraph* DialogGraph = GetDialogBuilderGraph();
					if (DialogGraph && CurrentSequenceNode.IsValid())
					{
						for (UDialogStage* DialogStage : DialogGraph->DialogStages)
						{
							if (!DialogStage)
							{
								continue;
							}

							const FText DialogStageName = DialogStage->Name.IsEmpty()
								? LOCTEXT("ToolbarDialogStageNoneLabel", "None")
								: DialogStage->Name;

							const FUIAction Action(
								FExecuteAction::CreateLambda([this, DialogStage]()
									{
										if (!CurrentSequenceNode.IsValid() || !DialogStage)
										{
											return;
										}

										const FScopedTransaction Transaction(LOCTEXT("ToolbarSetDialogStageTransaction", "Set Dialog Stage"));
										if (UDialogBuilderNode_DialogSequence* SequenceNode = CurrentSequenceNode.Get())
										{
											SequenceNode->Modify();
											SequenceNode->UseDialogStage(DialogStage);
										}

										DialogStageTemplate = DialogStage;
										UpdateDialogStage();

										if (DialogStageManagerWidget.IsValid())
										{
											DialogStageManagerWidget->Refresh();
										}
									}),
								FCanExecuteAction(),
								FIsActionChecked::CreateLambda([this, DialogStage]() -> bool
									{
										return DialogStageTemplate.Get() == DialogStage;
									})
							);

							MenuBuilder.AddMenuEntry(
								DialogStageName,
								LOCTEXT("ToolbarDialogStagePickerTooltip", "Pick which dialog stage to use for this sequence"),
								FSlateIcon(),
								Action,
								NAME_None,
								EUserInterfaceActionType::Check);
						}
					}

					MenuBuilder.EndSection();
					return MenuBuilder.MakeWidget();
				}),
			LOCTEXT("ToolbarDialogStagePicker_Label", "Dialog Stage"),
			LOCTEXT("ToolbarDialogStagePicker_Tooltip", "Pick which dialog stage to use for this sequence"),
			FSlateIcon(),
			false,
			FName("Dialog Stage"),
			EVisibility::All,
			TAttribute<FText>::CreateLambda([this]()
				{
					const UDialogStage* CurrentStage = DialogStageTemplate.Get();
					if (!CurrentStage)
					{
						return LOCTEXT("ToolbarDialogStagePickerLabel_None", "Stage: None");
					}

					const FText CurrentStageName = CurrentStage->Name.IsEmpty()
						? LOCTEXT("ToolbarDialogStagePickerLabel_Empty", "Stage: None")
						: CurrentStage->Name;

					return FText::Format(LOCTEXT("ToolbarDialogStagePickerLabel", "{0}"), CurrentStageName);
				})
		);
	}
	InToolbarBuilder.EndSection();

	InToolbarBuilder.BeginSection("DialogCameraGenerate");
	{
		InToolbarBuilder.AddToolBarButton(
			FUIAction(
				FExecuteAction::CreateSP(this, &FDialogBuilderEditor::OpenGenerateSequenceCameraDialog),
				FCanExecuteAction::CreateSP(this, &FDialogBuilderEditor::CanGenerateSequenceCameraFromDialogSections)),
			NAME_None,
			LOCTEXT("GenerateDialogSequenceCamera_Label", ""),
			LOCTEXT("GenerateDialogSequenceCamera_Tooltip", "Generate camera keys from each dialog section"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.LockCamera"));
	}
	InToolbarBuilder.EndSection();
}

bool FDialogBuilderEditor::CanGenerateSequenceCameraFromDialogSections() const
{
	return Sequencer.IsValid()
		&& EditingDialogSequence != nullptr
		&& CurrentSequenceNode.IsValid()
		&& DialogCameraPresetsWidget.IsValid();
}

void FDialogBuilderEditor::OpenGenerateSequenceCameraDialog()
{
	UDialogGenerateCameraSettings* GenerateSettings = NewObject<UDialogGenerateCameraSettings>(GetTransientPackage(), NAME_None, RF_Transient);
	GenerateSettings->Initialize();
	GenerateSettings->AddToRoot();

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs SettingsDetailsViewArgs;
	SettingsDetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	SettingsDetailsViewArgs.bHideSelectionTip = true;
	SettingsDetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
	SettingsDetailsViewArgs.bAllowSearch = false;

	const TSharedRef<IDetailsView> SettingsDetailsView = PropertyModule.CreateDetailView(SettingsDetailsViewArgs);
	SettingsDetailsView->SetObject(GenerateSettings);

	TSharedPtr<SWindow> DialogWindow = SNew(SWindow)
		.Title(LOCTEXT("GenerateDialogSequenceCamera_Title", "Generate Sequence Camera"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(500.0f, 320.0f))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	TWeakPtr<SWindow> DialogWindowWeak = DialogWindow;

	DialogWindow->SetContent(
		SNew(SBorder)
		.Padding(12.0f)
		[
			SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SettingsDetailsView
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				[
					SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.f, 0.f, 8.f, 0.f)
						[
							SNew(SButton)
								.Text(LOCTEXT("GenerateDialogSequenceCamera_Cancel", "Cancel"))
								.OnClicked_Lambda([DialogWindowWeak]()
									{
										if (TSharedPtr<SWindow> PinnedWindow = DialogWindowWeak.Pin())
										{
											PinnedWindow->RequestDestroyWindow();
										}
										return FReply::Handled();
									})
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
								.Text(LOCTEXT("GenerateDialogSequenceCamera_Generate", "Generate"))
								.OnClicked_Lambda([this, DialogWindowWeak, GenerateSettings]()
									{
										const FScopedTransaction Transaction(LOCTEXT("GenerateSequenceCameraTransaction", "Generate Sequence Camera"));
										GenerateSequenceCameraFromDialogSections(GenerateSettings);

										if (TSharedPtr<SWindow> PinnedWindow = DialogWindowWeak.Pin())
										{
											PinnedWindow->RequestDestroyWindow();
										}
										return FReply::Handled();
									})
						]
				]
		]
	);

	DialogWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([GenerateSettings](const TSharedRef<SWindow>&)
		{
			if (IsValid(GenerateSettings) && GenerateSettings->IsRooted())
			{
				GenerateSettings->RemoveFromRoot();
			}
		}));

	FSlateApplication::Get().AddModalWindow(DialogWindow.ToSharedRef(), FSlateApplication::Get().GetActiveTopLevelWindow(), false);
}


void FDialogBuilderEditor::UnbindSequencerDelegates()
{
	if (!Sequencer.IsValid())
	{
		return;
	}

	if (SequencerGlobalTimeChangedHandle.IsValid())
	{
		Sequencer->OnGlobalTimeChanged().Remove(SequencerGlobalTimeChangedHandle);
		SequencerGlobalTimeChangedHandle.Reset();
	}

	if (SequencerMovieSceneDataChangedHandle.IsValid())
	{
		Sequencer->OnMovieSceneDataChanged().Remove(SequencerMovieSceneDataChangedHandle);
		SequencerMovieSceneDataChangedHandle.Reset();
	}

	Sequencer->GetSelectionChangedObjectGuids().RemoveAll(this);

	LastSequencerCameraCutActor.Reset();
}

void FDialogBuilderEditor::OnSequencerGlobalTimeChanged()
{
	RefreshSequencerCameraLock();
}

void FDialogBuilderEditor::OnSequencerMovieSceneDataChanged(EMovieSceneDataChangeType ChangeType)
{
	RefreshSequencerCameraLock();
}

void FDialogBuilderEditor::EnsureDialogCameraCutSection()
{
	if (!Sequencer.IsValid() || !EditingDialogSequence)
	{
		return;
	}

	AActor* CameraActor = DialogCamera.Get();
	if (!IsValid(CameraActor))
	{
		return;
	}

	UMovieScene* MovieScene = EditingDialogSequence->GetMovieScene();
	if (!MovieScene)
	{
		return;
	}

	UDialogSequenceSlot* CameraSlot = GetDialogSlotKey(CameraActor);
	if(!CameraSlot)
	{
		return;
	}

	FGuid CameraBindingId = CameraSlot->ID;
	if (!CameraBindingId.IsValid())
	{
		return;
	}

	const FFrameNumber CurrentFrame = Sequencer->GetLocalTime().Time.FloorToFrame();
	MovieSceneToolHelpers::CreateCameraCutSectionForCamera(MovieScene, CameraBindingId, CurrentFrame);


	
}

static AActor* ResolveActiveCameraCutActor(ISequencer& InSequencer)
{
	UMovieSceneSequence* FocusedSequence = InSequencer.GetFocusedMovieSceneSequence();
	UMovieScene* MovieScene = FocusedSequence ? FocusedSequence->GetMovieScene() : nullptr;
	UMovieSceneCameraCutTrack* CameraCutTrack = MovieScene ? Cast<UMovieSceneCameraCutTrack>(MovieScene->GetCameraCutTrack()) : nullptr;
	if (!CameraCutTrack)
	{
		return nullptr;
	}

	const FFrameNumber CurrentFrame = InSequencer.GetLocalTime().Time.FloorToFrame();

	for (UMovieSceneSection* Section : CameraCutTrack->GetAllSections())
	{
		UMovieSceneCameraCutSection* CameraCutSection = Cast<UMovieSceneCameraCutSection>(Section);
		if (!CameraCutSection || !CameraCutSection->GetRange().Contains(CurrentFrame))
		{
			continue;
		}

		const FMovieSceneObjectBindingID CameraBindingID = CameraCutSection->GetCameraBindingID();
		for (TWeakObjectPtr<> WeakObject : CameraBindingID.ResolveBoundObjects(InSequencer.GetFocusedTemplateID(), InSequencer))
		{
			if (AActor* CameraActor = Cast<AActor>(WeakObject.Get()))
			{
				return CameraActor;
			}
		}

		break;
	}

	return nullptr;
}

static AActor* ResolveBoundCineCameraActor(ISequencer& InSequencer)
{
	UMovieSceneSequence* FocusedSequence = InSequencer.GetFocusedMovieSceneSequence();
	const UMovieScene* MovieScene = FocusedSequence ? FocusedSequence->GetMovieScene() : nullptr;
	if (!MovieScene)
	{
		return nullptr;
	}

	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		bool bHasTransformTrack = false;
		for (UMovieSceneTrack* Track : Binding.GetTracks())
		{
			InSequencer.GetTrackEditor(Track);
			bHasTransformTrack = true;
			break;
			
		}

		if (!bHasTransformTrack)
		{
			continue;
		}

		for (TWeakObjectPtr<> WeakObject : InSequencer.FindBoundObjects(Binding.GetObjectGuid(), InSequencer.GetFocusedTemplateID()))
		{
			if (ACineCameraActor* CineCamera = Cast<ACineCameraActor>(WeakObject.Get()))
			{
				return CineCamera;
			}
		}
	}

	return nullptr;
}

void FDialogBuilderEditor::RefreshSequencerCameraLock()
{
	if (!Sequencer.IsValid() || !DialogViewportWidget.IsValid())
	{
		return;
	}

	FDialogBuilderViewportClient* ViewportClient = DialogViewportWidget->GetDialogViewportClientPtr();
	if (!ViewportClient)
	{
		return;
	}

	if (!Sequencer->IsPerspectiveViewportCameraCutEnabled())
	{
		return;
	}

	AActor* ActiveCameraActor = ResolveActiveCameraCutActor(*Sequencer);
	if (LastSequencerCameraCutActor.Get() != ActiveCameraActor)
	{
		ViewportClient->SetCinematicActorLock(ActiveCameraActor);
		ViewportClient->UpdateViewForLockedActor();
		ViewportClient->Invalidate();
		LastSequencerCameraCutActor = ActiveCameraActor;
	}


}



void NewCameraAdded(TSharedRef<ISequencer> Sequencer, ACameraActor* NewCamera, FGuid CameraGuid)
{
	if (Sequencer->OnCameraAddedToSequencer().IsBound() && !Sequencer->OnCameraAddedToSequencer().Execute(NewCamera, CameraGuid))
	{
		return;
	}

	MovieSceneToolHelpers::LockCameraActorToViewport(Sequencer, NewCamera);

	UMovieSceneSequence* Sequence = Sequencer->GetFocusedMovieSceneSequence();
	if (Sequence && Sequence->IsTrackSupported(UMovieSceneCameraCutTrack::StaticClass()) == ETrackSupport::Supported)
	{
		MovieSceneToolHelpers::CreateCameraCutSectionForCamera(Sequence->GetMovieScene(), CameraGuid, Sequencer->GetLocalTime().Time.FloorToFrame());
	}
}


UDialogSequenceSlot* FDialogBuilderEditor::GetDialogSlotKey(AActor* InActor)
{
	for (const TPair<UDialogSequenceSlot*, TWeakObjectPtr<AActor>>& Pair : DialogSlotActors)
	{
		if (Pair.Value.Get() == InActor)
		{
			return Pair.Key;
		}
	}
	return nullptr;
}

UDialogSequenceSlot* FDialogBuilderEditor::GetTemplateSlot(AActor* InActor)
{
	if (!InActor || !EditingDialogGraph || !CurrentSequenceNode.IsValid())
	{
		return nullptr;
	}

	UDialogStage* DialogStage = CurrentSequenceNode.Get()->DialogStage;
	if (!DialogStage || !DialogStageTemplate.IsValid())
	{
		return nullptr;
	}

	UDialogSequenceSlot* SlotToFind = GetDialogSlotKey(InActor);

	if(DialogStage->Slots.Contains(SlotToFind))
	{
		int index = 0;
		for (int i = 0; i < DialogStage->Slots.Num(); i++)
		{
			if (DialogStage->Slots.IsValidIndex(i) && DialogStage->Slots[i] == SlotToFind)
			{
				index = i;
				break;
			}
		}

		if (DialogStageTemplate.Get()->Slots.IsValidIndex(index))
		{
			return DialogStageTemplate->Slots[index];
		}
	}
	else
	{
		//if not found in slot, try to find in light slot
		int index = 0;
		for (int i = 0; i < DialogStage->LightSlots.Num(); i++)
		{
			if (DialogStage->LightSlots.IsValidIndex(i) && DialogStage->LightSlots[i] == SlotToFind)
			{
				index = i;
				break;
			}
		}

		if (DialogStageTemplate.Get()->LightSlots.IsValidIndex(index))
		{
			return DialogStageTemplate->LightSlots[index];
		}
	}


	



	return nullptr;
}

void FDialogBuilderEditor::UpdateTrackModelRule()
{
	if (UMovieScene* MovieScene = EditingDialogSequence ? EditingDialogSequence->GetMovieScene() : nullptr)
	{
		constexpr int32 DialogTrackSortingOrder = 0;

		AActor* CameraActor = DialogCamera.Get();
		UDialogSequenceSlot* CameraSlot = GetDialogSlotKey(CameraActor);



		for (UMovieSceneTrack* Track : MovieScene->GetTracks())
		{
			if (UMovieSceneDialogTrack* DialogTrack = Cast<UMovieSceneDialogTrack>(Track))
			{
				DialogTrack->SetSortingOrder(DialogTrackSortingOrder);
			}
		}


		if (TSharedPtr<UE::Sequencer::FSequencerEditorViewModel> EditorViewModel = Sequencer->GetViewModel())
		{
			TSharedPtr<UE::Sequencer::FSequenceModel> RootSequenceModel = EditorViewModel->GetRootSequenceModel();
			if (RootSequenceModel.IsValid())
			{
				for (TSharedPtr<UE::Sequencer::FTrackModel> TrackModel :
					RootSequenceModel->GetDescendantsOfType<UE::Sequencer::FTrackModel>(true))
				{
					if (!TrackModel.IsValid()) continue;
					bool bLocked = TrackModel->GetLockState() == UE::Sequencer::ELockableLockState::Locked;
					bool bPinned = TrackModel->IsPinned();

					//ensure lock and pin camera cut track
					if (TrackModel->GetTrack() == EditingDialogSequence->GetMovieScene()->GetCameraCutTrack())
					{
						TrackModel->SetPinned(true);
						continue;
					}

					//ensure pin dialog track
					if (TrackModel->GetTrack()->IsA(UMovieSceneDialogTrack::StaticClass()) &&
						(!bPinned))
					{
						TrackModel->SetPinned(true);
						continue;
					}

				}

			}
			
		}
	}
}

void FDialogBuilderEditor::UpdateSectionRule()
{
	//update the section rule to make sure the section in specific track to follonw a certain rule/setting
	if (!Sequencer.IsValid() || !EditingDialogSequence)
	{
		return;
	}

	UMovieScene* MovieScene = EditingDialogSequence->GetMovieScene();
	if (!MovieScene)
	{
		return;
	}

	UDialogSequenceSlot* CameraSlot = GetDialogSlotKey(DialogCamera.Get());

	if (!CameraSlot || !CameraSlot->ID.IsValid())
	{
		return;
	}

	//resolve camera cut section binding
	if (UMovieSceneTrack* CameraCutTrack = MovieScene->GetCameraCutTrack())
	{
		for (UMovieSceneSection* Section : CameraCutTrack->GetAllSections())
		{
			UMovieSceneCameraCutSection* CameraCutSection = Cast<UMovieSceneCameraCutSection>(Section);
			if (!CameraCutSection->GetCameraBindingID().IsValid())
			{
				CameraCutSection->SetCameraBindingID(FMovieSceneObjectBindingID(CameraSlot->ID));
			}
		}
	}

	const FMovieSceneBinding* CameraSlotBinding = MovieScene->FindBinding(CameraSlot->ID);
	if (!CameraSlotBinding)
	{
		return;
	}

	for (UMovieSceneTrack* Track : CameraSlotBinding->GetTracks())
	{
		UMovieScene3DTransformTrack* TransformTrack = Cast<UMovieScene3DTransformTrack>(Track);
		if (!TransformTrack)
		{
			continue;
		}

		for (UMovieSceneSection* Section : TransformTrack->GetAllSections())
		{
			if (!Section)
			{
				continue;
			}

			if (Section->GetCompletionMode() != EMovieSceneCompletionMode::KeepState)
			{
				Section->SetCompletionMode(EMovieSceneCompletionMode::KeepState);
			}
		}
	}
}

void FDialogBuilderEditor::OnSequencerSelectionChangedObjectGuids(TArray<FGuid> Guids)
{
	if (Guids.IsValidIndex(0))
	{
		TArrayView<TWeakObjectPtr<>> BoundObjects = Sequencer->FindBoundObjects(Guids[0], Sequencer->GetRootTemplateID());
		if (!BoundObjects.IsValidIndex(0)) return;
		if (AActor* ActorToSelect = Cast<AActor>(BoundObjects[0]))
		{

			SelectActor(ActorToSelect);

			if (ActorToSelect == DialogCamera)
			{
				SetDetailsObject(ActorToSelect);
				return;
			}
			UDialogSequenceSlot* Slot = GetTemplateSlot(ActorToSelect);

			if (Slot)
			{
				SetDetailsObject(Slot);
			}
			
		}
	}
}

void FDialogBuilderEditor::ApplyDefaultCameraSetting()
{
	AActor* CameraActor = DialogCamera.Get();
	if (!IsValid(CameraActor))
	{
		return;
	}

	UDialogBuilderGraph* DialogGraph = GetDialogBuilderGraph();
	if (!DialogGraph)
	{
		return;
	}

	if (ACineCameraActor* CineCam = Cast<ACineCameraActor>(CameraActor))
	{
		if (UCineCameraComponent* CinecamComp = CineCam->GetCineCameraComponent())
		{
			CinecamComp->CropSettings = DialogGraph->CropSettings;
			CinecamComp->SetCurrentFocalLength(DialogGraph->FocalLength);
			CinecamComp->SetCurrentAperture(DialogGraph->Aperture);
			CinecamComp->SetFilmback(DialogGraph->Filmback);
			CinecamComp->SetLensSettings(DialogGraph->LensSettings);
			CinecamComp->SetConstraintAspectRatio(DialogGraph->bConstrainAspectRatio);
			CinecamComp->bOverride_CustomNearClippingPlane = DialogGraph->bOverride_CustomNearClippingPlane;
			CinecamComp->CustomNearClippingPlane = DialogGraph->CustomNearClippingPlane;

			CinecamComp->FocusSettings.bSmoothFocusChanges = true;
			CinecamComp->FocusSettings.FocusSmoothingInterpSpeed = 15.0f;
			CinecamComp->FocusSettings.FocusMethod = DialogGraph->FocusMethod;
			if (DialogGraph->bUsePostProcess)
			{
				CinecamComp->PostProcessSettings = DialogGraph->PostProcessSettings;
			}
			else
			{
				CinecamComp->PostProcessSettings = FPostProcessSettings();
			}


			
		}
	}
}


void FDialogBuilderEditor::GenerateSequenceCameraFromDialogSections(const UDialogGenerateCameraSettings* InSettings)
{
	if (!Sequencer.IsValid() || !EditingDialogSequence || !CurrentSequenceNode.IsValid() || !DialogCameraPresetsWidget.IsValid())
	{
		return;
	}

	UDialogStage* DialogStage = CurrentSequenceNode.Get()->DialogStage;
	if (!DialogStage || !DialogStage->CameraSlots.IsValidIndex(0) || !DialogStage->CameraSlots[0])
	{
		return;
	}

	UMovieScene* MovieScene = EditingDialogSequence->GetMovieScene();
	if (!MovieScene)
	{
		return;
	}

	const FGuid CameraBindingId = DialogStage->CameraSlots[0]->ID;
	if (!CameraBindingId.IsValid())
	{
		return;
	}

	UMovieSceneDialogTrack* DialogTrack = MovieScene->FindTrack<UMovieSceneDialogTrack>();
	if (!DialogTrack)
	{
		return;
	}

	AActor* CameraActor = GetDialogCamera();
	if (!IsValid(CameraActor))
	{
		return;
	}


	const bool bRandomizeCameraAngles = InSettings ? InSettings->bRandomizeCameraAngles : false;
	const bool bRandomizeCameraPresets = InSettings ? InSettings->bRandomizeCameraPresets : true;
	const EMovieSceneKeyInterpolation KeyInterpolationType = InSettings ? InSettings->KeyInterpolation : EMovieSceneKeyInterpolation::Constant;
	const TArray<UDialogSequenceShot*> CameraPresets = InSettings ? InSettings->DefaultShots : TArray<UDialogSequenceShot*>();
	const bool bOverridePlaybackRangeStart = InSettings ? InSettings->bOverridePlaybackRangeStart : false;
	const bool bOverridePlaybackRangeEnd = InSettings ? InSettings->bOverridePlaybackRangeEnd : false;
	const float PlaybackRangeStart = InSettings ? InSettings->PlaybackRangeStart : 0.f;
	const float PlaybackRangeEnd = InSettings ? InSettings->PlaybackRangeEnd : 0.f;

	const FFrameNumber DefaultPlaybackRangeStart = MovieScene->GetPlaybackRange().GetLowerBoundValue();
	const FFrameNumber DefaultPlaybackRangeEnd = MovieScene->GetPlaybackRange().GetUpperBoundValue();

	FFrameNumber EffectivePlaybackRangeStart = bOverridePlaybackRangeStart
		? FFrameNumber(FMath::RoundToInt(PlaybackRangeStart))
		: DefaultPlaybackRangeStart;

	FFrameNumber EffectivePlaybackRangeEnd = bOverridePlaybackRangeEnd
		? FFrameNumber(FMath::RoundToInt(PlaybackRangeEnd))
		: DefaultPlaybackRangeEnd;

	if (EffectivePlaybackRangeEnd < EffectivePlaybackRangeStart)
	{
		Swap(EffectivePlaybackRangeStart, EffectivePlaybackRangeEnd);
	}

	UMovieScene3DTransformTrack* TransformTrack = MovieScene->FindTrack<UMovieScene3DTransformTrack>(CameraBindingId);
	if (!TransformTrack)
	{
		TransformTrack = MovieScene->AddTrack<UMovieScene3DTransformTrack>(CameraBindingId);
	}
	if (!TransformTrack)
	{
		return;
	}

	UMovieScene3DTransformSection* TransformSection = nullptr;
	if (TransformTrack->GetAllSections().Num() == 0)
	{
		TransformSection = Cast<UMovieScene3DTransformSection>(TransformTrack->CreateNewSection());
		TransformTrack->AddSection(*TransformSection);
	}
	else
	{
		TransformSection = Cast<UMovieScene3DTransformSection>(TransformTrack->GetAllSections()[0]);
	}
	if (!TransformSection)
	{
		return;
	}

	//modify
	MovieScene->Modify();

	if (TransformTrack)
	{
		TransformTrack->Modify();
	}

	TransformSection->Modify();
	TransformSection->SetBlendType(EMovieSceneBlendType::Absolute);

	TArray<UMovieSceneSection*> DialogSections = DialogTrack->GetAllSections();
	DialogSections.RemoveAll([](UMovieSceneSection* Section) { return Section == nullptr; });

	DialogSections.Sort([](const UMovieSceneSection& A, const UMovieSceneSection& B)
		{
			const FFrameNumber StartA = A.GetRange().HasLowerBound() ? A.GetRange().GetLowerBoundValue() : FFrameNumber(0);
			const FFrameNumber StartB = B.GetRange().HasLowerBound() ? B.GetRange().GetLowerBoundValue() : FFrameNumber(0);
			return StartA < StartB;
		});

	FScopedSlowTask SlowTask(static_cast<float>(DialogSections.Num()) + 1.0f, LOCTEXT("GenerateDialogSequenceCamera_Progress", "Generating camera keys..."));
	SlowTask.MakeDialog(true);

	TArrayView<FMovieSceneDoubleChannel*> Channels = TransformSection->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
	for (FMovieSceneDoubleChannel* Channel : Channels)
	{
		if (!Channel)
		{
			continue;
		}

		TArray<FFrameNumber> KeyTimes;
		TArray<FKeyHandle> KeyHandles;

		Channel->GetKeys(
			TRange<FFrameNumber>(EffectivePlaybackRangeStart, EffectivePlaybackRangeEnd),
			&KeyTimes,
			&KeyHandles);

		if (KeyHandles.Num() > 0)
		{
			Channel->DeleteKeys(KeyHandles);
		}
	}

	if (Channels.Num() < 9)
	{
		return;
	}

	for (int32 Index = 0; Index < DialogSections.Num(); ++Index)
	{
		const float ProgressPercent = DialogSections.Num() > 0
			? ((Index + 1) / static_cast<float>(DialogSections.Num())) * 100.0f
			: 100.0f;

		SlowTask.EnterProgressFrame(
			1.0f,
			FText::Format(
				LOCTEXT("GenerateDialogSequenceCamera_ProgressPercent", "Generating camera keys... {0}%"),
				FText::AsNumber(FMath::RoundToInt(ProgressPercent))));

		UMovieSceneSection* Section = DialogSections[Index];
		if (!Section || !Section->GetRange().HasLowerBound())
		{
			continue;
		}

		const FFrameNumber SectionStartFrame = Section->GetRange().GetLowerBoundValue();
		if (SectionStartFrame < EffectivePlaybackRangeStart || SectionStartFrame > EffectivePlaybackRangeEnd)
		{
			continue;
		}

		const int32 PresetIndex = FMath::RandHelper(CameraPresets.Num());
		UDialogSequenceShot* Preset = bRandomizeCameraPresets && CameraPresets.IsValidIndex(PresetIndex) ? CameraPresets[PresetIndex] : InSettings->ShotToUse.Get();
		if (!Preset)
		{
			continue;
		}

		const FFrameNumber StartFrame = SectionStartFrame;

		if (ApplyCameraPreset(Preset, StartFrame.Value, bRandomizeCameraAngles))
		{
			AddKeyFromCameraPreset(StartFrame.Value, KeyInterpolationType);
		}
	}

	SlowTask.EnterProgressFrame(1.0f, LOCTEXT("GenerateDialogSequenceCamera_Finished", "Generating..."));

	const double EndTime = FPlatformTime::Seconds() + 0.5;
	while (FPlatformTime::Seconds() < EndTime)
	{
		FSlateApplication::Get().Tick();
		FPlatformProcess::Sleep(0.01f);
	}

	TransformSection->SetRange(TRange<FFrameNumber>(EffectivePlaybackRangeStart, EffectivePlaybackRangeEnd));
	Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::RefreshTree);
	Sequencer->ForceEvaluate();
	SetViewportCameraMode(EDialogViewportCameraMode::DialogCameraLock);
}

bool FDialogBuilderEditor::HasBindingWithValid3DTransformSection(const FGuid& InBindingId) const
{
	if (!InBindingId.IsValid() || !EditingDialogSequence)
	{
		return false;
	}

	UMovieScene* MovieScene = EditingDialogSequence->GetMovieScene();
	if (!MovieScene)
	{
		return false;
	}

	const FMovieSceneBinding* Binding = MovieScene->FindBinding(InBindingId);
	if (!Binding)
	{
		return false;
	}

	const UMovieScene3DTransformTrack* TransformTrack = MovieScene->FindTrack<UMovieScene3DTransformTrack>(InBindingId);
	if (!TransformTrack)
	{
		return false;
	}

	for (UMovieSceneSection* Section : TransformTrack->GetAllSections())
	{
		if (Section)
		{
			return true;
		}
	}

	return false;
}

bool FDialogBuilderEditor::ApplyCameraPreset(UDialogSequenceShot* InSequenceShot, int32 InFrameNumber, bool bRandomizeAngle)
{
	if (!InSequenceShot || !EditingDialogGraph || !Sequencer.IsValid() || !EditingDialogSequence)
	{
		return false;
	}

	UMovieScene* MovieScene = EditingDialogSequence->GetMovieScene();
	if (!MovieScene)
	{
		return false;
	}

	UDialogDefinition* InDialogDefinition = nullptr;
	const FFrameNumber CurrentFrame = InFrameNumber >= 0 ? InFrameNumber : Sequencer->GetLocalTime().Time.FloorToFrame();

	if (InSequenceShot->ActorToFocus)
	{
		InDialogDefinition = InSequenceShot->ActorToFocus;
	}
	else
	{
		for (UMovieSceneTrack* Track : MovieScene->GetTracks())
		{
			UMovieSceneDialogTrack* DialogTrack = Cast<UMovieSceneDialogTrack>(Track);
			if (!DialogTrack)
			{
				continue;
			}

			for (UMovieSceneSection* Section : DialogTrack->GetAllSections())
			{
				UMovieSceneDialogSection* DialogSection = Cast<UMovieSceneDialogSection>(Section);
				if (!DialogSection || !DialogSection->GetRange().Contains(CurrentFrame))
				{
					continue;
				}

				InDialogDefinition = DialogSection->SpeakerParticipantDefinition;
				break;
			}
		}
	}




	AActor* ActorToFocus = nullptr;

	for (const TPair<UDialogSequenceSlot*, TWeakObjectPtr<AActor>>& Pair : DialogSlotActors)
	{
		if (AActor* Actor = Pair.Value.Get())
		{
			if (Actor->IsA<ACharacter>())
			{
				ActorToFocus = Actor;
				break;
			}
		}
	}

	for (const TPair<UDialogSequenceSlot*, TWeakObjectPtr<AActor>>& Pair : DialogSlotActors)
	{
		if (UDialogSequenceSlot* Slot = Pair.Key)
		{
			if (!Slot->DialogDefinition) continue;
			if (Slot->DialogDefinition == InDialogDefinition)
			{
				ActorToFocus = Pair.Value.Get();
				break;
			}
		}
	}

	if (!IsValid(ActorToFocus))
	{
		return false;
	}

	AActor* CameraActor = DialogCamera.Get();
	if (!IsValid(CameraActor))
	{
		return false;
	}

	InSequenceShot->K2_PreCameraSetup();

	FVector FocusLocation = ActorToFocus->GetActorLocation();
	FVector ForwardVector = ActorToFocus->GetActorForwardVector();

	if (InSequenceShot->TrackedBone != NAME_None)
	{
		TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		ActorToFocus->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);

		for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshComponents)
		{
			if (SkeletalMesh && SkeletalMesh->DoesSocketExist(InSequenceShot->TrackedBone))
			{
				const FTransform SocketTransform = SkeletalMesh->GetSocketTransform(
					InSequenceShot->TrackedBone,
					ERelativeTransformSpace::RTS_World);

				FocusLocation = SocketTransform.GetLocation();
				break;
			}
		}
	}

	// Flatten the camera placement direction so the camera stays level.
	ForwardVector.Z = 0.0f;
	ForwardVector = ForwardVector.GetSafeNormal();

	if (ForwardVector.IsNearlyZero())
	{
		ForwardVector = ActorToFocus->GetActorForwardVector();
		ForwardVector.Z = 0.0f;
		ForwardVector = ForwardVector.GetSafeNormal();
	}

	// TargetOffset now follows the actor axes:
	// X = forward, Y = right, Z = up.
	const FVector ActorForward = ActorToFocus->GetActorForwardVector().GetSafeNormal();
	const FVector ActorRight = ActorToFocus->GetActorRightVector().GetSafeNormal();
	const FVector ActorUp = ActorToFocus->GetActorUpVector().GetSafeNormal();

	const FVector TargetOffsetWorld =
		(ActorForward * InSequenceShot->TargetOffset.X) +
		(ActorRight * InSequenceShot->TargetOffset.Y) +
		(ActorUp * InSequenceShot->TargetOffset.Z);

	const FVector AimLocation = FocusLocation + TargetOffsetWorld;

	const FVector CameraWorldOffset = ActorToFocus->GetActorTransform().TransformVectorNoScale(InSequenceShot->CameraOffset);
	FVector DesiredLocation = FocusLocation + (ForwardVector * InSequenceShot->CameraDistance) + CameraWorldOffset;

	if (bRandomizeAngle)
	{
		constexpr float MinYaw = -20.0f;
		constexpr float MaxYaw = 20.0f;
		constexpr float MinPitch = -5.0f;
		constexpr float MaxPitch = 10.0f;

		const FVector OrbitOffset = DesiredLocation - AimLocation;
		const float OrbitDistance = OrbitOffset.Size();

		if (OrbitDistance > KINDA_SMALL_NUMBER)
		{
			FRotator OrbitRotation = OrbitOffset.Rotation();
			OrbitRotation.Yaw += FMath::RandRange(MinYaw, MaxYaw);
			OrbitRotation.Pitch += FMath::RandRange(MinPitch, MaxPitch);

			const FVector RandomizedDirection = OrbitRotation.Vector();
			DesiredLocation = AimLocation + (RandomizedDirection * OrbitDistance);
		}
	}

	const FVector RelativeOffset = ActorToFocus->GetActorTransform().InverseTransformPosition(AimLocation);
	ACineCameraActor* CineCam = Cast<ACineCameraActor>(CameraActor);
	UCineCameraComponent* CinecamComp = CineCam ? CineCam->GetCineCameraComponent() : nullptr;

	if (CinecamComp)
	{
		//clear existing focus settings
		CinecamComp->FocusSettings.TrackingFocusSettings.ActorToTrack = nullptr;
		if (InSequenceShot->bFocusCameraOnActor)
		{
			if (ActorToFocus)
			{
				CinecamComp->FocusSettings.TrackingFocusSettings.RelativeOffset = RelativeOffset;
				CinecamComp->FocusSettings.TrackingFocusSettings.ActorToTrack = ActorToFocus;
			}
		}

	}
	
	const FRotator DesiredRotation = (AimLocation - DesiredLocation).Rotation();

	if (InSequenceShot->bDrawTargetFocus)
	{
		DrawDebugSphere(
			ActorToFocus->GetWorld(),
			AimLocation,
			5.0f,
			12,
			FColor::Green,
			false,
			3.0f);
	}


	CameraActor->SetActorLocation(DesiredLocation, false);
	CameraActor->SetActorRotation(DesiredRotation);
	if (DialogViewportWidget.IsValid())
	{
		if (FDialogBuilderViewportClient* ViewportClient = DialogViewportWidget->GetDialogViewportClientPtr())
		{
			ViewportClient->SetViewLocation(DesiredLocation);
			ViewportClient->SetViewRotation(DesiredRotation);
			ViewportClient->Invalidate();
		}
	}
	return true;
}

void FDialogBuilderEditor::AddKeyFromCameraPreset(int32 InFrameNumber, EMovieSceneKeyInterpolation KeyInterpolationType)
{
	if (!Sequencer.IsValid() || !EditingDialogSequence || !CurrentSequenceNode.IsValid() || !DialogCameraPresetsWidget.IsValid())
	{
		return;
	}

	UDialogStage* DialogStage = CurrentSequenceNode.Get()->DialogStage;
	if (!DialogStage || !DialogStage->CameraSlots.IsValidIndex(0) || !DialogStage->CameraSlots[0])
	{
		return;
	}
	UMovieScene* MovieScene = EditingDialogSequence->GetMovieScene();
	if(!MovieScene)
	{
		return;
	}

	AActor* CameraActor = GetDialogCamera();
	if (!IsValid(CameraActor))
	{
		return;
	}

	ACineCameraActor* CineCam = Cast<ACineCameraActor>(CameraActor);
	UCineCameraComponent* CinecamComp = CineCam ? CineCam->GetCineCameraComponent() : nullptr;


	const USceneComponent* RootComponent = CameraActor->GetRootComponent();
	if (!RootComponent)
	{
		return;
	}

	const FVector Location = RootComponent->GetRelativeLocation();
	const FRotator Rotation = RootComponent->GetRelativeRotation();
	const FVector Scale = RootComponent->GetRelativeScale3D();

	const FGuid CameraBindingId = DialogStage->CameraSlots[0]->ID;
	if (!CameraBindingId.IsValid())
	{
		return;
	}

	UMovieScene3DTransformTrack* TransformTrack = MovieScene->FindTrack<UMovieScene3DTransformTrack>(CameraBindingId);
	if (!TransformTrack)
	{
		TransformTrack = MovieScene->AddTrack<UMovieScene3DTransformTrack>(CameraBindingId);
	}
	if (!TransformTrack)
	{
		return;
	}

	UMovieScene3DTransformSection* TransformSection = nullptr;
	if (TransformTrack->GetAllSections().Num() == 0)
	{
		TransformSection = Cast<UMovieScene3DTransformSection>(TransformTrack->CreateNewSection());
		TransformTrack->AddSection(*TransformSection);
	}
	else
	{
		TransformSection = Cast<UMovieScene3DTransformSection>(TransformTrack->GetAllSections()[0]);
	}
	if (!TransformSection)
	{
		return;
	}

	TArrayView<FMovieSceneDoubleChannel*> Channels = TransformSection->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();

	if (Channels.Num() < 9)
	{
		return;
	}

	//if frame number is not specified, use current sequencer time
	const FFrameNumber InFrame = (InFrameNumber >= 0) ? FFrameNumber(InFrameNumber) : Sequencer->GetLocalTime().Time.FloorToFrame();

	TransformSection->Modify();
	TransformSection->SetBlendType(EMovieSceneBlendType::Absolute);


	auto AddKey = [KeyInterpolationType](FMovieSceneDoubleChannel* Channel, FFrameNumber Frame, double Value)
		{
			if (!Channel)
			{
				return;
			}

			switch (KeyInterpolationType)
			{
			case EMovieSceneKeyInterpolation::Constant:
				Channel->AddConstantKey(Frame, Value);
				break;

			case EMovieSceneKeyInterpolation::Linear:
				Channel->AddLinearKey(Frame, Value);
				break;
			case EMovieSceneKeyInterpolation::Auto:
				Channel->AddCubicKey(Frame, Value, RCTM_Auto);
				break;
			case EMovieSceneKeyInterpolation::User:
				Channel->AddCubicKey(Frame, Value, RCTM_User);
				break;
			case EMovieSceneKeyInterpolation::Break:
				Channel->AddCubicKey(Frame, Value, RCTM_Break);
				break;
			case EMovieSceneKeyInterpolation::SmartAuto:
				Channel->AddCubicKey(Frame, Value, RCTM_SmartAuto);
				break;
			default:
				Channel->AddCubicKey(Frame, Value);
				break;
			}
		};


	AddKey(Channels[0], InFrame, Location.X);
	AddKey(Channels[1], InFrame, Location.Y);
	AddKey(Channels[2], InFrame, Location.Z);

	AddKey(Channels[3], InFrame, Rotation.Roll);
	AddKey(Channels[4], InFrame, Rotation.Pitch);
	AddKey(Channels[5], InFrame, Rotation.Yaw);

	AddKey(Channels[6], InFrame, Scale.X);
	AddKey(Channels[7], InFrame, Scale.Y);
	AddKey(Channels[8], InFrame, Scale.Z);

	if (CinecamComp && IsValid(CinecamComp->FocusSettings.TrackingFocusSettings.ActorToTrack.Get()))
	{
		AddActorToTrackKeyIfValid(MovieScene, Sequencer.Get(), CinecamComp, InFrame);
	}

	const FFrameNumber DefaultPlaybackRangeStart = MovieScene->GetPlaybackRange().GetLowerBoundValue();
	const FFrameNumber DefaultPlaybackRangeEnd = MovieScene->GetPlaybackRange().GetUpperBoundValue();
	TransformSection->SetRange(TRange<FFrameNumber>(DefaultPlaybackRangeStart, DefaultPlaybackRangeEnd));
}


TSharedRef<SDockTab> FDialogBuilderEditor::SpawnTab_Sequencer(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDialogBuilderEditorTabs::DialogSequencerTabID);

	EnsureSequencerCreated();

	return SNew(SDockTab)
		.Label(LOCTEXT("DialogSequencerTab_Title", "Sequencer"))
		[
			Sequencer.IsValid()
				? Sequencer->GetSequencerWidget()
				: SNullWidget::NullWidget
		];
}

TSharedRef<SDockTab> FDialogBuilderEditor::SpawnTab_CurveEditor(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDialogBuilderEditorTabs::SequencerGraphEditor);

	return SNew(SDockTab)
		.Label(NSLOCTEXT("Sequencer", "SequencerMainGraphEditorTitle", "Sequencer Curves"))
		[
			SNullWidget::NullWidget
		];
}

TSharedRef<SDockTab> FDialogBuilderEditor::SpawnTab_CameraPresets(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDialogBuilderEditorTabs::DialogSequencerViewportID);

	return SNew(SDockTab)
		.Label(LOCTEXT("DialogSequencerViewport_Title", "Viewport"))
		[
			DialogCameraPresetsWidget.IsValid()
				? DialogCameraPresetsWidget.ToSharedRef()
				: SNullWidget::NullWidget
		];
}


TSharedRef<SDockTab> FDialogBuilderEditor::SpawnTab_SequencerViewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDialogBuilderEditorTabs::DialogCameraPresetsID);

	return SNew(SDockTab)
		.Label(LOCTEXT("DialogSequencerViewport_Title", "Viewport"))
		[
			DialogViewportWidget.IsValid()
				? DialogViewportWidget.ToSharedRef()
				: SNullWidget::NullWidget
		];
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

void FDialogBuilderEditor::RestoreDialogEditor()
{
	// Update dialog asset data based on saved graph to have correct data in editor
	TWeakObjectPtr< UEdGraph > FocusedGraphPtr = EditingDialogGraph->DialogGraphPages.Num() > 0 ? EditingDialogGraph->DialogGraphPages[0] : nullptr;
	UDialogBuilderEdGraph* MyGraph = Cast<UDialogBuilderEdGraph>(FocusedGraphPtr.Get());
	const bool bNewGraph = MyGraph == NULL;

	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(MyGraph);
	TSharedPtr<SDockTab> DocumentTab = DocumentManager->OpenDocument(Payload, bNewGraph ? FDocumentTracker::OpenNewDocument : FDocumentTracker::RestorePreviousDocument);

		
	

	if (EditingDialogGraph->LastEditedDocuments.Num() > 0 && DocumentTab.IsValid())
	{
		TSharedPtr<SWidget> Content = DocumentTab->GetContent();
		if (Content.IsValid())
		{
			TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Content.ToSharedRef());
			GraphEditor->SetViewLocation(
				EditingDialogGraph->LastEditedDocuments[0].SavedViewOffset,
				EditingDialogGraph->LastEditedDocuments[0].SavedZoomAmount
			);
		}
	}

}

void FDialogBuilderEditor::SaveEditedObjectState()
{
	// Clear currently edited documents
	EditingDialogGraph->LastEditedDocuments.Empty();

	// Ask all open documents to save their state, which will update LastEditedDocuments
	DocumentManager->SaveAllState();
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
	if (DialogDefinitionsWidget.IsValid())
	{
		// Force a refresh immediately, the item has to be present in the list for the rename redialogs to be successful.
		DialogDefinitionsWidget->Refresh();
		DialogDefinitionsWidget->SelectItemByName(InActionName, ESelectInfo::OnMouseClick);
		DialogDefinitionsWidget->OnRequestRenameOnActionNode();
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

bool FDialogBuilderEditor::CanAccessDialogEditorMode() const
{
	return true;
}

bool FDialogBuilderEditor::CanAccessDialogSequencerMode() const
{
	return true;
}

FText FDialogBuilderEditor::GetLocalizedMode(FName InMode)
{
	static TMap< FName, FText > LocModes;

	if (LocModes.Num() == 0)
	{
		LocModes.Add(DialogEditorMode, DialogEditorModeText);
		LocModes.Add(DialogSequencerMode,DialogSequencerModeText);
	}

	check(InMode != NAME_None);
	const FText* OutDesc = LocModes.Find(InMode);
	check(OutDesc);
	return *OutDesc;
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
	if (CurrentGraphWidget.IsValid() && DialogDefinitionsWidget.IsValid())
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
			DialogDefinitionsWidget->Refresh();
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
	return GEditor->IsSimulateInEditorInProgress() || GEditor->PlayWorld;
}

bool FDialogBuilderEditor::IsPIENotSimulating()
{
	return !GEditor->IsSimulateInEditorInProgress() && (GEditor->PlayWorld == NULL);
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

TSharedRef<SDockTab> FDialogBuilderEditor::SpawnTab_DialogStageSettings(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDialogBuilderEditorTabs::DialogStageSettingsID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif
		.Label(LOCTEXT("DialogStageSettings_Title", "Dialog Set Manager"))
		[
			DialogStageManagerWidget.ToSharedRef()
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

TSharedRef<SDockTab> FDialogBuilderEditor::SpawnTab_DialogDefinitions(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FDialogBuilderEditorTabs::DialogDefinitionsID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("DialogDefinitions_Title", "My Dialog"))
		[
			DialogDefinitionsWidget.ToSharedRef()
		];
}

void FDialogBuilderEditor::CreateInternalWidgets()
{
	//create details view
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;	

	PropertyWidget = PropertyModule.CreateDetailView(DetailsViewArgs);
	PropertyWidget->SetObject( EditingDialogGraph );
	PropertyWidget->OnFinishedChangingProperties().AddSP(this, &FDialogBuilderEditor::OnFinishedChangingProperties);

	DialogStageWidget = PropertyModule.CreateDetailView(DetailsViewArgs);



	DialogCameraPresetsWidget = SNew(SDialogCameraPresets, SharedThis(this));
	this->DialogDefinitionsWidget = SNew(SDialogDefinitions, SharedThis(this));
	DialogStageManagerWidget = SNew(SDialogStageManager, SharedThis(this), GetDialogBuilderGraph());

	if (!DialogViewportWidget.IsValid())
	{
		DialogViewportWidget = SNew(SDialogPreviewViewport, SharedThis(this));
		DialogViewportWidget->SetUseLevelWorld(ViewportWorldMode == EDialogViewportWorldMode::CurrentLevel);
	}
}


TSharedRef<SWidget> FDialogBuilderEditor::SpawnProperties()
{
	return
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Fill)
		[
			PropertyWidget.ToSharedRef()
		];
		
}

TSharedRef<SWidget> FDialogBuilderEditor::SpawnDialogStageSettings()
{
	return
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Fill)
		[
			DialogStageManagerWidget.ToSharedRef()
		];
}

TSharedRef<SWidget> FDialogBuilderEditor::SpawnDialogDefinitions()
{
	return DialogDefinitionsWidget.ToSharedRef();
}

TSharedRef<SWidget> FDialogBuilderEditor::SpawnDialogSequencerTab()
{
	EnsureSequencerCreated();

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Fill)
		[
			Sequencer.IsValid()
				? Sequencer->GetSequencerWidget()
				: SNullWidget::NullWidget
		];
}

TSharedRef<SWidget> FDialogBuilderEditor::SpawnDialogSequencerViewportTab()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Fill)
		[
			DialogViewportWidget.IsValid()
				? DialogViewportWidget.ToSharedRef()
				: SNullWidget::NullWidget
		];
}

TSharedRef<SWidget> FDialogBuilderEditor::SpawnDialogCameraPresetsTab()
{
	return
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Fill)
		[
			DialogCameraPresetsWidget.IsValid()
				? DialogCameraPresetsWidget.ToSharedRef()
				: SNullWidget::NullWidget
		];
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

void FDialogBuilderEditor::HandleNewClassPicked(UClass* InClass) const
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

void FDialogBuilderEditor::CreateNewDialogStageTemplate(UDialogStage* InDialogStage)
{
	if (!InDialogStage || !EditingDialogGraph || !EditingDialogGraph->GetOutermost())
	{
		return;
	}

	FString PathName = EditingDialogGraph->GetOutermost()->GetPathName();
	PathName = FPaths::GetPath(PathName);

	const FString DefaultStageName = TEXT("DialogStage");

	FSaveAssetDialogConfig SaveAssetDialogConfig;
	SaveAssetDialogConfig.DialogTitleOverride = LOCTEXT("SaveDialogStageTemplateTitle", "Save Dialog Stage Template");
	SaveAssetDialogConfig.DefaultPath = PathName;
	SaveAssetDialogConfig.DefaultAssetName = DefaultStageName;
	SaveAssetDialogConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::Disallow;

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	const FString SaveObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveAssetDialogConfig);
	if (!SaveObjectPath.IsEmpty())
	{
		const FString SavePackageName = FPackageName::ObjectPathToPackageName(SaveObjectPath);
		const FString SaveAssetName = FPaths::GetBaseFilename(SavePackageName);

		UPackage* Package = CreatePackage(*SavePackageName);
		if (ensure(Package))
		{
			if (UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(
				UDialogStage::StaticClass(),
				Package,
				FName(*SaveAssetName),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass()))
			{
				if (UDialogStage* DefaultStage = Cast<UDialogStage>(NewBP->GeneratedClass->GetDefaultObject()))
				{
					DefaultStage->Modify();
					DefaultStage->bIsTemplate = true;
					DefaultStage->OwningDialogGraph = nullptr;
					DefaultStage->Name = InDialogStage->Name;
					DefaultStage->Location = InDialogStage->Location;
					DefaultStage->Rotation = InDialogStage->Rotation;

					DefaultStage->Slots.Empty();
					DefaultStage->CameraSlots.Empty();
					DefaultStage->LightSlots.Empty();

					for (UDialogSequenceSlot* Slot : InDialogStage->Slots)
					{
						if (!Slot)
						{
							continue;
						}

						if (UDialogSequenceSlot* NewSlot = DuplicateObject<UDialogSequenceSlot>(Slot, DefaultStage))
						{
							NewSlot->OwningDialogGraph = nullptr;
							NewSlot->DialogDefinition = nullptr;
							DefaultStage->Slots.Add(NewSlot);
						}
					}

					for (UDialogSequenceSlot* Slot : InDialogStage->CameraSlots)
					{
						if (!Slot)
						{
							continue;
						}

						if (UDialogSequenceSlot* NewSlot = DuplicateObject<UDialogSequenceSlot>(Slot, DefaultStage))
						{
							NewSlot->OwningDialogGraph = nullptr;
							NewSlot->DialogDefinition = nullptr;
							DefaultStage->CameraSlots.Add(NewSlot);
						}
					}

					for (UDialogSequenceSlot_Light* Slot : InDialogStage->LightSlots)
					{
						if (!Slot)
						{
							continue;
						}

						if (UDialogSequenceSlot_Light* NewSlot = DuplicateObject<UDialogSequenceSlot_Light>(Slot, DefaultStage))
						{
							NewSlot->OwningDialogGraph = nullptr;
							NewSlot->DialogDefinition = nullptr;
							DefaultStage->LightSlots.Add(NewSlot);
						}
					}
				}

				GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(NewBP);
				FAssetRegistryModule::AssetCreated(NewBP);
				Package->MarkPackageDirty();
			}
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

void FDialogBuilderEditor::CreateNewDialogDecorator()
{
	HandleNewClassPicked(UOrionDecorator::StaticClass());
}
bool FDialogBuilderEditor::CanCreateDialogDecorator() const
{
	return true;
}
void FDialogBuilderEditor::CreateNewDialogEvent()
{
	HandleNewClassPicked(UOrionEvent::StaticClass());
}
bool FDialogBuilderEditor::CanCreateDialogEvent() const
{
	return true;
}
void FDialogBuilderEditor::CreateNewDialogCameraShot()
{
	HandleNewClassPicked(UDialogCameraShot::StaticClass());
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

void FDialogBuilderEditor::SetViewportWorldMode(EDialogViewportWorldMode NewMode)
{
	if (ViewportWorldMode == NewMode)
	{
		return;
	}

	ViewportWorldMode = NewMode;

	DestroyDialogStage();

	if (DialogViewportWidget.IsValid())
	{
		DialogViewportWidget->SetUseLevelWorld(ViewportWorldMode == EDialogViewportWorldMode::CurrentLevel);
	}

	UpdateDialogStage();
	ApplyViewportCameraMode();

}

void FDialogBuilderEditor::SetViewportCameraMode(EDialogViewportCameraMode NewMode)
{
	ViewportCameraMode = NewMode;
	ApplyViewportCameraMode();
}

void FDialogBuilderEditor::ApplyViewportCameraMode()
{
	if (!DialogViewportWidget.IsValid())
	{
		return;
	}

	FDialogBuilderViewportClient* ViewportClient = DialogViewportWidget->GetDialogViewportClientPtr();
	if (!ViewportClient)
	{
		return;
	}

	auto UnlockCinematic = [this, ViewportClient]()
		{
			if (ViewportClient->IsLockedToCinematic())
			{
			}

			ViewportClient->SetCinematicActorLock(nullptr);
			ViewportClient->UpdateViewForLockedActor();
			DialogViewportWidget->ResetCameraSetting();
			DialogViewportWidget->OnActorUnlock();
			ViewportClient->SetViewportType(LVT_Perspective);
			ViewportClient->Invalidate();

			LastSequencerCameraCutActor.Reset();
		};

	switch (ViewportCameraMode)
	{
	case EDialogViewportCameraMode::Perspective:
		if (Sequencer.IsValid())
		{
			Sequencer->SetPerspectiveViewportCameraCutEnabled(false);
		}
		UnlockCinematic();
		break;

	case EDialogViewportCameraMode::DialogCameraLock:

		UnlockCinematic();

		if (Sequencer.IsValid())
		{
			Sequencer->SetPerspectiveViewportCameraCutEnabled(false);
		}
		if (AActor* DialogCameraActor = DialogCamera.Get())
		{
			DialogViewportWidget->OnActorLockToggleFromMenu(DialogCameraActor);
		}
		
		break;

	case EDialogViewportCameraMode::SequencerCameraCuts:
		EnsureSequencerCreated();
		if (Sequencer.IsValid())
		{
			Sequencer->SetPerspectiveViewportCameraCutEnabled(true);
		}
		RefreshSequencerCameraLock();
		break;
	}

}

void FDialogBuilderEditor::SetDetailsObject(UObject* InObject)
{
	if (!PropertyWidget.IsValid())
	{
		return;
	}

	PropertyWidget->SetObject(InObject);
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

	if(UDialogBuilderNode_DialogSequence* DialogSequenceNode = Cast<UDialogBuilderNode_DialogSequence>(DialogEdNode ? DialogEdNode->NodeInstance : nullptr))
	{
		OnOpenDialogSequenceNode(DialogSequenceNode);	}


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

		if (UDialogBuilderEdGraph* DialogEdGraph = Cast<UDialogBuilderEdGraph>(EdGraph))
		{
			DialogEdGraph->UpdateAsset(true);
			DialogEdGraph->NotifyGraphChanged();
		}
	}
	DocumentManager->RefreshAllTabs();

}
#include "Async/Async.h"

void FDialogBuilderEditor::OnPackageMarkedDirty(UPackage* ModifiedPackage, bool bWasDirty)
{   
	if (!EditingDialogGraph || !EditingDialogGraph->GetOutermost())
	{
		return;
	}

	if (ModifiedPackage != EditingDialogGraph->GetOutermost())
	{
		return;
	}


	bPendingRefreshDialogEditor = true;
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
	/*if (DialogViewportWidget.IsValid() && CurrentSequenceNode.IsValid())
	{
		if (UTexture2D* Thumbnail = DialogViewportWidget->CaptureViewportThumbnail())
		{
			CurrentSequenceNode->Thumbnail = Thumbnail;
		}
	}*/
}


#endif // #else // #if ENGINE_MAJOR_VERSION < 5

#undef LOCTEXT_NAMESPACE
