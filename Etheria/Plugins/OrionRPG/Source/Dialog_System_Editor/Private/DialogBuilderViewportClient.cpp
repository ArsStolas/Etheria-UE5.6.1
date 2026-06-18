#include "DialogBuilderViewportClient.h"

#include "DialogBuilderEditor.h"
#include "DialogStage.h"
#include "Camera/CameraActor.h"
#include "DialogBuilderSetting.h"

#include "AdvancedPreviewScene.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "SNameComboBox.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"
#include "ToolMenus.h"
#include "Viewports.h"
#include "EditorViewportSelectability.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#include "HitProxies.h"
#include "ComponentVisualizer.h"
#include "Styling/AppStyle.h"
#include "SnappingUtils.h"
#include "EditorModeManager.h"
#include "PreviewScene.h"
#include "Engine/RendererSettings.h"
#include "Elements/Component/ComponentElementLevelEditorViewportInteractionCustomization.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EditorModeManager.h"
#include "UnrealWidget.h"
#include "ScopedTransaction.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "AdvancedPreviewSceneMenus.h"
#include "ViewportToolbar/UnrealEdViewportToolbar.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "DialogBuilderViewportClient"

TMap< TObjectKey< AActor >, TWeakObjectPtr< UActorComponent > > FDialogBuilderViewportClient::ViewComponentForActorCache;

FDialogBuilderViewportClient::FDialogBuilderViewportClient(
	FPreviewScene* InPreviewScene,
	const TSharedRef<SEditorViewport>& InViewport,
	const TSharedRef<FDialogBuilderEditor>& InEditor,
	bool bUseLevelWorld)
	: FEditorViewportClient(nullptr, InPreviewScene, InViewport)
	, bLockedCameraView(true)
	, ScopedTransaction(nullptr)
	, Editor(InEditor)
	, WidgetMode(UE::Widget::WM_Translate)
	, WidgetCoordSystem(COORD_Local)
	, CurrentPreviewScene(InPreviewScene)
	, bEditorCameraCut(false)
	, bWasEditorCameraCut(false)
	, ViewportWidgetAnchor(InViewport)
{
	bAlwaysShowModeWidgetAfterSelectionChanges = true;
	ViewportType = LVT_Perspective;
	SetViewLocation(FVector(-250.0, 0.0, 125.0));
	SetViewRotation(FRotator(0.0, 0.0, 0.0));
	SetRealtime(true);
	bSetListenerPosition = false;
	bAllowCinematicControl = true;
	check(Widget);
	Widget->SetSnapEnabled(true);

	// Set if the grid will be drawn
	DrawHelper.bDrawGrid = GetDefault<UDialogBuilderSetting>()->bShowGrid;

	EngineShowFlags.SetSelectionOutline(true);
	EngineShowFlags.SetCompositeEditorPrimitives(true);

	bShowWidget = true;

	if (CurrentPreviewScene && CurrentPreviewScene->GetWorld())
	{
		CurrentPreviewScene->GetWorld()->SetBegunPlay(false);
	}

	if (GUnrealEd)
	{
		CurrentPreviewScene->SetSkyCubemap(GUnrealEd->GetThumbnailManager()->AmbientCubemap);
	}

	SetUseLevelWorld(bUseLevelWorld);

	
}

FDialogBuilderViewportClient::~FDialogBuilderViewportClient()
{
	
	// Ensure that an in-progress transaction is ended
	EndTransaction();
}

void FDialogBuilderViewportClient::SetUseLevelWorld(bool bInUseLevelWorld)
{
	bUseLevelWorld = bInUseLevelWorld;	
	if (bInUseLevelWorld)
	{
		PreviewScene = nullptr;
	}
	else
	{
		PreviewScene = CurrentPreviewScene;
	}

	RefreshRenderFeature();
}

void FDialogBuilderViewportClient::RefreshRenderFeature()
{
	if (bUseLevelWorld)
	{
		//fetch renderer settings
		const URendererSettings* RendererSettings = GetDefault<URendererSettings>();
		const bool bLumenGIEnabled = RendererSettings->DynamicGlobalIllumination == EDynamicGlobalIlluminationMethod::Lumen;
		const bool bLumenReflectionsEnabled = RendererSettings->Reflections == EReflectionMethod::Lumen;
		const bool bMegaLightsEnabled = RendererSettings->bEnableMegaLights;

		{
			EngineShowFlags.EnableAdvancedFeatures();
			EngineShowFlags.SetLumenGlobalIllumination(bLumenGIEnabled);
			EngineShowFlags.SetLumenReflections(bLumenReflectionsEnabled);
			EngineShowFlags.SetMegaLights(bMegaLightsEnabled);
		}
	}
	else
	{
		EngineShowFlags.DisableAdvancedFeatures();
		EngineShowFlags.SetLensFlares(true);
		EngineShowFlags.SetColorGrading(true);
		EngineShowFlags.SetCameraImperfections(true);
		EngineShowFlags.SetDepthOfField(true);
		EngineShowFlags.SetVignette(true);
		EngineShowFlags.SetGrain(true);
		EngineShowFlags.SetSeparateTranslucency(true);
		EngineShowFlags.SetScreenSpaceReflections(true);
		EngineShowFlags.SetTemporalAA(true);
		EngineShowFlags.SetAmbientOcclusion(true);

		EngineShowFlags.SetIndirectLightingCache(true);

		EngineShowFlags.SetLightShafts(true);
		EngineShowFlags.SetPostProcessMaterial(true);
		EngineShowFlags.SetDistanceFieldAO(true);

	}

	EngineShowFlags.SetSelectionOutline(true);
}

void FDialogBuilderViewportClient::BeginTransaction(const FText& Description)
{
	if (!ScopedTransaction)
	{
		if (!ScopedTransaction)
		{
			ScopedTransaction = new FScopedTransaction(Description);
		}
	}

}

