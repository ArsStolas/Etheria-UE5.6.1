#include "SDialogPreviewViewport.h"

#include "DialogBuilderEditor.h"
#include "DialogBuilderViewportClient.h"
#include "DialogBuilder_EditorCommands.h"
#include "AdvancedPreviewScene.h"
#include "Types/SlateEnums.h"
#include "Blueprint/UserWidget.h"
#include "Slate/SceneViewport.h"
#include "Editor.h"
#include "Widgets/SOverlay.h"
#include "EditorViewportClient.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "SNameComboBox.h"
#include "ToolMenus.h"
#include "Viewports.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Selection.h"
#include "EditorViewportCommands.h"
#include "EngineUtils.h"
#include "HitProxies.h"
#include "ComponentVisualizer.h"
#include "Styling/AppStyle.h"
#include "EditorModeManager.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "PreviewScene.h"
#include "Engine/Selection.h"
#include "EditorModeManager.h"
#include "IVREditorModule.h"
#include "UnrealEdGlobals.h"
#include "LevelEditor.h"
#include "Editor/UnrealEdEngine.h"
#include "AdvancedPreviewSceneMenus.h"
#include "ViewportToolbar/UnrealEdViewportToolbar.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "DialogEditorSubmenus.h"
#include "Widgets/Input/SCheckBox.h"
#include "Engine/Texture2D.h"
#include "RHI.h"

#define LOCTEXT_NAMESPACE "DialogPreviewViewport"

void SDialogPreviewViewport::Construct(const FArguments& InArgs, TSharedRef<FDialogBuilderEditor> InEditor)
{
	bIsActiveTimerRegistered = false;
	AssetEditorToolkitPtr = InEditor;
	DialogEditor = InEditor;
	bUseLevelWorld = false;

	SEditorViewport::Construct(SEditorViewport::FArguments());

	// Restore last used feature level
	if (DialogViewportClient.IsValid())
	{
		UWorld* World = DialogViewportClient->GetPreviewScene()->GetWorld();
		if (World != nullptr)
		{
			World->ChangeFeatureLevel(GWorld->GetFeatureLevel());
		}
	}

	// Use a delegate to inform the attached world of feature level changes.
	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	PreviewFeatureLevelChangedHandle = Editor->OnPreviewFeatureLevelChanged().AddLambda([this](ERHIFeatureLevel::Type NewFeatureLevel)
		{
			if (DialogViewportClient.IsValid())
			{
				UWorld* World = DialogViewportClient->GetPreviewScene()->GetWorld();
				if (World != nullptr)
				{
					World->ChangeFeatureLevel(NewFeatureLevel);

					// Refresh the preview scene. Don't change the camera.
					RequestRefresh(false);
				}
			}
		});

	// Refresh the preview scene
	RequestRefresh(true);

	AddOverlayViewportMenu();
	ToggleGameView();
}


SDialogPreviewViewport::~SDialogPreviewViewport()
{
	RemoveOverlayViewportMenu();
	RemoveDialogOverlayWidget();

	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	Editor->OnPreviewFeatureLevelChanged().Remove(PreviewFeatureLevelChangedHandle);

	if (DialogViewportClient.IsValid())
	{
		DialogViewportClient->Viewport = NULL;
	}
}

void SDialogPreviewViewport::AddOverlayViewportMenu()
{
	if (!SceneViewport.IsValid())
	{
		return;
	}

	TSharedPtr<FDialogBuilderEditor> EditorPinned = DialogEditor.Pin();
	if (!EditorPinned.IsValid())
	{
		return;
	}

	ViewportMenuOverlayWidget = UE::DialogEditor::CreateViewportCameraModeWidget(DialogEditor);

	TWeakPtr<SWidget> EditorViewportContainer = SceneViewport->GetViewportWidget().Pin()->GetContent();
	if (TSharedPtr<SOverlay> Overlay = StaticCastSharedPtr<SOverlay>(EditorViewportContainer.Pin()))
	{
		Overlay->AddSlot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(FMargin(10.0f, 10.0f, 10.0f, 10.0f))
			[
				ViewportMenuOverlayWidget.ToSharedRef()
			];
	}
}

