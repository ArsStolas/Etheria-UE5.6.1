#pragma once

#include "CoreMinimal.h"
#include "LevelEditorViewport.h"
#include "EditorViewportClient.h"

class FDialogBuilderEditor;
class FPreviewScene;
class SEditorViewport;
class SWidget;

class FDialogBuilderViewportClient final : public FEditorViewportClient, public TSharedFromThis<FDialogBuilderViewportClient>
{
public:
	FDialogBuilderViewportClient(
		FPreviewScene* InPreviewScene,
		const TSharedRef<SEditorViewport>& InViewport,
		const TSharedRef<FDialogBuilderEditor>& InEditor,
		bool bUseLevelWorld);


	/**
	 * Destructor.
	 */
	virtual ~FDialogBuilderViewportClient();

	void SetUseLevelWorld(bool bInUseLevelWorld);
	void RefreshRenderFeature();

	// FEditorViewportClient interface
	virtual void Tick(float DeltaSeconds) override;
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
	virtual void ProcessClick(class FSceneView& View, class HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;
	virtual void MouseMove(FViewport* InViewport, int32 InX, int32 InY) override;
	virtual EMouseCursor::Type GetCursor(FViewport* InViewport, int32 InX, int32 InY) override;
	virtual bool InputWidgetDelta(FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale) override;
	virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
	virtual ECoordSystem GetWidgetCoordSystemSpace() const override;
	virtual void SetWidgetMode(UE::Widget::EWidgetMode NewMode) override;
	virtual void SetWidgetCoordSystemSpace(ECoordSystem NewCoordSystem) override;
	virtual FVector GetWidgetLocation() const override;
	virtual FMatrix GetWidgetCoordSystem() const override;
	virtual void TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge) override;
	virtual void TrackingStopped() override;
	virtual bool BeginTransform(const FGizmoState& InState) override;
	virtual bool EndTransform(const FGizmoState& InState) override;
	virtual void PerspectiveCameraMoved() override;
	virtual void BeginCameraMovement(bool bHasMovement) override;
	virtual void EndCameraMovement() override;
	virtual void SummonContextMenu(const FTypedElementHandle& HitProxyElement = FTypedElementHandle());

	virtual ELevelViewportType GetViewportType() const override;
	virtual void SetViewportType(ELevelViewportType InViewportType) override;
	//virtual void UpdateLinkedOrthoViewports(bool bInvalidate = false) override;
	virtual void RotateViewportType() override;
	virtual void OverridePostProcessSettings(FSceneView& View) override;
	virtual bool ShouldLockPitch() const override;

	/**
	 * Checks to see the viewports locked actor need updating
	 */
	void UpdateLockedActorViewport(const AActor* InActor, const bool bCheckRealtime);

	/**
	 * Moves the locked actor according to the viewport cameras location and rotation
	 */
	void MoveLockedActorToCamera();

	/** Sets a flag for this frame indicating that the camera has been cut, and temporal effects (such as motion blur) should be reset */
	void SetIsCameraCut()
	{
		bEditorCameraCut = true;
		bWasEditorCameraCut = false;
	}

	bool GetIsCameraCut() const
	{
		return bEditorCameraCut;
	}


	/**
	 * Updates or resets view properties such as aspect ratio, FOV, location etc to match that of any actor we are locked to
	 */
	void UpdateViewForLockedActor(float DeltaTime = 0.f);

	/**
	 * Returns true if the grid is currently visible in the viewport
	 */
	bool GetShowGrid();

	/**
	 * Will toggle the grid's visibility in the viewport
	 */
	void ToggleShowGrid();


	/**
	 * Focuses the viewport on the selected components
	 */
	void FocusViewportToSelection();

	/**
	 * Recreates the preview scene and invalidates the owning viewport.
	 *
	 * @param bResetCamera Whether or not to reset the camera after recreating the preview scene.
	 */
	void InvalidatePreview(bool bResetCamera = true);

	/**
	 * Resets the camera position
	 */
	void ResetCamera();