void FDialogBuilderViewportClient::EndTransaction()
{
	if (ScopedTransaction)
	{
		delete ScopedTransaction;
		ScopedTransaction = nullptr;
	}
}

void FDialogBuilderViewportClient::Tick(float DeltaSeconds)
{
	if (bWasEditorCameraCut && bEditorCameraCut)
	{
		bEditorCameraCut = false;
	}
	bWasEditorCameraCut = bEditorCameraCut;

	// Gives FindViewComponentForActor a chance to refresh once every Tick.
	ViewComponentForActorCache.Reset();

	FEditorViewportClient::Tick(DeltaSeconds);



	// Tick the preview scene world.
	if (!GIntraFrameDebuggingGameThread && !bUseLevelWorld)
	{

		// Allow full tick only if preview simulation is enabled and we're not currently in an active SIE or PIE session
		if (GEditor->PlayWorld == NULL && !GEditor->bIsSimulatingInEditor)
		{
			PreviewScene->GetWorld()->Tick(IsRealtime() ? LEVELTICK_All : LEVELTICK_TimeOnly, DeltaSeconds);
		}
		else
		{
			PreviewScene->GetWorld()->Tick(IsRealtime() ? LEVELTICK_ViewportsOnly : LEVELTICK_TimeOnly, DeltaSeconds);
		}
	}

	UpdateViewForLockedActor(DeltaSeconds);

}


void FDialogBuilderViewportClient::UpdateViewForLockedActor(float DeltaTime)
{
	// We can't be locked to a cinematic actor if this viewport doesn't allow cinematic control
	if (!bAllowCinematicControl && ActorLocks.CinematicActorLock.HasValidLockedActor())
	{
		ActorLocks.CinematicActorLock = FLevelViewportActorLock::None;
	}

	bUseControllingActorViewInfo = false;
	ControllingActorViewInfo = FMinimalViewInfo();
	ControllingActorAspectRatioAxisConstraint.Reset();
	ControllingActorExtraPostProcessBlends.Empty();
	ControllingActorExtraPostProcessBlendWeights.Empty();

	const FLevelViewportActorLock& ActiveLock = ActorLocks.GetLock();
	AActor* Actor = ActiveLock.GetLockedActor();
	if (Actor != NULL)
	{
		// Check if the viewport is transitioning
		FViewportCameraTransform& ViewTransform = GetViewTransform();
		if (!ViewTransform.IsPlaying())
		{
			// Update transform
			bool bGotCameraView = false;
			if (bLockedCameraView)
			{
				// If this is a camera actor, then inherit some other settings
				UActorComponent* const ViewComponent = FindViewComponentForActor(Actor);
				if (ViewComponent != nullptr)
				{
					if (ensure(ViewComponent->GetEditorPreviewInfo(DeltaTime, ControllingActorViewInfo)))
					{
						bUseControllingActorViewInfo = true;
						if (UCameraComponent* CameraComponent = Cast<UCameraComponent>(ViewComponent))
						{
							CameraComponent->GetExtraPostProcessBlends(ControllingActorExtraPostProcessBlends, ControllingActorExtraPostProcessBlendWeights);
						}

						// Axis constraint for aspect ratio
						ControllingActorAspectRatioAxisConstraint = ActiveLock.AspectRatioAxisConstraint;

						// Post processing is handled by OverridePostProcessingSettings
						ViewFOV = ControllingActorViewInfo.FOV;
						AspectRatio = ControllingActorViewInfo.AspectRatio;
						SetViewLocation(ControllingActorViewInfo.Location);
						SetViewRotation(ControllingActorViewInfo.Rotation);
						bGotCameraView = true;
					}
				}
			}
			if (!bGotCameraView)
			{
				if (Actor->GetAttachParentActor() != NULL)
				{
					// Actor is parented, so use the actor to world matrix for translation and rotation information.
					SetViewLocation(Actor->GetActorLocation());
					SetViewRotation(Actor->GetActorRotation());
				}
				else if (Actor->GetRootComponent() != NULL)
				{
					// No attachment, so just use the relative location, so that we don't need to
					// convert from a quaternion, which loses winding information.
					SetViewLocation(Actor->GetRootComponent()->GetRelativeLocation());
					SetViewRotation(Actor->GetRootComponent()->GetRelativeRotation());
				}
			}

			const double DistanceToCurrentLookAt = FVector::Dist(GetViewLocation(), GetLookAtLocation());

			const FQuat CameraOrientation = FQuat::MakeFromEuler(GetViewRotation().Euler());
			FVector Direction = CameraOrientation.RotateVector(FVector(1, 0, 0));

			SetLookAtLocation(GetViewLocation() + Direction * DistanceToCurrentLookAt);
		}
	}
}


bool FDialogBuilderViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
	bool bHandled = GUnrealEd->ComponentVisManager.HandleInputKey(this, EventArgs.Viewport, EventArgs.Key, EventArgs.Event);

	if (!bHandled)
	{
		bHandled = FEditorViewportClient::InputKey(EventArgs);
	}

	return bHandled;
}

void FDialogBuilderViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	const FViewportClick Click(&View, this, Key, Event, HitX, HitY);
	TSharedPtr<FDialogBuilderEditor> DialogEditor = Editor.Pin();



	if (!DialogEditor.IsValid()) return;
	if (Key == EKeys::RightMouseButton && Event == IE_Released)
	{
		SummonContextMenu(HitProxy ? HitProxy->GetElementHandle() : FTypedElementHandle());
		return;
	}

	if (!HitProxy)
	{
		SelectActor(nullptr);
		return;
	}
	if (HitProxy->IsA(HWidgetAxis::StaticGetType()))
	{
		const bool bOldModeWidgets1 = EngineShowFlags.ModeWidgets;
		const bool bOldModeWidgets2 = View.Family->EngineShowFlags.ModeWidgets;

		EngineShowFlags.SetModeWidgets(false);
		FSceneViewFamily* SceneViewFamily = const_cast<FSceneViewFamily*>(View.Family);
		SceneViewFamily->EngineShowFlags.SetModeWidgets(false);
		bool bWasWidgetDragging = Widget->IsDragging();
		Widget->SetDragging(false);

		// Invalidate the hit proxy map so it will be rendered out again when GetHitProxy
		// is called
		Viewport->InvalidateHitProxy();

		// This will actually re-render the viewport's hit proxies!
		HHitProxy* HitProxyWithoutAxisWidgets = Viewport->GetHitProxy(HitX, HitY);
		if (HitProxyWithoutAxisWidgets != NULL && !HitProxyWithoutAxisWidgets->IsA(HWidgetAxis::StaticGetType()))
		{
			// Try this again, but without the widget this time!
			ProcessClick(View, HitProxyWithoutAxisWidgets, Key, Event, HitX, HitY);
		}

		// Undo the evil
		EngineShowFlags.SetModeWidgets(bOldModeWidgets1);
		SceneViewFamily->EngineShowFlags.SetModeWidgets(bOldModeWidgets2);

		Widget->SetDragging(bWasWidgetDragging);

		// Invalidate the hit proxy map again so that it'll be refreshed with the original
		// scene contents if we need it again later.
		Viewport->InvalidateHitProxy();
		return;
	}

	if (GUnrealEd->ComponentVisManager.HandleClick(this, HitProxy, Click))
	{
		// Component Vis Manager handled this click, no need to do anything
		return;
	}


	UObject* ObjectForDetails = nullptr;
	AActor* ActorToSelect = nullptr;

	if (HitProxy->IsA(HActor::StaticGetType()))
	{
		if (!CanSelectActorFromHitProxy(HitProxy))
		{

			SelectActor(ActorToSelect);
			return;
		}
		HActor* ActorProxy = (HActor*)HitProxy;
		if (ActorProxy && ActorProxy->Actor && ActorProxy->PrimComponent)
		{
			ActorToSelect = ActorProxy->Actor;
			ObjectForDetails = ActorToSelect;
		}
		bool bIsCamera = ActorToSelect->IsA(ACameraActor::StaticClass());
		UDialogDefinition* DialogDefinition = DialogEditor->FindDialogDefinitionByActor(ActorToSelect);
		UDialogSequenceSlot* Slot =  DialogEditor->GetTemplateSlot(ActorToSelect);

		
		SelectActor(ActorToSelect);

		if (DialogEditor->GetSequencePivot() == ActorToSelect)
		{
			UDialogStage* InDialogStage = DialogEditor->GetDialogStageTemplate();
			if (InDialogStage)
			{
				DialogEditor->SetDetailsObject(InDialogStage);
			}
			
		}
		else if (bIsCamera)
		{
			DialogEditor->SetDetailsObject(ObjectForDetails);
		}
		else if (Slot)
		{
			DialogEditor->SetDetailsObject(Slot);
		}
		else
		{
			DialogEditor->SetDetailsObject(ObjectForDetails);
		}
		


		Invalidate();
	}
}

void FDialogBuilderViewportClient::SelectActor(AActor* ActorToSelect)
{
	const bool bNoteSelectionChange = true;
	const bool bDeselectBSPSurfs = true;
	const bool bWarnAboutManyActors = false;
	if (GEditor)
	{
		GEditor->SelectNone(bNoteSelectionChange, bDeselectBSPSurfs, bWarnAboutManyActors);
		CachedSelectedActor = ActorToSelect;
		if (ActorToSelect)
		{
			GEditor->SelectActor(ActorToSelect, true, true);
		}
	}
	Invalidate();
}

void FDialogBuilderViewportClient::MouseMove(FViewport* InViewport, int32 InX, int32 InY)
{
	HHitProxy* const HitResult = InViewport ? InViewport->GetHitProxy(InX, InY) : nullptr;
	UpdateHoverFromHitProxy(HitResult);

	return FEditorViewportClient::MouseMove(InViewport, InX, InY);
}

EMouseCursor::Type FDialogBuilderViewportClient::GetCursor(FViewport* InViewport, int32 InX, int32 InY)
{
	if (MouseCursor.IsSet())
	{
		return MouseCursor.GetValue();
	}

	return FEditorViewportClient::GetCursor(InViewport, InX, InY);
}


void FDialogBuilderViewportClient::TrackingStarted(const struct FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge)
{
	if (!bIsManipulating && bIsDraggingWidget)
	{
		HandleBeginTransform();
	}
}

void FDialogBuilderViewportClient::TrackingStopped()
{
	if (bIsManipulating)
	{
		HandleEndTransform();
	}
}

bool FDialogBuilderViewportClient::BeginTransform(const FGizmoState& InState)
{
	return HandleBeginTransform();
}

bool FDialogBuilderViewportClient::EndTransform(const FGizmoState& InState)
{
	return HandleEndTransform();
}

void FDialogBuilderViewportClient::PerspectiveCameraMoved()
{
	// Update the locked actor (if any) from the camera
	MoveLockedActorToCamera();

	// If any other viewports have this actor locked too, we need to update them
	if (GetActiveActorLock().IsValid())
	{
		UpdateLockedActorViewport(GetActiveActorLock().Get(), false);
	}

	// Broadcast 'camera moved' delegate
	FEditorDelegates::OnEditorCameraMoved.Broadcast(GetViewLocation(), GetViewRotation(), ViewportType, ViewIndex);
}