void SDialogPreviewViewport::RemoveOverlayViewportMenu()
{
	if (!SceneViewport.IsValid() || !ViewportMenuOverlayWidget.IsValid())
	{
		return;
	}

	TWeakPtr<SWidget> EditorViewportContainer = SceneViewport->GetViewportWidget().Pin()->GetContent();
	if (TSharedPtr<SOverlay> Overlay = StaticCastSharedPtr<SOverlay>(EditorViewportContainer.Pin()))
	{
		Overlay->RemoveSlot(ViewportMenuOverlayWidget.ToSharedRef());
	}

	ViewportMenuOverlayWidget.Reset();
}

TSharedRef<FEditorViewportClient> SDialogPreviewViewport::MakeEditorViewportClient()
{
	DialogViewportClient = MakeShared<FDialogBuilderViewportClient>(DialogEditor.Pin()->GetPreviewScene(), SharedThis(this), DialogEditor.Pin().ToSharedRef(), bUseLevelWorld);
	DialogViewportClient->SetRealtime(true);
	DialogViewportClient->bSetListenerPosition = false;

	return DialogViewportClient.ToSharedRef();
}

void SDialogPreviewViewport::SetUseLevelWorld(bool bInUseLevelWorld)
{
	if (bUseLevelWorld == bInUseLevelWorld)
	{
		return;
	}

	bUseLevelWorld = bInUseLevelWorld;

	if (DialogViewportClient.IsValid())
	{
		const TSharedPtr<FDialogBuilderViewportClient> DialogClient =
			StaticCastSharedPtr<FDialogBuilderViewportClient>(DialogViewportClient);

		if (DialogClient.IsValid())
		{
			DialogClient->SetUseLevelWorld(bInUseLevelWorld);
		}
	}
}



void SDialogPreviewViewport::RequestRefresh(bool bResetCamera, bool bRefreshNow)
{
	if (bRefreshNow)
	{
		if (DialogViewportClient.IsValid())
		{
			DialogViewportClient->InvalidatePreview(bResetCamera);
		}
	}
	else
	{
		// Defer the update until the next tick. This way we don't accidentally spawn the preview actor in the middle of a transaction, for example.
		if (!bIsActiveTimerRegistered)
		{
			bIsActiveTimerRegistered = true;
			RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SDialogPreviewViewport::DeferredUpdatePreview, bResetCamera));
		}
	}
}

UWorld* SDialogPreviewViewport::GetWorld() const
{
	return Client->GetWorld();
}


bool SDialogPreviewViewport::IsSelectedActorLocked() const
{
	USelection* ActorSelection = GEditor->GetSelectedActors();
	if (1 == ActorSelection->Num() && IsAnyActorLocked())
	{
		AActor* Actor = CastChecked<AActor>(ActorSelection->GetSelectedObject(0));
		if (DialogViewportClient->GetActiveActorLock().Get() == Actor)
		{
			return true;
		}
	}
	return false;
}

void SDialogPreviewViewport::OnActorLockToggleFromMenu(AActor* Actor)
{
	if (Actor != nullptr)
	{
		const bool bLockNewActor = Actor != DialogViewportClient->GetActiveActorLock().Get();

		// Lock the new actor if it wasn't the same actor that we just unlocked
		if (bLockNewActor)
		{
			// Unlock the previous actor
			OnActorUnlock();

			LockActorInternal(Actor);
		}
	}
}

void SDialogPreviewViewport::OnActorLockToggleFromMenu()
{
	OnActorUnlock();
}

void SDialogPreviewViewport::OnSelectLockedActor()
{
	if (AActor* LockedActor = DialogViewportClient->GetActiveActorLock().Get())
	{
		// Deselect any currently selected actors, then select the locked/piloted actor
		if (GEditor)
		{
			GEditor->SelectNone(true, true, false);
			if (LockedActor)
			{
				GEditor->SelectActor(LockedActor, true, true);
			}

			GEditor->NoteSelectionChange();
		}
	}
}

