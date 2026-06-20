// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderSetting.h"
#include "UObject/Package.h"
#include "SSubobjectEditor.h"
#include "Dialog_System_Editor.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "DialogBuilderGraph.h"
#include "Tickable.h"
#include "TickableEditorObject.h"
#include "KeyParams.h"
#include "Misc/ITransaction.h"
#include "Templates/SharedPointer.h"

#include "DialogBuilderEditor.generated.h"

class FAdvancedPreviewScene;
class SDialogPreviewViewport;
class ISequencer;
class ULevelSequence;
class UDialogStage;
class SDialogDefinitions;
class SDialogCameraPresets;
class UDialogParticipant;
class UDialogSequence;
class UDialogSequenceSlot;
class UDialogDefinition;
class ACineCameraActor;
class FToolBarBuilder;

enum class EMovieSceneDataChangeType;

namespace DialogSectionID
{
	enum Type
	{
		NONE = 0,
		PARTICIPANTS,	// Participants
		PROPS,			// Props actor 
		//Add More to populate the section
	};
};

enum class EDialogViewportWorldMode : uint8
{
	PreviewScene,
	CurrentLevel
};

enum class EDialogViewportCameraMode : uint8
{
	Perspective,
	DialogCameraLock,
	SequencerCameraCuts
};

struct FDialogBuilderEditorTabs
{
	// Tab identifiers
	static const FName DialogDefinitionsID;
	static const FName DialogBuilderPropertyID;
	static const FName DialogStageSettingsID;
	static const FName ViewportID;
	static const FName DialogBuilderEditorSettingsID;
	static const FName DialogSequencerTabID;
	static const FName DialogSequencerViewportID;
	static const FName DialogCameraPresetsID;
	static const FName SequencerGraphEditor;

};



UCLASS()
class UDialogSequenceEditorMenuContext : public UObject
{
	GENERATED_BODY()
public:
	TWeakPtr<class FDialogBuilderEditor> DialogEditor;
};


/**
 * Dialog editor public interface
 */
class DIALOG_SYSTEM_EDITOR_API IDialogEditor : public FWorkflowCentricApplication
{
public:
	virtual void JumpToHyperlink(const UObject* ObjectReference, bool bRedialogRename) = 0;
	virtual void JumpToPin(const UEdGraphPin* PinToFocusOn) = 0;

	/** Invokes the search UI and sets the mode and search terms optionally */
	virtual void SummonSearchUI(bool bSetFindWithinDialogSystem, FString NewSearchTerms = FString(), bool bSelectFirstResult = false) = 0;

	/** Invokes the Find and Replace UI */
	virtual void SummonFindAndReplaceUI() = 0;

	/** Tries to open the specified graph and bring it's document to the front (note: this can return NULL) */
	virtual TSharedPtr<class SGraphEditor> OpenGraphAndBringToFront(class UEdGraph* Graph, bool bSetFocus = true) = 0;

	virtual void RefreshEditors() = 0;

	virtual void RefreshDialogDefinitions() = 0;

	virtual void RefreshInspector() = 0;

	virtual void AddToSelection(UEdGraphNode* InNode) = 0;

	virtual bool CanPasteNodes() const = 0;

	virtual void PasteNodesHere(class UEdGraph* Graph, const FVector2D& Location) = 0;

	/** Return the class viewer filter associated with the current set of imported namespaces within this editor context. Default is NULL (no filter). */
	virtual TSharedPtr<class IClassViewerFilter> GetImportedClassViewerFilter() const { return nullptr; }

	/** Return the pin type selector filter associated with the current set of imported namespaces within this editor context. Default is NULL (no filter). */
	UE_DEPRECATED(5.1, "Please use GetPinTypeSelectorFilters")
		virtual TSharedPtr<class IPinTypeSelectorFilter> GetImportedPinTypeSelectorFilter() const { return nullptr; }

	/** Get all the the pin type selector filters within this editor context. */
	virtual void GetPinTypeSelectorFilters(TArray<TSharedPtr<class IPinTypeSelectorFilter>>& OutFilters) const {}

	/** Return whether the given object falls outside the scope of the current set of imported namespaces within this editor context. Default is FALSE (imported). */
	virtual bool IsNonImportedObject(const UObject* InObject) const { return false; }

	/** Return whether the given object (referenced by path) falls outside the scope of the current set of imported namespaces within this editor context. Default is FALSE (imported). */
	virtual bool IsNonImportedObject(const FSoftObjectPath& InObject) const { return false; }

};