void FDialogBuilderViewportClient::BeginCameraMovement(bool bHasMovement)
{
	const bool bIsUsingLegacyMovementNotify = GetDefault<ULevelEditorViewportSettings>()->bUseLegacyCameraMovementNotifications;
	// If there's new movement broadcast it
	if (bHasMovement)
	{
		if (!bIsCameraMoving)
		{
			AActor* ActorLock = GetActiveActorLock().Get();
			if (!bIsCameraMovingOnTick && ActorLock)
			{
				GEditor->BroadcastBeginCameraMovement(*ActorLock);
				// consider modification from piloting as relative location changes
				FProperty* TransformProperty = FComponentElementLevelEditorViewportInteractionCustomization::GetEditTransformProperty(UE::Widget::WM_Translate);
				if (TransformProperty)
				{
					// Create edit property event
					FEditPropertyChain PropertyChain;
					PropertyChain.AddHead(TransformProperty);

					// Broadcast Pre Edit change notification, we can't call PreEditChange directly on Actor or ActorComponent from here since it will unregister the components until PostEditChange
					FCoreUObjectDelegates::OnPreObjectPropertyChanged.Broadcast(ActorLock, PropertyChain);
				}
			}

			if (!bIsUsingLegacyMovementNotify)
			{
				if (!HandleEndTransform())
				{
					EndTransaction();
				}
			}
			bIsCameraMoving = true;
		}
	}
	else if (bIsUsingLegacyMovementNotify || !bIsTracking)
	{
		bIsCameraMoving = false;
	}
}

void FDialogBuilderViewportClient::EndCameraMovement()
{
	// If there was movement and it has now stopped, broadcast it
	if (bIsCameraMovingOnTick && !bIsCameraMoving)
	{
		if (AActor* ActorLock = GetActiveActorLock().Get())
		{
			GEditor->BroadcastEndCameraMovement(*ActorLock);
			// Create post edit property change event, consider modification from piloting as relative location changes
			FProperty* TransformProperty = FComponentElementLevelEditorViewportInteractionCustomization::GetEditTransformProperty(UE::Widget::WM_Translate);
			FPropertyChangedEvent PropertyChangedEvent(TransformProperty, EPropertyChangeType::ValueSet);

			// Broadcast Post Edit change notification, we can't call PostEditChangeProperty directly on Actor or ActorComponent from here since it wasn't pair with a proper PreEditChange
			FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(ActorLock, PropertyChangedEvent);
		}
	}
}

void FDialogBuilderViewportClient::RotateViewportType()
{
	SetActorLock(nullptr);
	UpdateViewForLockedActor();

	FEditorViewportClient::RotateViewportType();
}

void FDialogBuilderViewportClient::OverridePostProcessSettings(FSceneView& View)
{
	const UCameraComponent* CameraComponent = GetCameraComponentForView();
	if (CameraComponent)
	{
		View.OverridePostProcessSettings(CameraComponent->PostProcessSettings, CameraComponent->PostProcessBlendWeight);
	}
}

bool FDialogBuilderViewportClient::ShouldLockPitch() const
{
	// If we have somehow gotten out of the locked rotation
	if ((GetViewRotation().Pitch < -90.f + KINDA_SMALL_NUMBER || GetViewRotation().Pitch > 90.f - KINDA_SMALL_NUMBER)
		&& FMath::Abs(GetViewRotation().Roll) > (90.f - KINDA_SMALL_NUMBER))
	{
		return false;
	}
	// Else use the standard rules
	return FEditorViewportClient::ShouldLockPitch();
}

void FDialogBuilderViewportClient::SummonContextMenu(const FTypedElementHandle& HitProxyElement)
{
	TSharedPtr<FDialogBuilderEditor> DialogEditor = Editor.Pin();
	if (!DialogEditor.IsValid())
	{
		return;
	}

	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MovePivotHere", "Move Pivot Here"),
		LOCTEXT("MovePivotHere_Tooltip", "Move the dialog pivot to the current viewport camera location."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDialogBuilderViewportClient::MovePivotToViewportCamera))
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ResetCamera", "Reset Camera"),
		LOCTEXT("ResetCamera_Tooltip", "Reset the dialog preview camera."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDialogBuilderViewportClient::ResetCamera))
	);

	TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
	if (!ActiveWindow.IsValid())
	{
		return;
	}

	FSlateApplication::Get().PushMenu(
		ActiveWindow.ToSharedRef(),
		FWidgetPath(),
		MenuBuilder.MakeWidget(),
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));


}
ELevelViewportType FDialogBuilderViewportClient::GetViewportType() const
{
	const UCameraComponent* ActiveCameraComponent = GetCameraComponentForView();

	if (ActiveCameraComponent != NULL)
	{
		return (ActiveCameraComponent->ProjectionMode == ECameraProjectionMode::Perspective) ? LVT_Perspective : LVT_OrthoFreelook;
	}
	else
	{
		return FEditorViewportClient::GetViewportType();
	}
}

void FDialogBuilderViewportClient::SetViewportType(ELevelViewportType InViewportType)
{
	if (InViewportType != LVT_Perspective)
	{
		SetActorLock(nullptr);
		UpdateViewForLockedActor();
	}

	FEditorViewportClient::SetViewportType(InViewportType);
}