bool SDialogPreviewViewport::CanExecuteSelectLockedActor() const
{
	if (const AActor* LockedActor = DialogViewportClient->GetActiveActorLock().Get())
	{
		return LockedActor->IsSelectable();
	}

	return false;
}


void SDialogPreviewViewport::LockActorInternal(AActor* NewActorToLock)
{
	if (NewActorToLock != nullptr)
	{
		DialogViewportClient->SetActorLock(NewActorToLock);
		if (DialogViewportClient->IsPerspective() && DialogViewportClient->GetActiveActorLock().IsValid())
		{
			// Store perspective camera transform before piloting
			CachedPerspectiveCameraTransform = DialogViewportClient->GetViewTransform();
			DialogViewportClient->MoveCameraToLockedActor();
		}
	}

	// Make sure the inset preview is closed if we are locking a camera that was already part of the selection set and thus being previewed.
	OnPreviewSelectedCamerasChange();
}


bool SDialogPreviewViewport::IsActorLocked(const TWeakObjectPtr<AActor> Actor) const
{
	return DialogViewportClient->IsActorLocked(Actor);
}

bool SDialogPreviewViewport::IsAnyActorLocked() const
{
	return DialogViewportClient->IsAnyActorLocked();
}

void SDialogPreviewViewport::OnActorUnlock()
{
	if (AActor* LockedActor = DialogViewportClient->GetActiveActorLock().Get())
	{
		// Check to see if the locked actor was previously overriding the camera settings
		if (CanGetCameraInformationFromActor(LockedActor))
		{
			// Reset the settings
			ResetCameraSetting();
		}

		DialogViewportClient->SetActorLock(nullptr);

		// remove roll and pitch from camera when unbinding from actors
		GEditor->RemovePerspectiveViewRotation(true, true, false);

		// Move perspective camera back to pre-piloting transform
		DialogViewportClient->SetViewLocation(CachedPerspectiveCameraTransform.GetLocation());
		DialogViewportClient->SetViewRotation(CachedPerspectiveCameraTransform.GetRotation());
		Invalidate();

		// If we had a camera actor locked, and it was selected, then we should re-show the inset preview
		OnPreviewSelectedCamerasChange();
	}
}

bool SDialogPreviewViewport::CanExecuteActorUnlock() const
{
	return IsAnyActorLocked();
}


void SDialogPreviewViewport::ToggleActorPilotCameraView()
{
	DialogViewportClient->bLockedCameraView = !DialogViewportClient->bLockedCameraView;
}

bool SDialogPreviewViewport::IsLockedCameraViewEnabled() const
{
	return DialogViewportClient->bLockedCameraView;
}

void SDialogPreviewViewport::OnActorLockSelected()
{
	USelection* ActorSelection = GEditor->GetSelectedActors();
	if (1 == ActorSelection->Num())
	{
		AActor* Actor = CastChecked<AActor>(ActorSelection->GetSelectedObject(0));
		LockActorInternal(Actor);
	}
}

bool SDialogPreviewViewport::CanExecuteActorLockSelected() const
{
	USelection* ActorSelection = GEditor->GetSelectedActors();
	if (1 == ActorSelection->Num())
	{
		return true;
	}
	return false;
}

bool SDialogPreviewViewport::GetCameraInformationFromActor(AActor* Actor, FMinimalViewInfo& out_CameraInfo)
{
	//
	//@TODO: CAMERA: Support richer camera interactions in SIE; this may shake out naturally if everything uses camera components though

	bool bFoundCamInfo = false;
	if (UActorComponent* ViewComponent = FDialogBuilderViewportClient::FindViewComponentForActor(Actor))
	{
		bFoundCamInfo = ViewComponent->GetEditorPreviewInfo(/*DeltaTime =*/0.0f, out_CameraInfo);
		ensure(bFoundCamInfo);
	}
	return bFoundCamInfo;
}

