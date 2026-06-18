#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"

class FPreviewScene;
class FEditorViewportClient;
class FDialogBuilderViewportClient;
class FDialogBuilderEditor;

class SDialogPreviewViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SDialogPreviewViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FDialogBuilderEditor> InEditor);

	void SetUseLevelWorld(bool bInUseLevelWorld);

	void AddDialogOverlayWidget(UUserWidget* InWidget);
	void RemoveDialogOverlayWidget();

	void AddOverlayViewportMenu();
	void RemoveOverlayViewportMenu();

	/**
	 * Destructor.
	 */
	virtual ~SDialogPreviewViewport();
	
	/**
	 * Request a refresh of the preview scene/world. Will recreate actors as needed.
	 *
	 * @param bResetCamera If true, the camera will be reset to its default position based on the preview.
	 * @param bRefreshNow If true, the preview will be refreshed immediately. Otherwise, it will be deferred until the next tick (default behavior).
	 */
	void RequestRefresh(bool bResetCamera = false, bool bRefreshNow = false);

	virtual UWorld* GetWorld() const override;
	const FDialogBuilderViewportClient& GetDialogViewportClient() const
	{
		return *DialogViewportClient;
	}

	FDialogBuilderViewportClient* GetDialogViewportClientPtr() const
	{
		return DialogViewportClient.Get();
	}

	/** Called to select the currently locked actor */
	void OnSelectLockedActor();

	/**
	 * @return true if the currently locked actor is selectable
	 */
	bool CanExecuteSelectLockedActor() const;

	/**
	 * @return true if the viewport is locked to selected actor
	 */
	bool IsSelectedActorLocked() const;

	/**
	 * @return true if the actor is locked to the viewport
	 */
	bool IsActorLocked(const TWeakObjectPtr<AActor> Actor) const;

	/**
	 * @return true if an actor is locked to the viewport
	 */
	bool IsAnyActorLocked() const;


	/** Called to clear the current actor lock */
	void OnActorUnlock();

	/**
	 * @return true if clearing the current actor lock is a valid input
	 */
	bool CanExecuteActorUnlock() const;

	/** Called to lock the viewport to the currently selected actor */
	void OnActorLockSelected();

	/**
	 * @return true if clearing the setting the actor lock to the selected actor is a valid input
	 */
	bool CanExecuteActorLockSelected() const;

	/**
	 * Toggles enabling the exact camera view when locking a viewport to a camera
	 */
	void ToggleActorPilotCameraView();

	/**
	 * Check whether locked camera view is enabled
	 */

	bool IsLockedCameraViewEnabled() const;
	/** Called to lock/unlock the actor from the viewport's context menu */
	void OnActorLockToggleFromMenu(AActor* Actor);

	/** Called to unlock the actor from the viewport's context menu */
	void OnActorLockToggleFromMenu();

	/** Called when Preview Selected Cameras preference is changed.*/
	void OnPreviewSelectedCamerasChange();

	/**
	 * Called when game view should be toggled
	 */
	void ToggleGameView();

	/**
	 * @return true if we can toggle game view
	 */
	bool CanToggleGameView() const;

	/**
	 * @return true if we are in game view
	 */
	bool IsInGameView() const;

	/**
	 * Reset Camera setting after unlock actor
	 */
	void ResetCameraSetting();

	TSharedPtr<FDialogBuilderEditor> GetDialogEditor() const
	{
		return DialogEditor.Pin();
	}

public:
	UTexture2D* CaptureViewportThumbnail() const;

protected:

	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> BuildViewportToolbar() override;
	virtual void BindCommands() override;
	/**
	 * Focuses the viewport on the currently selected components
	 */
	virtual void OnFocusViewportToSelection() override;

	void LockActorInternal(AActor* NewActorToLock);

private:
	/** One-off active timer to update the preview */
	EActiveTimerReturnType DeferredUpdatePreview(double InCurrentTime, float InDeltaTime, bool bResetCamera);

private:
	TWeakPtr<FDialogBuilderEditor> DialogEditor;
	TWeakPtr<class FAssetEditorToolkit> AssetEditorToolkitPtr;
	
	TSharedPtr<SWidget> DialogOverlayWidget;
	TSharedPtr<SWidget> ViewportMenuOverlayWidget;

	TSharedPtr<class FDialogBuilderViewportClient> DialogViewportClient;

	bool bUseLevelWorld = false;

	/** Whether the active timer (for updating the preview) is registered */
	bool bIsActiveTimerRegistered;

	/** Handle to the registered OnPreviewFeatureLevelChanged delegate. */
	FDelegateHandle PreviewFeatureLevelChangedHandle;

	/**
	 * Used to store last perspective camera transform before piloting (Actor Lock)
	 * Once piloting ends, this transform is re-applied to the camera.
	 */
	FViewportCameraTransform CachedPerspectiveCameraTransform;


public:
	static bool GetCameraInformationFromActor(AActor* Actor, FMinimalViewInfo& out_CameraInfo);

	static bool CanGetCameraInformationFromActor(AActor* Actor);
};