void FDialogBuilderViewportClient::MoveLockedActorToCamera()
{
	// If turned on, move any selected actors to the cameras location/rotation
	AActor* ActiveActorLock = GetActiveActorLock().Get();
	if (ActiveActorLock)
	{
		if (!ActiveActorLock->IsLockLocation())
		{
			if (ScopedTransaction != nullptr)
			{
				SnapshotTransactionBuffer(ActiveActorLock);

				USceneComponent* ActiveActorLockComponent = ActiveActorLock->GetRootComponent();
				if (ActiveActorLockComponent && !ActiveActorLockComponent->IsCreatedByConstructionScript())
				{
					SnapshotTransactionBuffer(ActiveActorLockComponent);
				}
			}

			// Need to disable orbit camera before setting actor position so that the viewport camera location is converted back
			ToggleOrbitCamera(false);

			USceneComponent* ActiveActorLockComponent = ActiveActorLock->GetRootComponent();
			TOptional<FRotator> PreviousRotator;
			if (ActiveActorLockComponent)
			{
				PreviousRotator = ActiveActorLockComponent->GetRelativeRotation();
			}

			// If we're locked to a camera then we're reflecting the camera view and not the actor position. We need to reflect that delta when we reposition the piloted actor
			if (bUseControllingActorViewInfo)
			{
				const UActorComponent* ViewComponent = FindViewComponentForActor(ActiveActorLock);
				if (const UCameraComponent* CameraViewComponent = Cast<UCameraComponent>(ViewComponent))
				{
					FTransform AdditiveOffset;
					float AdditiveFOV;
					CameraViewComponent->GetAdditiveOffset(AdditiveOffset, AdditiveFOV);
					const FTransform RelativeTransform = (AdditiveOffset * CameraViewComponent->GetComponentTransform()).Inverse();
					const FTransform DesiredTransform = FTransform(GetViewRotation(), GetViewLocation());
					ActiveActorLock->SetActorTransform(ActiveActorLock->GetActorTransform() * RelativeTransform * DesiredTransform);
				}
				else if (const USceneComponent* SceneViewComponent = Cast<USceneComponent>(ViewComponent))
				{
					const FTransform RelativeTransform = SceneViewComponent->GetComponentTransform().Inverse();
					const FTransform DesiredTransform = FTransform(GetViewRotation(), GetViewLocation());
					ActiveActorLock->SetActorTransform(ActiveActorLock->GetActorTransform() * RelativeTransform * DesiredTransform);
				}
			}
			else
			{
				ActiveActorLock->SetActorLocation(GetViewLocation(), false);
				ActiveActorLock->SetActorRotation(GetViewRotation());
			}

			if (ActiveActorLockComponent)
			{
				const FRotator Rot = PreviousRotator.GetValue();
				FRotator ActorRotWind, ActorRotRem;
				Rot.GetWindingAndRemainder(ActorRotWind, ActorRotRem);
				const FQuat ActorQ = ActorRotRem.Quaternion();
				const FQuat ResultQ = ActiveActorLockComponent->GetRelativeRotation().Quaternion();
				FRotator NewActorRotRem = FRotator(ResultQ);
				ActorRotRem.SetClosestToMe(NewActorRotRem);
				FRotator DeltaRot = NewActorRotRem - ActorRotRem;
				DeltaRot.Normalize();
				ActiveActorLockComponent->SetRelativeRotationExact(Rot + DeltaRot);
			}
		}

		if (ABrush* Brush = Cast<ABrush>(ActiveActorLock))
		{
			Brush->SetNeedRebuild(Brush->GetLevel());
		}

		FScopedLevelDirtied LevelDirtyCallback;
		LevelDirtyCallback.Request();

		RedrawAllViewportsIntoThisScene();
	}
}



void FDialogBuilderViewportClient::UpdateLockedActorViewport(const AActor* InActor, const bool bCheckRealtime)
{
	// If this viewport has the actor locked and we need to update the camera, then do so
	if (IsActorLocked(InActor) && (!bCheckRealtime || IsRealtime()))
	{
		MoveCameraToLockedActor();
	}
}

bool FDialogBuilderViewportClient::HandleBeginTransform()
{
	if (!bIsManipulating)
	{
		// Suspend component modification during each delta step to avoid recording unnecessary overhead into the transaction buffer
		GEditor->DisableDeltaModification(true);

		// Begin transaction
		BeginTransaction(NSLOCTEXT("DialogEd", "ModifyComponents", "Modify Component(s)"));
		bIsManipulating = true;
		return true;
	}

	return false;
}

bool FDialogBuilderViewportClient::HandleEndTransform()
{
	if (bIsManipulating)
	{
		bIsManipulating = false;

		TSharedPtr<FDialogBuilderEditor> DialogEditor = Editor.Pin();
		USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;
		AActor* SelectedActor = SelectedActors ? Cast<AActor>(SelectedActors->GetSelectedObject(0)) : nullptr;
		UDialogSequenceSlot* TemplateSlot = DialogEditor ? DialogEditor->GetTemplateSlot(SelectedActor) : nullptr;
		UDialogSequenceSlot* ActorSlot = DialogEditor ? DialogEditor->GetDialogSlotKey(SelectedActor) : nullptr;

		const USceneComponent* RootComponent = SelectedActor ? SelectedActor->GetRootComponent() : nullptr;

		const FVector Location = RootComponent ? RootComponent->GetRelativeLocation() : FVector::ZeroVector;
		const FRotator Rotation = RootComponent ? RootComponent->GetRelativeRotation() : FRotator::ZeroRotator;

		if (DialogEditor && DialogEditor->GetSequencePivot() == SelectedActor)
		{
			UDialogStage* DialogStageTemplate = DialogEditor->GetDialogStageTemplate();
			if (DialogStageTemplate)
			{
				DialogStageTemplate->Modify();
				DialogStageTemplate->Location = Location;
				DialogStageTemplate->Rotation = Rotation;
			}

			EndTransaction();

			// Restore component delta modification
			GEditor->DisableDeltaModification(false);
			return true;
		}

		bool bApplyToTemplateSlot = true;
		if (DialogEditor && ActorSlot && ActorSlot->ID.IsValid())
		{
			const bool bHasValidBindingAndTransformSection =
				DialogEditor->HasBindingWithValid3DTransformSection(ActorSlot->ID);

			// If binding + 3D transform section exists, do NOT apply to template slot.
			bApplyToTemplateSlot = !bHasValidBindingAndTransformSection;
		}

		if (TemplateSlot && bApplyToTemplateSlot)
		{
			TemplateSlot->Modify();
			TemplateSlot->SlotLocation = Location;
			TemplateSlot->SlotRotation = Rotation;
		}

		EndTransaction();

		// Restore component delta modification
		GEditor->DisableDeltaModification(false);

		return true;
	}

	return false;
}