bool SDialogPreviewViewport::CanGetCameraInformationFromActor(AActor* Actor)
{
	FMinimalViewInfo CameraInfo;

	return GetCameraInformationFromActor(Actor, /*out*/ CameraInfo);
}

void SDialogPreviewViewport::OnPreviewSelectedCamerasChange()
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const bool bPreviewInDesktopViewport = !IVREditorModule::Get().IsVREditorModeActive();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

		// Check to see if previewing selected cameras is enabled and if we're the active level viewport client.
		/*if (GetDefault<ULevelEditorViewportSettings>()->bPreviewSelectedCameras && GCurrentLevelEditingViewportClient == DialogViewportClient.Get())
		{
			PreviewSelectedCameraActors(bPreviewInDesktopViewport);
		}
		else
		{
			// We're either not the active viewport client or preview selected cameras option is disabled, so remove any existing previewed actors
			PreviewActors(TArray<AActor*>(), bPreviewInDesktopViewport);
		}*/
}

EActiveTimerReturnType SDialogPreviewViewport::DeferredUpdatePreview(double InCurrentTime, float InDeltaTime, bool bResetCamera)
{
	if (DialogViewportClient.IsValid())
	{
		DialogViewportClient->InvalidatePreview(bResetCamera);
	}

	bIsActiveTimerRegistered = false;
	return EActiveTimerReturnType::Stop;
}


void SDialogPreviewViewport::BindCommands()
{
	//CommandList->Append(Editor->GetToolkitCommands());
	SEditorViewport::BindCommands();

	const FDialogBuilder_EditorCommands& Commands = FDialogBuilder_EditorCommands::Get();
	CommandList->MapAction(
		Commands.ShowGrid,
		FExecuteAction::CreateSP(DialogViewportClient.Get(), &FDialogBuilderViewportClient::ToggleShowGrid),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(DialogViewportClient.Get(), &FDialogBuilderViewportClient::GetShowGrid));


	CommandList->MapAction(
		Commands.ToggleGameView,
		FExecuteAction::CreateSP(this, &SDialogPreviewViewport::ToggleGameView),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SDialogPreviewViewport::IsInGameView));

	CommandList->MapAction(
		Commands.SelectPilotedActor,
		FExecuteAction::CreateSP(this, &SDialogPreviewViewport::OnSelectLockedActor),
		FCanExecuteAction::CreateSP(this, &SDialogPreviewViewport::CanExecuteSelectLockedActor)
	);

	CommandList->MapAction(
		Commands.EjectActorPilot,
		FExecuteAction::CreateSP(this, &SDialogPreviewViewport::OnActorUnlock),
		FCanExecuteAction::CreateSP(this, &SDialogPreviewViewport::CanExecuteActorUnlock)
	);

	CommandList->MapAction(
		Commands.PilotSelectedActor,
		FExecuteAction::CreateSP(this, &SDialogPreviewViewport::OnActorLockSelected),
		FCanExecuteAction::CreateSP(this, &SDialogPreviewViewport::CanExecuteActorLockSelected)
	);

	CommandList->MapAction(
		Commands.ToggleActorPilotCameraView,
		FExecuteAction::CreateSP(this, &SDialogPreviewViewport::ToggleActorPilotCameraView),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SDialogPreviewViewport::IsLockedCameraViewEnabled)
	);

	// Re-Map Perspective command so that
	// - Perspective menu entry radial checkbox is not flagged when actors are being piloted (including cameras)
	// - Clicking on the Perspective entry interrupts piloting
	{
		const TWeakPtr<SDialogPreviewViewport> DialogViewportWeak = SharedThis(this);

		CommandList->MapAction(
			FEditorViewportCommands::Get().Perspective,
			FExecuteAction::CreateLambda(
				[DialogViewportWeak]()
				{
					if (const TSharedPtr<::SDialogPreviewViewport>& DialogViewportPinned =
						DialogViewportWeak.Pin())
					{
						// If piloting, stop
						DialogViewportPinned->OnActorUnlock();

						if (const TSharedPtr<FEditorViewportClient>& ViewportClient = DialogViewportPinned->GetViewportClient())
						{
							// Set to perspective
							ViewportClient->SetViewportType(LVT_Perspective);
						}
					}
				}
			),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda(
				[DialogViewportWeak]()
				{
					if (const TSharedPtr<::SDialogPreviewViewport>& DialogViewportPinned =
						DialogViewportWeak.Pin())
					{
						// Don't check action when piloting an actor
						if (DialogViewportPinned->IsAnyActorLocked())
						{
							return false;
						}

						if (const TSharedPtr<FEditorViewportClient>& ViewportClient = DialogViewportPinned->GetViewportClient())
						{
							return ViewportClient->IsPerspective();
						}
					}
					return false;
				}
			)
		);
	}

	const TWeakPtr<SDialogPreviewViewport> DialogViewportWeak = SharedThis(this);

	
}