class DIALOG_SYSTEM_EDITOR_API FDialogBuilderEditor : public IDialogEditor,
	public FTickableEditorObject,
	public FNotifyHook,
	public FGCObject, 
	public FEditorUndoClient
{
public:
	FDialogBuilderEditor();
	virtual	~FDialogBuilderEditor();

	void Initialize(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UObject* InObject);


	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	// End of IToolkit interface

	//~ Begin FTickableEditorObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual TStatId GetStatId() const override;
	//~ End FTickableEditorObject Interface
	// 
	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// End of FEditorUndoClient

	// FAssetEditorToolkit
	virtual const FSlateBrush* GetDefaultTabIcon() const override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FString GetDocumentationLink() const override;
	virtual void SaveAsset_Execute() override;
	// End of FAssetEditorToolkit
	
	//IDialogEditor
	virtual void RefreshEditors() override;
	virtual void RefreshDialogDefinitions();
	virtual void RefreshInspector();
	virtual void AddToSelection(UEdGraphNode* InNode) override;
	virtual void JumpToHyperlink(const UObject* ObjectReference, bool bRedialogRename = false) override;
	virtual void JumpToPin(const class UEdGraphPin* Pin) override;
	virtual void SummonSearchUI(bool bSetFindWithinBlueprint, FString NewSearchTerms = FString(), bool bSelectFirstResult = false) override;
	virtual void SummonFindAndReplaceUI() override;
	virtual TSharedPtr<SGraphEditor> OpenGraphAndBringToFront(UEdGraph* Graph, bool bSetFocus = true) override;
	// End of IDialogEditor

	void BindDelegates();
	void UnbindDelegates();


	void OnPieEvent(bool);
	void OnMapChange(uint32);
	void OnWorldAdded(UWorld*);
	void OnWorldDestroyed(UWorld*);
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void OnTransactionStateChanged(const FTransactionContext& TransactionContext, ETransactionStateEventType TransactionState);

	//Tabs
	/** Spawns the detail property tab */
	TSharedRef<SWidget> SpawnProperties();

	/** Spawns the dialog set settings tab */
	TSharedRef<SWidget> SpawnDialogStageSettings();

	/** Spawns the dialog definitions tab*/
	TSharedRef<SWidget> SpawnDialogDefinitions();

	/** Spawns the dialog sequencer tab*/
	TSharedRef<SWidget> SpawnDialogSequencerTab();
	
	/** Spawns the dialog viewport tab*/
	TSharedRef<SWidget> SpawnDialogSequencerViewportTab();

	/** Spawns the dialog camera presets tab*/
	TSharedRef<SWidget> SpawnDialogCameraPresetsTab();

	//Toolbar
	void UpdateToolbar();
	TSharedPtr<class FDialogBuilderEditorToolbar> GetToolbarBuilder() { return ToolbarBuilder; }
	void RegisterToolbarTab(const TSharedRef<class FTabManager>& TabManager);

	
	// FSerializableObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	// End of FSerializableObject interface

	/** Returns a pointer to the Dialog graph object we are currently editing, as long as we are editing exactly one */
	virtual UDialogBuilderGraph* GetDialogBuilderGraph() const;

	/** Restores the dialog graph we were editing or creates a new one if none is available */
	void RestoreDialogEditor();

	/** Save the graph state for later editing */
	void SaveEditedObjectState();

	// Type of new document/graph being created by a menu item
	enum ECreatedDialogDocumentType
	{
		CGT_None,
		CGT_NewDialogGraph,
	};

	// Called to see if the new document menu items is visible for this type
	virtual bool IsSectionVisible(DialogSectionID::Type InSectionID) const { return true; }
	virtual bool NewDocument_IsVisibleForType(ECreatedDialogDocumentType GraphType) const;
	EVisibility NewDocument_GetVisibilityForType(ECreatedDialogDocumentType GraphType) const
	{
		return NewDocument_IsVisibleForType(GraphType) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	/** Called when New Function button is clicked */
	virtual void NewDocument_OnClicked(ECreatedDialogDocumentType GraphType);
	FReply NewDocument_OnClick(ECreatedDialogDocumentType GraphType) { NewDocument_OnClicked(GraphType); return FReply::Handled(); }

	/** Returns true if in editing mode */
	bool InEditingMode() const;

	bool CanAddNewDialogGraph() const;

	TSharedPtr<SDockTab> OpenDocument(const UObject* DocumentID, FDocumentTracker::EOpenDocumentCause Cause);
	
	/** Finds the tab associated with the specified asset, and closes if it is open */
	void CloseDocumentTab(const UObject* DocumentID);

	/** Utility function to handle all steps required to rename a newly added action */
	void RenameNewlyAddedAction(FName InActionName);

	/** Throw a simple message into the log */
	void LogSimpleMessage(const FText& MessageText);

	/** Create a graph title bar widget */
	TSharedRef<SWidget> CreateGraphTitleBarWidget(TSharedRef<FTabInfo> InTabInfo, UEdGraph* InGraph);

	/**
	 * Util for finding a glyph for a graph
	 *
	 * @param Graph - The graph to evaluate
	 * @param bInLargeIcon - if true the icon returned is 22x22 pixels, else it is 16x16
	 * @return An appropriate brush to use to represent the graph, if the graph is an unknown type the function will return the default "function" glyph
	 */
	static const FSlateBrush* GetGlyphForGraph(const UEdGraph* Graph, bool bInLargeIcon = false);

	// Finds any open tabs containing the specified document and adds them to the specified array; returns true if at least one is found
	bool FindOpenTabsContainingDocument(const UObject* DocumentID, /*inout*/ TArray< TSharedPtr<SDockTab> >& Results);

	/** Create new tab for each element of LastEditedObjects array */
	void InitializeDocumentTab();

	/**	Returns whether the editor is currently editing a single Dialog object */
	bool IsEditingSingleDialogGraph() const;

	/**
	 * Returns the currently focused graph in the Dialog editor
	 */
	UEdGraph* GetFocusedGraph() const;

	/** Check whether the dialog editor mode can be accessed*/
	bool CanAccessDialogEditorMode() const;

	/** Check whether the dialog sequencer mode can be accessed*/
	bool CanAccessDialogSequencerMode() const;

	/**
	 * Get the localized text to display for the specified mode
	 * @param	InMode	The mode to display
	 * @return the localized text representation of the mode
	 */
	static FText GetLocalizedMode(FName InMode);

	/** Returns the currently selected node if there is a single node selected (if there are multiple nodes selected or none selected, it will return nullptr) */
	UEdGraphNode* GetSingleSelectedNode() const;

	/** Called when graph editor focus is changed */
	virtual void OnGraphEditorFocused(const TSharedRef<class SGraphEditor>& InGraphEditor);

	/** Called when the graph editor tab is backgrounded */
	virtual void OnGraphEditorBackgrounded(const TSharedRef<SGraphEditor>& InGraphEditor);

	/** Checks to see if the provided graph is contained within the current DialogBuilderGraph */
	bool IsGraphInCurrentDialogGraph(const UEdGraph* InGraph) const;

	/**
	 * Provides access to the preview scene.
	 */
	FPreviewScene* GetPreviewScene()
	{
		return &PreviewScene;
	}


	//Sequencer
	void OnOpenDialogSequenceNode(class UDialogBuilderNode_DialogSequence* InSequenceNode);
	void OpenDialogSequence(UDialogSequence* InDialogSequence);
	void EnsureSequencerCreated();
	void CloseSequencerForUndoRedo();
	void EnsureDialogCameraCutSection();
	void UpdateTrackModelRule();
	void UpdateSectionRule();
	void OnSequencerSelectionChangedObjectGuids(TArray<FGuid> Guids);
	void ApplyDefaultCameraSetting();

	// Returns true when the binding exists and has at least one valid 3D transform section.
	bool HasBindingWithValid3DTransformSection(const FGuid& InBindingId) const;

	bool ApplyCameraPreset(class UDialogSequenceShot* InSequenceShot, int32 InFrameNumber = -1, bool bRandomizeAngle = false);
	void AddKeyFromCameraPreset(int32 InFrameNumber = -1, EMovieSceneKeyInterpolation KeyInterpolationType = EMovieSceneKeyInterpolation::Constant);

	/** Called whenever sequencer has received focus */
	void OnSequencerReceivedFocus();

	void OnSequencerGlobalTimeChanged();
	void OnSequencerMovieSceneDataChanged(EMovieSceneDataChangeType ChangeType);
	void RefreshSequencerCameraLock();
	void UnbindSequencerDelegates();
	void ApplyViewportCameraMode();

	void SelectActor(AActor* ActorToSelect);

	/** Called whenever sequencer in initializing tool menu context */
	void OnInitToolMenuContext(FToolMenuContext& MenuContext);

	UWorld* GetPreviewWorld();

	//DialogDefinitions
	void UpdateDialogStage();
	void DestroyDialogStage();
	UDialogSequenceSlot* GetDialogSlotKey(AActor* InActor);
	//Get the slot which is being used as a template for the opened dialogset
	UDialogSequenceSlot* GetTemplateSlot(AActor* InActor);
	void RetrieveDialogSlotActor(UDialogSequenceSlot* InSlot, AActor*& OutActor, bool bIsCamera = false);
	void RetrieveLightSlotActor(class UDialogSequenceSlot_Light* InLightSlot, AActor*& OutActor);
	void UpdateLightSlotProperty(class UDialogSequenceSlot_Light* InLightSlot, AActor* InActor);
	void InitializeLightActor(class AActor* InLightActor);
	void CreateDialogTrackFromSlot(UDialogSequenceSlot* InSlot, bool bIsCamera = false);
	TArray<AActor*> GetDialogSlotActors() const;

	AActor* GetDialogDefinitionActor(UDialogDefinition* InDialogDefinition);
	void SelectDialogDefinitionActor(UDialogDefinition* InDialogDefinition);
	void SelectDialogDefinition(UDialogDefinition* InDialogDefinition);
	void SelectDialogSlotActor(int32 index);
	void SelectLightSlotActor(int32 index);
	UDialogDefinition* FindDialogDefinitionByActor(const AActor* InActor) const;

	void OnDialogDefinitionAdded(UDialogDefinition* InDialogDefinition);
	void OnDialogDefinitionRemoved(UDialogDefinition* InDialogDefinition);

	void ResolveDialogBoundObjects();

	//scene viewport
	void SetViewportWorldMode(EDialogViewportWorldMode NewMode);
	bool IsViewportWorldMode(EDialogViewportWorldMode Mode) const { return ViewportWorldMode == Mode; }

	void SetViewportCameraMode(EDialogViewportCameraMode NewMode);
	bool IsViewportCameraMode(EDialogViewportCameraMode Mode) const { return ViewportCameraMode == Mode; }

	void SetDetailsObject(UObject* InObject);

	//Getter
	TSharedPtr<SDialogDefinitions> GetDialogDefinitionsWidget() const { return DialogDefinitionsWidget; }
	AActor* GetSequencePivot() { return SequencePivot.Get(); };
	class UDialogStage* GetDialogStageTemplate() { return DialogStageTemplate.Get(); };
	AActor* GetDialogCamera() { return DialogCamera.Get(); }

	FGraphAppearanceInfo GetGraphAppearance() const;
	bool InEditingMode(bool bGraphIsEditable) const;

	static bool IsPIESimulating();
	static bool IsPIENotSimulating();


	//New class Functions
	void HandleNewClassPicked(UClass* InClass) const;
	void CreateNewDialogStageTemplate(UDialogStage* InDialogStage);
private:
	/** Helper to move focused graph when clicking on graph breadcrumb */
	void OnChangeBreadCrumbGraph(class UEdGraph* InGraph);

	/** A callback every time graph changed */
	void OnGraphChanged(const FEdGraphEditAction& Action);

private:
	//Sequencer toolbar
	void ExtendSequencerToolbar(FToolBarBuilder& InToolbarBuilder);
	void OpenGenerateSequenceCameraDialog();
	void GenerateSequenceCameraFromDialogSections(const class UDialogGenerateCameraSettings* InSettings);
	bool CanGenerateSequenceCameraFromDialogSections() const;

protected:
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_DialogStageSettings(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_EditorSettings(const FSpawnTabArgs& Args); 
	TSharedRef<SDockTab> SpawnTab_DialogDefinitions(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Sequencer(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_SequencerViewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_CameraPresets(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_CurveEditor(const FSpawnTabArgs& Args);
	
	void CreateInternalWidgets();

	// Create new graph editor widget for the supplied document container
	virtual TSharedRef<SGraphEditor> CreateGraphEditorWidget(TSharedRef<class FTabInfo> InTabInfo, class UEdGraph* InGraph);

	
	void BindCommands();


	void CreateCommandList();
	
	
	TSharedPtr<SGraphEditor> GetCurrGraphEditor() const;

	FGraphPanelSelectionSet GetSelectedNodes() const;

	void RebuildDialogBuilderGraphPages();

	// Delegates for graph editor commands
	void SelectAllNodes();
	bool CanSelectAllNodes();
	void DeleteSelectedNodes();
	bool CanDeleteNodes();
	void DeleteSelectedDuplicatableNodes();
	void CutSelectedNodes();
	bool CanCutNodes();
	void CopySelectedNodes();
	bool CanCopyNodes();
	void PasteNodes();
	virtual void PasteNodesHere(class UEdGraph* DestinationGraph, const FVector2D& Location) override;

	virtual bool CanPasteNodes() const override;
	void DuplicateNodes();
	bool CanDuplicateNodes();


	void CreateNewDialogDecorator();
	bool CanCreateDialogDecorator() const;

	void CreateNewDialogEvent();
	bool CanCreateDialogEvent() const;

	void CreateNewDialogCameraShot();
	bool CanCreateDialogCameraShot() const;
	
	void OpenDialogSetting();
	bool CanOpenDialogSetting() const;

	void OnRenameNode();
	bool CanRenameNodes() const;

	bool CanCreateComment() const;
	void OnCreateComment();





	//////////////////////////////////////////////////////////////////////////
	// graph editor event
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	void OnNodeDoubleClicked(UEdGraphNode* Node);
	
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);

	void OnPackageMarkedDirty(UPackage* ModifiedPackage, bool bWasDirty);

#if ENGINE_MAJOR_VERSION < 5
	void OnPackageSaved(const FString& PackageFileName, UObject* Outer);
#else // #if ENGINE_MAJOR_VERSION < 5
	void OnPackageSavedWithContext(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext ObjectSaveContext);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5

protected:
	virtual void FixupPastedNodes(const TSet<UEdGraphNode*>& NewPastedGraphNodes, const TMap<FGuid/*New*/, FGuid/*Old*/>& NewToOldNodeMapping);

public:

	TSharedPtr<FDocumentTracker> DocumentManager;
	TSharedPtr<class IDetailsView> PropertyWidget;

	TSharedPtr<class IDetailsView> DialogStageWidget;

	/** Currently focused Editor Graph*/
	UEdGraph* FocusedEdGraph;

	/** Currently focused slate graph editor */
	TSharedPtr<SGraphEditor> CurrentGraphWidget;

	/** determine if node should be checked for new unique ID*/
	bool ShouldGetNewID = false;
protected:
	/** Factory that spawns graph editors; used to look up all tabs spawned by it. */
	TWeakPtr<FDocumentTabFactory> DialogEditorTabFactoryPtr;

	UDialogBuilderGraph* EditingDialogGraph;


	//Toolbar
	TSharedPtr<class FDialogBuilderEditorToolbar> ToolbarBuilder;

	/** Handle to the registered OnPackageSave delegate */
	FDelegateHandle OnPackageSavedDelegateHandle;
	FDelegateHandle TransactionStateChangedHandle;

	TSharedPtr<class IDetailsView> EditorSettingsWidget;
	TSharedPtr<class SDialogDefinitions> DialogDefinitionsWidget;
	TSharedPtr<class SDialogStageManager> DialogStageManagerWidget;
	TSharedPtr<class SDialogCameraPresets> DialogCameraPresetsWidget;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

	//Viewport
	EDialogViewportWorldMode ViewportWorldMode = EDialogViewportWorldMode::PreviewScene;
	EDialogViewportCameraMode ViewportCameraMode = EDialogViewportCameraMode::Perspective;

	FPreviewScene PreviewScene;
	TSharedPtr<SDialogPreviewViewport> DialogViewportWidget;

	
	//sequencer
	TWeakObjectPtr<class UDialogBuilderNode_DialogSequence> CurrentSequenceNode;
	TWeakObjectPtr<class UDialogStage> DialogStageTemplate;
	TSharedPtr<ISequencer> Sequencer;
	TWeakObjectPtr<AActor> SequencePivot;
	TMap<UDialogSequenceSlot*, TWeakObjectPtr<AActor>> DialogSlotActors;
	TWeakObjectPtr<AActor> DialogCamera;

	//sequencer
	FDelegateHandle SequencerGlobalTimeChangedHandle;
	FDelegateHandle SequencerMovieSceneDataChangedHandle;
	TWeakObjectPtr<AActor> LastSequencerCameraCutActor;

	bool bPendingRefreshDialogEditor = false;
	bool bIsRefreshDialogEditorInProgress = false;
	bool bIsClosing = false;

	/** MovieScene for displaying this dialog sequence in timeline. */
	TObjectPtr<UDialogSequence> EditingDialogSequence = nullptr;

	/** Instance of a class used for managing the playback context for a level sequence. */
	TSharedPtr<class FDialogSequencePlaybackContext> PlaybackContext;

public:
	/** Modes in mode switcher */
	static const FName DialogEditorMode;
	static const FName DialogSequencerMode;

	static FText DialogEditorModeText;
	static FText DialogSequencerModeText;

};