bool FDialogBuilderViewportClient::InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale)
{
	if (!bIsManipulating || CurrentAxis == EAxisList::None || !GEditor)
	{
		return false;
	}

	if (GUnrealEd->ComponentVisManager.HandleInputDelta(this, InViewport, Drag, Rot, Scale))
	{
		GUnrealEd->RedrawLevelEditingViewports();
		Invalidate();
		return true;
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (!SelectedActors || SelectedActors->Num() == 0)
	{
		return false;
	}

	FVector ModifiedScale = Scale;
	if (!Scale.IsNearlyZero())
	{
		// In both cases, the scale is assumed to be in the 0-100 range and is adjusted to 0-1.
		if (GEditor->GetScaleGridSize() > 0.0f && GEditor->GetGridSize() > 0.0f)
		{
			ModifiedScale *= ((GEditor->GetScaleGridSize() / 100.0f) / GEditor->GetGridSize());
		}
		else
		{
			ModifiedScale /= 100.0f;
		}
	}
	else
	{
		ModifiedScale = FVector::ZeroVector;
	}

	const bool bAltDown = InViewport && (InViewport->KeyState(EKeys::LeftAlt) || InViewport->KeyState(EKeys::RightAlt));
	const bool bShiftDown = InViewport && (InViewport->KeyState(EKeys::LeftShift) || InViewport->KeyState(EKeys::RightShift));
	const bool bControlDown = InViewport && (InViewport->KeyState(EKeys::LeftControl) || InViewport->KeyState(EKeys::RightControl));

	bool bHandled = false;

	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		AActor* SelectedActor = Cast<AActor>(*It);
		if (!IsValid(SelectedActor))
		{
			continue;
		}

		SelectedActor->Modify();

		GEditor->ApplyDeltaToActor(
			SelectedActor,
			true,
			&Drag,
			&Rot,
			&ModifiedScale,
			bAltDown,
			bShiftDown,
			bControlDown);

		bHandled = true;
	}

	if (bHandled)
	{
		GUnrealEd->RedrawLevelEditingViewports();
		Invalidate();
	}

	return bHandled;
}

FVector FDialogBuilderViewportClient::GetWidgetLocation() const
{
	FVector ComponentVisWidgetLocation;
	if (GUnrealEd->ComponentVisManager.IsVisualizingArchetype() &&
		GUnrealEd->ComponentVisManager.GetWidgetLocation(this, ComponentVisWidgetLocation))
	{
		return ComponentVisWidgetLocation;
	}

	if (CachedSelectedActor.IsValid())
	{
		return CachedSelectedActor.Get()->GetActorLocation();
		
	}

	return FVector::ZeroVector;
}

FMatrix FDialogBuilderViewportClient::GetWidgetCoordSystem() const
{
	FMatrix ComponentVisWidgetCoordSystem;
	if (GUnrealEd->ComponentVisManager.IsVisualizingArchetype() &&
		GUnrealEd->ComponentVisManager.GetCustomInputCoordinateSystem(this, ComponentVisWidgetCoordSystem))
	{
		return ComponentVisWidgetCoordSystem;
	}

	FMatrix Matrix = FMatrix::Identity;

	if (GetWidgetCoordSystemSpace() == COORD_Local)
	{
		if (CachedSelectedActor.IsValid())
		{
			Matrix = FQuatRotationMatrix(CachedSelectedActor.Get()->GetActorQuat());
		}
		
	}

	if (!Matrix.Equals(FMatrix::Identity))
	{
		Matrix.RemoveScaling();
	}

	return Matrix;
}


UE::Widget::EWidgetMode FDialogBuilderViewportClient::GetWidgetMode() const
{
	if (GUnrealEd && GUnrealEd->ComponentVisManager.IsActive() && GUnrealEd->ComponentVisManager.IsVisualizingArchetype())
	{
		return WidgetMode;
	}


	if (CachedSelectedActor.IsValid())
	{
		return WidgetMode;
	}

	return UE::Widget::WM_None;
}

ECoordSystem FDialogBuilderViewportClient::GetWidgetCoordSystemSpace() const
{
	return WidgetCoordSystem;
}


void FDialogBuilderViewportClient::SetWidgetMode(UE::Widget::EWidgetMode NewMode)
{
	WidgetMode = NewMode;
}

void FDialogBuilderViewportClient::SetWidgetCoordSystemSpace(ECoordSystem NewCoordSystem)
{
	WidgetCoordSystem = NewCoordSystem;
}


bool FDialogBuilderViewportClient::GetShowGrid()
{
	return GetDefault<UDialogBuilderSetting>()->bShowGrid;
}

void FDialogBuilderViewportClient::ToggleShowGrid()
{
	UDialogBuilderSetting* Settings = GetMutableDefault<UDialogBuilderSetting>();

	bool bShowGrid = Settings->bShowGrid;
	bShowGrid = !bShowGrid;

	DrawHelper.bDrawGrid = bShowGrid;

	Settings->bShowGrid = bShowGrid;
	Settings->PostEditChange();

	Invalidate();
}


void FDialogBuilderViewportClient::UpdateHoverFromHitProxy(HHitProxy* const InHitProxy)
{
	MouseCursor.Reset();
}

bool FDialogBuilderViewportClient::CanSelectActorFromHitProxy(HHitProxy* const InHitProxy)
{
	const TSharedPtr<FDialogBuilderEditor> DialogEditor = Editor.Pin();
	if (!DialogEditor.IsValid()) return false;

	if (InHitProxy == nullptr) return false;

	const HActor* ActorProxy = HitProxyCast<HActor>(InHitProxy);
	if (ActorProxy == nullptr || !IsValid(ActorProxy->Actor))
	{
		return false;
	}

	const AActor* const HitActor = ActorProxy->Actor;

	if (DialogEditor->GetSequencePivot() == HitActor)
	{
		return true;
	}

	// Replace `GetParticipantTrackActors()` with your actual editor accessor.
	for (AActor* ParticipantActor : DialogEditor->GetDialogSlotActors())
	{
		if (ParticipantActor == HitActor)
		{
			return true;
		}
	}

	return false;
}