void SDialogPreviewViewport::OnFocusViewportToSelection()
{
	DialogViewportClient->FocusViewportToSelection();
}

TSharedPtr<SWidget> SDialogPreviewViewport::BuildViewportToolbar()
{
	const FName ViewportToolbarName("DialogSequencer.ViewportToolbar");

	if (!UToolMenus::Get()->IsMenuRegistered(ViewportToolbarName))
	{
		UToolMenu* const ViewportToolbarMenu = UToolMenus::Get()->RegisterMenu(
			ViewportToolbarName,
			NAME_None,
			EMultiBoxType::SlimHorizontalToolBar
		);

		ViewportToolbarMenu->StyleName = "ViewportToolbar";

		// Left section (standard-ish viewport actions)
		{
			FToolMenuSection& LeftSection = ViewportToolbarMenu->AddSection("Left");
			LeftSection.AddEntry(UE::UnrealEd::CreateTransformsSubmenu());
			LeftSection.AddEntry(UE::UnrealEd::CreateSnappingSubmenu());
		}

		// Right section (standard menus)
		{
			FToolMenuSection& RightSection = ViewportToolbarMenu->AddSection("Right");
			RightSection.Alignment = EToolMenuSectionAlign::Last;

			const FName SubmenuName = UToolMenus::JoinMenuPaths(ViewportToolbarName, "Camera");
			RightSection.AddEntry(UE::DialogEditor::CreateToolbarCameraSubmenu());
			UE::DialogEditor::ExtendCameraSubmenu(SubmenuName);

			RightSection.AddEntry(UE::UnrealEd::CreateViewModesSubmenu());

			RightSection.AddEntry(UE::UnrealEd::CreateAssetViewerProfileSubmenu());

			const FName AssetViewerProfileMenuName("DialogEditor.ViewportToolbar.AssetViewerProfile");
			UE::AdvancedPreviewScene::Menus::ExtendAdvancedPreviewSceneSettings(AssetViewerProfileMenuName);
			UE::UnrealEd::ExtendPreviewSceneSettingsWithTabEntry(AssetViewerProfileMenuName);
			
			RightSection.AddEntry(UE::UnrealEd::CreateShowSubmenu(FNewToolMenuDelegate::CreateLambda(
				[](UToolMenu* Submenu) -> void
				{
					FToolMenuSection& UnnamedSection = Submenu->FindOrAddSection(NAME_None);
					UnnamedSection.AddMenuEntry(FDialogBuilder_EditorCommands::Get().ShowGrid);
				})
			));
			
		}
	}

	// Generate Unreal toolbar widget
	TSharedPtr<SWidget> UnrealToolbarWidget;
	{
		FToolMenuContext ViewportToolbarContext;
		ViewportToolbarContext.AppendCommandList(GetCommandList());

		UUnrealEdViewportToolbarContext* const ContextObject =
			UE::UnrealEd::CreateViewportToolbarDefaultContext(SharedThis(this));

		ContextObject->AssetEditorToolkit = AssetEditorToolkitPtr;
		ContextObject->PreviewSettingsTabId = FName("AdvancedPreviewTab");

		ViewportToolbarContext.AddObject(ContextObject);

		if (TSharedPtr<FDialogBuilderEditor> EditorPinned = DialogEditor.Pin())
		{
			EditorPinned->InitToolMenuContext(ViewportToolbarContext);
		}

		UnrealToolbarWidget = UToolMenus::Get()->GenerateWidget(ViewportToolbarName, ViewportToolbarContext);
	}

	// Build your custom right-side widget
	TSharedPtr<SWidget> CustomToolbarWidget = SNullWidget::NullWidget;
	{
		TSharedPtr<FDialogBuilderEditor> EditorPinned = DialogEditor.Pin();
		if (EditorPinned.IsValid())
		{
			FToolBarBuilder ToolbarBuilder(GetCommandList(), FMultiBoxCustomization::None);

			ToolbarBuilder.AddToolBarButton(
				FUIAction(
					FExecuteAction::CreateSP(EditorPinned.Get(), &FDialogBuilderEditor::SetViewportWorldMode, EDialogViewportWorldMode::PreviewScene),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(EditorPinned.Get(), &FDialogBuilderEditor::IsViewportWorldMode, EDialogViewportWorldMode::PreviewScene)
				),
				NAME_None,
				FText::FromString(TEXT("Preview Scene")),
				FText::FromString(TEXT("Render the dialog preview/template scene.")),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"),
				EUserInterfaceActionType::ToggleButton
			);

			ToolbarBuilder.AddToolBarButton(
				FUIAction(
					FExecuteAction::CreateSP(EditorPinned.Get(), &FDialogBuilderEditor::SetViewportWorldMode, EDialogViewportWorldMode::CurrentLevel),
					FCanExecuteAction(),
					FIsActionChecked::CreateSP(EditorPinned.Get(), &FDialogBuilderEditor::IsViewportWorldMode, EDialogViewportWorldMode::CurrentLevel)
				),
				NAME_None,
				FText::FromString(TEXT("Current Level")),
				FText::FromString(TEXT("Render the current editor level world.")),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.WorldProperties.Tab"),
				EUserInterfaceActionType::ToggleButton
			);

			CustomToolbarWidget = ToolbarBuilder.MakeWidget();
		}
	}

	// Combine them: Unreal toolbar left, your toolbar right
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			UnrealToolbarWidget.IsValid() ? UnrealToolbarWidget.ToSharedRef() : SNullWidget::NullWidget
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SSpacer)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		[
			CustomToolbarWidget.IsValid() ? CustomToolbarWidget.ToSharedRef() : SNullWidget::NullWidget
		];
}