	/**
	 * Moves the dialog pivot to the current viewport camera transform.
	 */
	void MovePivotToViewportCamera();

	/**
	 * Check whether this viewport is locked to the specified actor
	 */
	bool IsLockedToActor(AActor* Actor) const
	{
		return ActorLocks.HasActorLocked(Actor);
	}

	/**
	 * Check whether this viewport is locked to display a cinematic camera, like a Sequencer camera.
	 */
	bool IsLockedToCinematic() const
	{
		return ActorLocks.CinematicActorLock.HasValidLockedActor();
	}
	/**
	 * Access the 'active' actor lock.
	 *
	 * This returns the actor lock (as per GetActorLock) if that is the currently active lock. It is *not* the currently
	 * active lock if there's a valid cinematic lock actor (as per GetCinematicActorLock), since cinematics take
	 * precedence.
	 *
	 * @return  The actor currently locked to the viewport and actively linked to the camera movements.
	 */
	TWeakObjectPtr<AActor> GetActiveActorLock() const
	{
		if (ActorLocks.CinematicActorLock.HasValidLockedActor())
		{
			return TWeakObjectPtr<AActor>();
		}
		return ActorLocks.ActorLock.LockedActor;
	}


	/**
	* Gets the actor lock. This is the actor locked to the viewport via the viewport menus.
	*/
	const FLevelViewportActorLock& GetActorLock() const
	{
		return ActorLocks.ActorLock;
	}

	/**
	 * Gets the actor lock. This is the actor locked to the viewport via the viewport menus.
	 */
	FLevelViewportActorLock& GetActorLock()
	{
		return ActorLocks.ActorLock;
	}

	/**
	 * Get the actor locked to the viewport by cinematic tools like Sequencer.
	 */
	const FLevelViewportActorLock& GetCinematicActorLock() const
	{
		return ActorLocks.CinematicActorLock;
	}

	/**
	 * Get the actor locked to the viewport by cinematic tools like Sequencer.
	 */
	FLevelViewportActorLock& GetCinematicActorLock()
	{
		return ActorLocks.CinematicActorLock;
	}

	/**
	 * Set the actor locked to the viewport by cinematic tools like Sequencer.
	 */
	void SetCinematicActorLock(AActor* Actor);

	/**
	 * Set the actor locked to the viewport by cinematic tools like Sequencer.
	 */
	void SetCinematicActorLock(const FLevelViewportActorLock& InActorLock);

	/**
	 * Gets the previous actor lock. This is the actor locked to the viewport via the viewport menus.
	 */
	const FLevelViewportActorLock& GetPreviousActorLock() const
	{
		return PreviousActorLocks.ActorLock;
	}

	/**
	 * Get the previous actor locked to the viewport by cinematic tools like Sequencer.
	 */
	const FLevelViewportActorLock& GetPreviousCinematicActorLock() const
	{
		return PreviousActorLocks.CinematicActorLock;
	}


	/**
	 * Set the actor lock. This is the actor locked to the viewport via the viewport menus.
	 */
	void SetActorLock(AActor* Actor);

	/**
	 * Set the actor lock. This is the actor locked to the viewport via the viewport menus.
	 */
	void SetActorLock(const FLevelViewportActorLock& InActorLock);


	/**
	 * Check to see if this actor is locked by the viewport
	 */
	bool IsActorLocked(const TWeakObjectPtr<const AActor> InActor) const;

	/**
	 * Check to see if any actor is locked by the viewport
	 */
	bool IsAnyActorLocked() const;

	/**
	 * Moves the viewport camera according to the locked actors location and rotation
	 */
	void MoveCameraToLockedActor();


	/**
	 * Find a view component to use for the specified actor. Prioritizes selected
	 * components first, followed by camera components (then falls through to the first component that implements GetEditorPreviewInfo)
	 */
	static UActorComponent* FindViewComponentForActor(AActor const* Actor);

	void SelectActor(AActor* ActorToSelect);

protected:
	/**
	 * Initiates a transaction.
	 */
	void BeginTransaction(const FText& Description);

	/**
	 * Ends the current transaction, if one exists.
	 */
	void EndTransaction();