void FDialogBuilderViewportClient::InvalidatePreview(bool bResetCamera)
{
	TSharedPtr<FDialogBuilderEditor> DialogEditor = Editor.Pin();

	if (!DialogEditor.IsValid()) return;
	
	DialogEditor->UpdateDialogStage();

	Invalidate();

	if (bResetCamera)
	{
		ResetCamera();
	}
}

void FDialogBuilderViewportClient::ResetCamera()
{
	TSharedPtr<FDialogBuilderEditor> DialogEditor = Editor.Pin();
	if (!DialogEditor.IsValid())
	{
		return;
	}

	AActor* SequencePivot = DialogEditor->GetSequencePivot();

	FVector FocusLocation = FVector::ZeroVector;
	FBox CombinedBounds(ForceInit);

	for (AActor* DialogActor : DialogEditor->GetDialogSlotActors())
	{
		if (IsValid(DialogActor))
		{
			CombinedBounds += DialogActor->GetComponentsBoundingBox(true);
		}
	}

	if (IsValid(SequencePivot))
	{
		FocusLocation = SequencePivot->GetActorLocation();
	}
	else if (CombinedBounds.IsValid)
	{
		FocusLocation = CombinedBounds.GetCenter();
	}

	FocusLocation.Z += 75.0f;

	USceneThumbnailInfo* ThumbnailInfo = USceneThumbnailInfo::StaticClass()->GetDefaultObject<USceneThumbnailInfo>();

	const float BoundsRadius = CombinedBounds.IsValid ? CombinedBounds.GetExtent().Length() : 150.0f;
	double OrbitZoom = ThumbnailInfo ? ThumbnailInfo->OrbitZoom : 0.0;

	if (BoundsRadius + OrbitZoom < 0.0)
	{
		OrbitZoom = -BoundsRadius;
	}

	const double TargetDistance = BoundsRadius > 0.0f ? BoundsRadius : 150.0f;
	const FRotator ThumbnailAngle = ThumbnailInfo
		? FRotator(ThumbnailInfo->OrbitPitch, ThumbnailInfo->OrbitYaw, 0.0f)
		: FRotator(-15.0f, -135.0f, 0.0f);

	ToggleOrbitCamera(true);
	SetViewLocationForOrbiting(FocusLocation);
	SetViewLocation(GetViewLocation() + FVector(0.0f, 200.0f, 0.0f));
	SetViewRotation(ThumbnailAngle);

	FBox BoundingBox(ForceInit);
	if (SequencePivot)
	{
		BoundingBox = SequencePivot->GetRootComponent()->Bounds.GetBox();
		float ExpandAmount = 125.0f;
		BoundingBox = BoundingBox.ExpandBy(ExpandAmount);
		if (BoundingBox.IsValid)
		{
			//FocusViewportOnBox(BoundingBox);
		}
	}
	Invalidate();
}

void FDialogBuilderViewportClient::MovePivotToViewportCamera()
{
	TSharedPtr<FDialogBuilderEditor> DialogEditor = Editor.Pin();
	if (!DialogEditor.IsValid())
	{
		return;
	}

	UDialogStage* DialogStageTemplate = DialogEditor->GetDialogStageTemplate();
	if (!DialogStageTemplate)
	{
		return;
	}
	AActor* LockedActor = GetActiveActorLock().Get();

	if (LockedActor)
	{
		FNotificationInfo Info(LOCTEXT("MovePivotRequiresPerspective", "Switch to viewport perspective mode before moving the pivot, or press 1"));
		Info.ExpireDuration = 3.0f;
		Info.bFireAndForget = true;
		Info.bUseLargeFont = false;

		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}


	BeginTransaction(LOCTEXT("MovePivotHereTransaction", "Move Pivot Here"));

	DialogStageTemplate->Modify();
	
	DialogStageTemplate->Location = GetViewLocation();

	EndTransaction();

	DialogEditor->UpdateDialogStage();
	Invalidate();
}

void FDialogBuilderViewportClient::FocusViewportToSelection()
{
	TSharedPtr<FDialogBuilderEditor> DialogEditor = Editor.Pin();

	AActor* PivotActor = DialogEditor ? DialogEditor->GetSequencePivot() : nullptr;
	USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;

	FBox BoundingBox(ForceInit);

	if (SelectedActors->Num() == 0 && PivotActor)
	{
		BoundingBox = PivotActor->GetRootComponent()->Bounds.GetBox();
		float ExpandAmount = 125.0f; 
		BoundingBox = BoundingBox.ExpandBy(ExpandAmount);
		if (BoundingBox.IsValid)
		{
			FocusViewportOnBox(BoundingBox);
		}
		return;
	}

	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		if (AActor* SelectedActor = Cast<AActor>(*It))
		{
			BoundingBox += SelectedActor->GetRootComponent()->Bounds.GetBox();
		}
	}

	if (BoundingBox.IsValid)
	{
		FocusViewportOnBox(BoundingBox);
	}
}


UActorComponent* FDialogBuilderViewportClient::FindViewComponentForActor(AActor const* Actor)
{
	UActorComponent* PreviewComponent = nullptr;
	if (Actor)
	{
		const TWeakObjectPtr<UActorComponent>* CachedComponent = ViewComponentForActorCache.Find(Actor);
		if (CachedComponent != nullptr)
		{
			PreviewComponent = CachedComponent->Get();
		}
		else
		{
			TSet<AActor const*> CheckedActors;
			PreviewComponent = FindViewComponentForActor(Actor, CheckedActors);
			ViewComponentForActorCache.Add(Actor, PreviewComponent);
		}
	}

	return PreviewComponent;
}