void SDialogPreviewViewport::ToggleGameView()
{
	bool bGameViewEnable = !DialogViewportClient->IsInGameView();

	// "Mode Widget" should not automatically be reactivated by selecting an actor after "Game View" is enabled
	DialogViewportClient->bAlwaysShowModeWidgetAfterSelectionChanges = true;// bGameViewEnable ? false : true;

	DialogViewportClient->SetGameView(bGameViewEnable);

	// LevelViewportClient->bShowWidget is set to "false" when entering game mode
	// Need to turn it back to "true" 
	DialogViewportClient->ShowWidget(true);
	DialogViewportClient->EngineShowFlags.SetBillboardSprites(true);
	DialogViewportClient->EngineShowFlags.SetCameras(true);
	DialogViewportClient->EngineShowFlags.SetSelection(true);
	DialogViewportClient->EngineShowFlags.SetSelectionOutline(true);

	DialogViewportClient->RefreshRenderFeature();
}

bool SDialogPreviewViewport::CanToggleGameView() const
{
	return DialogViewportClient->IsPerspective();
}

bool SDialogPreviewViewport::IsInGameView() const
{
	return DialogViewportClient->IsInGameView();
}

void SDialogPreviewViewport::ResetCameraSetting()
{
	DialogViewportClient->ViewFOV = DialogViewportClient->FOVAngle;
}