	void UpdateHoverFromHitProxy(HHitProxy* const InHitProxy);

	/**
	* Can only select actor from the definitions
	*/
	bool CanSelectActorFromHitProxy(HHitProxy* const InHitProxy);

	/**
	 * Find the camera component that is driving this viewport, in the following order of preference:
	 *		1. Cinematic locked actor
	 *		2. User actor lock (if (bLockedCameraView is true)
	 *
	 * @return  Pointer to a camera component to use for this viewport's view
	 */
	UCameraComponent* GetCameraComponentForView() const
	{
		const FLevelViewportActorLock& ActorLock = ActorLocks.GetLock(bLockedCameraView);
		return Cast<UCameraComponent>(FindViewComponentForActor(ActorLock.GetLockedActor()));
	}


private:
	/** Internal function for public FindViewComponentForActor, which finds a view component to use for the specified actor. */
	static UActorComponent* FindViewComponentForActor(AActor const* Actor, TSet<AActor const*>& CheckedActors);

public:
	/** True if this viewport is to change its view (aspect ratio, post processing, FOV etc) to match that of the currently locked camera, if applicable */
	bool bLockedCameraView;

	/** When enabled, the Unreal transform widget will become visible after an actor is selected, even if it was turned off via a show flag */
	bool bAlwaysShowModeWidgetAfterSelectionChanges;

private:
	/** The current transaction for undo/redo */
	FScopedTransaction* ScopedTransaction;

	bool bUseLevelWorld;

	/** If true then we are manipulating a specific property or component */
	bool bIsManipulating;

	bool HandleBeginTransform();
	bool HandleEndTransform();

	TWeakPtr<FDialogBuilderEditor> Editor;

	UE::Widget::EWidgetMode WidgetMode;
	ECoordSystem WidgetCoordSystem;

	FPreviewScene* CurrentPreviewScene = nullptr;


	/**
	 * When locked to an actor this view will be positioned in the same location and rotation as the actor.
	 * If the actor has a camera component the view will also inherit camera settings such as aspect ratio,
	 * FOV, post processing settings, and the like.
	 *
	 * This structure allows us to keep track of two actor locks: a normal actor lock, and a lock specifically
	 * for cinematic tools like Sequencer. A viewport locked to an actor by cinematics will always take
	 * precedent over any other.
	 */
	struct FActorLockStack
	{
		/** Get the active lock info. Cinematics take precedence. */
		const FLevelViewportActorLock& GetLock(bool bAllowActorLock = true) const
		{
			if (CinematicActorLock.LockedActor.IsValid())
			{
				return CinematicActorLock;
			}
			return bAllowActorLock ? ActorLock : FLevelViewportActorLock::None;
		}

		/** Returns whether the given actor is used as one of our locks. */
		bool HasActorLocked(const AActor* InActor) const
		{
			return CinematicActorLock.LockedActor.Get() == InActor || ActorLock.LockedActor.Get() == InActor;
		}

		FLevelViewportActorLock CinematicActorLock;
		FLevelViewportActorLock ActorLock;
	};
	FActorLockStack ActorLocks;
	FActorLockStack PreviousActorLocks;
	
	/** If true, we switched between two different cameras. Set by cinematics, used by the motion blur to invalidate this frames motion vectors */
	bool bEditorCameraCut;

	/** Stores the previous frame's value of bEditorCameraCut in order to reset it back to false on the next frame */
	bool bWasEditorCameraCut;

	/** Caching for expensive FindViewComponentForActor. Invalidated once per Tick. */
	static TMap<TObjectKey<AActor>, TWeakObjectPtr<UActorComponent>> ViewComponentForActorCache;

	/** Cached transform of pilot actor before transaction, used to allow other transactions while piloting by performing a single end-transaction */
	TOptional<FTransform> CachedPilotTransform;

	TOptional<EMouseCursor::Type> MouseCursor;

	TWeakObjectPtr<class AActor> CachedSelectedActor;

private:
	TWeakPtr<class SEditorViewport> ViewportWidgetAnchor;
};