UActorComponent* FDialogBuilderViewportClient::FindViewComponentForActor(AActor const* Actor, TSet<AActor const*>& CheckedActors)
{
	UActorComponent* PreviewComponent = nullptr;
	if (Actor && !CheckedActors.Contains(Actor))
	{
		CheckedActors.Add(Actor);
		// see if actor has a component with preview capabilities (prioritize camera components)
		const TSet<UActorComponent*>& Comps = Actor->GetComponents();

		// We need to know if any child component with preview info is selected
		bool bFoundSelectedComp = false;

		for (UActorComponent* Comp : Comps)
		{
			FMinimalViewInfo DummyViewInfo;
			if (Comp && Comp->IsActive() && Comp->GetEditorPreviewInfo(/*DeltaTime =*/0.0f, DummyViewInfo))
			{
				if (Comp->IsSelected())
				{
					PreviewComponent = Comp;
					bFoundSelectedComp = true;
					break;
				}
				else if (PreviewComponent)
				{
					UCameraComponent* AsCamComp = Cast<UCameraComponent>(Comp);
					if (AsCamComp != nullptr)
					{
						PreviewComponent = AsCamComp;
					}
					continue;
				}
				PreviewComponent = Comp;
			}
		}

		// No preview if default preview is forbidden and no children selection found
		if (!Actor->IsDefaultPreviewEnabled() && !bFoundSelectedComp)
		{
			return nullptr;
		}

		// now see if any actors are attached to us, directly or indirectly, that have an active camera component we might want to use
		// we will just return the first one.
		if (PreviewComponent == nullptr)
		{
			Actor->ForEachAttachedActors(
				[&](AActor* AttachedActor) -> bool
				{
					UActorComponent* const Comp = FindViewComponentForActor(AttachedActor, CheckedActors);
					if (Comp)
					{
						PreviewComponent = Comp;
						return false; /* stops iteration */
					}

					return true; /* continue iteration */
				}
			);
		}
	}

	return PreviewComponent;
}


void FDialogBuilderViewportClient::MoveCameraToLockedActor()
{
	// Do a slightly shorter version of the code in UpdateViewForLockedActor where we don't care about
	// post-process settings.
	const FLevelViewportActorLock& ActiveLock = ActorLocks.GetLock();
	AActor* Actor = ActiveLock.GetLockedActor();
	if (Actor)
	{
		bool bGotCameraView = false;
		if (bLockedCameraView)
		{
			// If this is a camera actor, then inherit some other settings
			UActorComponent* const ViewComponent = FindViewComponentForActor(Actor);
			if (ViewComponent != nullptr)
			{
				FMinimalViewInfo ViewInfo;
				if (ensure(ViewComponent->GetEditorPreviewInfo(0.f, ViewInfo)))
				{
					ViewFOV = ViewInfo.FOV;
					AspectRatio = ViewInfo.AspectRatio;
					SetViewLocation(ViewInfo.Location);
					SetViewRotation(ViewInfo.Rotation);
					bGotCameraView = true;
				}
			}
		}
		if (!bGotCameraView)
		{
			if (Actor->GetAttachParentActor() != NULL)
			{
				// Actor is parented, so use the actor to world matrix for translation and rotation information.
				SetViewLocation(Actor->GetActorLocation());
				SetViewRotation(Actor->GetActorRotation());
			}
			else if (Actor->GetRootComponent() != NULL)
			{
				// No attachment, so just use the relative location, so that we don't need to
				// convert from a quaternion, which loses winding information.
				SetViewLocation(Actor->GetRootComponent()->GetRelativeLocation());
				SetViewRotation(Actor->GetRootComponent()->GetRelativeRotation());
			}
		}

		Invalidate();
	}
}

bool FDialogBuilderViewportClient::IsActorLocked(const TWeakObjectPtr<const AActor> InActor) const
{
	return (InActor.IsValid() && GetActiveActorLock() == InActor);
}

bool FDialogBuilderViewportClient::IsAnyActorLocked() const
{
	return GetActiveActorLock().IsValid();
}

void FDialogBuilderViewportClient::SetActorLock(AActor* Actor)
{
	// If we had an active lock and are clearing it, also end the transaction for that lock
	if (!Actor)
	{
		AActor* ActiveActorLock = GetActiveActorLock().Get();
		if (ActiveActorLock && !ActiveActorLock->IsLockLocation() && CachedPilotTransform.IsSet())
		{
			FTransform EndPosition = ActiveActorLock->GetTransform();
			ActiveActorLock->SetActorTransform(CachedPilotTransform.GetValue());
			{
				const FScopedTransaction Transaction(LOCTEXT("PilotTransaction", "Pilot Actor"));
				ActiveActorLock->Modify();
				ActiveActorLock->SetActorTransform(EndPosition);
			}

			CachedPilotTransform = TOptional<FTransform>();
		}
	}
	SetActorLock(FLevelViewportActorLock(Actor));
}

void FDialogBuilderViewportClient::SetActorLock(const FLevelViewportActorLock& InActorLock)
{
	if (ActorLocks.ActorLock.LockedActor != InActorLock.LockedActor)
	{
		SetIsCameraCut();
	}
	if (ActorLocks.ActorLock.LockedActor.IsValid())
	{
		PreviousActorLocks.ActorLock = ActorLocks.ActorLock;
	}
	ActorLocks.ActorLock = InActorLock;
}


void FDialogBuilderViewportClient::SetCinematicActorLock(AActor* Actor)
{
	SetCinematicActorLock(FLevelViewportActorLock(Actor));
}

void FDialogBuilderViewportClient::SetCinematicActorLock(const FLevelViewportActorLock& InActorLock)
{
	if (ActorLocks.CinematicActorLock.LockedActor != InActorLock.LockedActor)
	{
		SetIsCameraCut();
	}
	if (ActorLocks.CinematicActorLock.LockedActor.IsValid())
	{
		PreviousActorLocks.CinematicActorLock = ActorLocks.CinematicActorLock;
	}
	ActorLocks.CinematicActorLock = InActorLock;
}


#undef LOCTEXT_NAMESPACE