void SDialogPreviewViewport::AddDialogOverlayWidget(UUserWidget* InWidget)
{
	if (!SceneViewport.IsValid() || !InWidget)
	{
		return;
	}

	// Get viewport size
	FIntPoint ViewportSize = SceneViewport->GetSize();

	// Get DPI scale
	float DPIScale = .5f;
	/*if (FSlateApplication::IsInitialized())
	{
		DPIScale = FSlateApplication::Get().GetApplicationScale();
	}*/

	// Calculate scaled size
	float ScaledWidth = ViewportSize.X / DPIScale;
	float ScaledHeight = ViewportSize.Y / DPIScale;

	// Create constraint canvas for proper positioning
	TSharedRef<SConstraintCanvas> ConstraintCanvas = SNew(SConstraintCanvas);

	TSharedRef<SWidget> WidgetContent = InWidget->TakeWidget();

	// Add widget to constraint canvas with full screen positioning
	ConstraintCanvas->AddSlot()
		.Anchors(FAnchors(0, 0, 1, 1))
		.Offset(FMargin(0, 0, 0, 0))
		.Alignment(FVector2D(0, 0))
		[
			SNew(SBox)
				.WidthOverride(ScaledWidth)
				.HeightOverride(ScaledHeight)
				[
					SNew(SDPIScaler)
						.DPIScale(DPIScale)
						[
							WidgetContent
						]
				]
		];

	DialogOverlayWidget = ConstraintCanvas;

	TWeakPtr<SWidget> EditorViewportContainer = SceneViewport->GetViewportWidget().Pin()->GetContent();
	if (TSharedPtr<SOverlay> Overlay = StaticCastSharedPtr<SOverlay>(EditorViewportContainer.Pin()))
	{
		Overlay->AddSlot()
			[
				DialogOverlayWidget.ToSharedRef()
			];
	}

}

void SDialogPreviewViewport::RemoveDialogOverlayWidget()
{
	if (!SceneViewport.IsValid())
	{
		return;
	}

	TWeakPtr<SWidget> EditorViewportContainer = SceneViewport->GetViewportWidget().Pin()->GetContent();
	if (DialogOverlayWidget.IsValid())
	{
		if (TSharedPtr<SOverlay> Overlay = StaticCastSharedPtr<SOverlay>(EditorViewportContainer.Pin()))
		{
			Overlay->RemoveSlot(DialogOverlayWidget.ToSharedRef());
		}
	}

	EditorViewportContainer.Reset();


	DialogOverlayWidget.Reset();
	

}

UTexture2D* SDialogPreviewViewport::CaptureViewportThumbnail() const
{
	if (!SceneViewport.IsValid())
	{
		return nullptr;
	}

	const FIntPoint Size = SceneViewport->GetSizeXY();
	if (Size.X <= 0 || Size.Y <= 0)
	{
		return nullptr;
	}

	TArray<FColor> Bitmap;
	Bitmap.AddUninitialized(Size.X * Size.Y);

	FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
	ReadFlags.SetLinearToGamma(true);

	if (!SceneViewport->ReadPixels(Bitmap, ReadFlags))
	{
		return nullptr;
	}

	UTexture2D* Thumbnail = UTexture2D::CreateTransient(Size.X, Size.Y, PF_B8G8R8A8);
	if (!Thumbnail)
	{
		return nullptr;
	}

	Thumbnail->SRGB = true;
	Thumbnail->CompressionSettings = TC_Default;
	Thumbnail->MipGenSettings = TMGS_NoMipmaps;
	Thumbnail->NeverStream = true;

	FTexturePlatformData* PlatformData = Thumbnail->GetPlatformData();
	if (!PlatformData || PlatformData->Mips.Num() == 0)
	{
		return nullptr;
	}

	FTexture2DMipMap& Mip = PlatformData->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Bitmap.GetData(), Bitmap.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();

	Thumbnail->UpdateResource();
	return Thumbnail;
}

#undef LOCTEXT_NAMESPACE