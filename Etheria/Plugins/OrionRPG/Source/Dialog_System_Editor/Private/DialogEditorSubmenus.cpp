#include "DialogEditorSubmenus.h"

#include "DialogBuilderViewportClient.h"
#include "DialogBuilder_EditorCommands.h"
#include "SDialogPreviewViewport.h"
#include "EngineUtils.h"
#include "Styling/SlateIconFinder.h"
#include "EditorViewportCommands.h"
#include "Engine/SceneCapture.h"
#include "Engine/GameViewportClient.h"
#include "SortHelper.h"
#include "Camera/CameraActor.h"
#include "CineCameraActor.h"
#include "ToolMenu.h"
#include "ToolMenus.h"
#include "UnrealEdGlobals.h"
#include "Editor.h"
#include "ViewportToolbar/UnrealEdViewportToolbar.h"
#include "Engine/Selection.h"
#include "ISettingsModule.h"
#include "Styling/AppStyle.h"
#include "ViewportToolbar/UnrealEdViewportToolbarContext.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "DialogEditorViewportToolbar"

namespace UE::DialogEditor::Private
{
	static const FSlateBrush* GetViewportCameraModeBrush(EDialogViewportCameraMode Mode)
	{
		switch (Mode)
		{
		case EDialogViewportCameraMode::Perspective:
			return FEditorViewportCommands::Get().Perspective->GetIcon().GetIcon();

		case EDialogViewportCameraMode::DialogCameraLock:
			return FSlateIconFinder::FindIconForClass(ACineCameraActor::StaticClass()).GetIcon();

		case EDialogViewportCameraMode::SequencerCameraCuts:
			return FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Cinematics").GetIcon();

		default:
			return FAppStyle::GetBrush("NoBrush");
		}
	}


	void GeneratePlacedCameraMenuEntries(
		FToolMenuSection& InSection, TArray<AActor*> InLookThroughActors, const TSharedPtr<::SDialogPreviewViewport>& InDialogViewport
	)
	{
		// Sort the cameras to make the ordering predictable for users.
		InLookThroughActors.StableSort(
			[](const AActor& Left, const AActor& Right)
			{
				// Do "natural sorting" via SceneOutliner::FNumericStringWrapper to make more sense to humans (also matches
				// the Scene Outliner). This sorts "Camera2" before "Camera10" which a normal lexicographical sort wouldn't.
				SceneOutliner::FNumericStringWrapper LeftWrapper(FString(Left.GetActorLabel()));
				SceneOutliner::FNumericStringWrapper RightWrapper(FString(Right.GetActorLabel()));

				return LeftWrapper < RightWrapper;
			}
		);

		for (AActor* LookThroughActor : InLookThroughActors)
		{
			// Needed for the delegate hookup to work below
			AActor* GenericActor = LookThroughActor;

			FText ActorDisplayName = FText::FromString(LookThroughActor->GetActorLabel());
			FUIAction LookThroughCameraAction(
				FExecuteAction::CreateSP(InDialogViewport.ToSharedRef(), &::SDialogPreviewViewport::OnActorLockToggleFromMenu, GenericActor),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(
					InDialogViewport.ToSharedRef(), &::SDialogPreviewViewport::IsActorLocked, MakeWeakObjectPtr(GenericActor)
				)
			);

			FSlateIcon ActorIcon;

			if (LookThroughActor->IsA<ACameraActor>() || LookThroughActor->IsA<ASceneCapture>())
			{
				ActorIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.CameraComponent");
			}
			else
			{
				ActorIcon = FSlateIconFinder::FindIconForClass(LookThroughActor->GetClass());
			}

			InSection.AddMenuEntry(
				NAME_None,
				ActorDisplayName,
				FText::Format(LOCTEXT("LookThroughCameraActor_ToolTip", "Look through and pilot {0}"), ActorDisplayName),
				ActorIcon,
				LookThroughCameraAction,
				EUserInterfaceActionType::RadioButton
			);
		}
	}

	static void AddCameraActorSelectSection(UToolMenu* InMenu)
	{
		if (!InMenu)
		{
			return;
		}

		UUnrealEdViewportToolbarContext* const ToolbarContext = InMenu->FindContext<UUnrealEdViewportToolbarContext>();
		if (!ToolbarContext)
		{
			return;
		}

		const TSharedPtr<SDialogPreviewViewport> DialogViewport = StaticCastSharedPtr<SDialogPreviewViewport>(ToolbarContext->Viewport.Pin());
		if (!DialogViewport.IsValid())
		{
			return;
		}

		TArray<AActor*> LookThroughActors;

		if (UWorld* World = DialogViewport->GetWorld())
		{
			/*for (TActorIterator<ACameraActor> It(World); It; ++It)
			{
				LookThroughActors.Add(Cast<AActor>(*It));
			}

			for (TActorIterator<ASceneCapture> It(World); It; ++It)
			{
				LookThroughActors.Add(Cast<AActor>(*It));
			}*/
			
			FDialogBuilderEditor* DialogEditor = DialogViewport->GetDialogEditor().Get();
			if (DialogEditor && DialogEditor->GetDialogCamera())
			{
				LookThroughActors.Add(DialogEditor->GetDialogCamera());
			}
		}

		FText CameraActorsHeading = LOCTEXT("CameraActorsHeading", "Cameras");

		FToolMenuInsert InsertPosition("LevelViewportCameraType_Perspective", EToolMenuInsertType::After);

		FToolMenuSection& Section = InMenu->AddSection("CameraActors");
		Section.InsertPosition = InsertPosition;


		// Don't add too many cameras to the top level menu or else it becomes too large
		constexpr uint32 MaxCamerasInTopLevelMenu = 10;
		if (LookThroughActors.Num() > MaxCamerasInTopLevelMenu)
		{
			FToolMenuEntry& Entry = Section.AddSubMenu(
				"CameraActors",
				CameraActorsHeading,
				LOCTEXT("LookThroughPlacedCameras_ToolTip", "Look through and pilot placed cameras"),
				FNewToolMenuDelegate::CreateLambda(
					[LookThroughActors, DialogViewportWeak = DialogViewport.ToWeakPtr()](UToolMenu* InMenu)
					{
						if (TSharedPtr<::SDialogPreviewViewport> DialogViewport = DialogViewportWeak.Pin())
						{
							FToolMenuSection& Section = InMenu->FindOrAddSection(NAME_None);
							UE::DialogEditor::Private::GeneratePlacedCameraMenuEntries(Section, LookThroughActors, DialogViewport);
						}
					}
				)
			);
			Entry.Icon = FSlateIconFinder::FindIconForClass(ACameraActor::StaticClass());
		}
		else if (!LookThroughActors.IsEmpty())
		{
			Section.AddSeparator(NAME_None);
			UE::DialogEditor::Private::GeneratePlacedCameraMenuEntries(Section, LookThroughActors, DialogViewport);
		}

		TWeakObjectPtr<AActor> LockedActorWeak = DialogViewport->GetDialogViewportClient().GetActorLock().LockedActor;

		if (TStrongObjectPtr<AActor> LockedActor = LockedActorWeak.Pin())
		{
			if (!LockedActor->IsA<ACameraActor>() && !LockedActor->IsA<ASceneCapture>())
			{
				UE::DialogEditor::Private::GeneratePlacedCameraMenuEntries(Section, { LockedActor.Get() }, DialogViewport);
			}
		}
	}

	// Can be used to show entries only in perspective view - specialized version of UE::UnrealEd method, for LevelViewport argument
	TAttribute<bool> GetIsPerspectiveAttribute(const TWeakPtr<::SDialogPreviewViewport>& DialogViewportWeak)
	{
		if (TSharedPtr<::SDialogPreviewViewport> DialogViewport = DialogViewportWeak.Pin())
		{
			return UE::UnrealEd::GetIsPerspectiveAttribute(DialogViewport->GetViewportClient());
		}

		return false;
	}

	FToolMenuEntry CreateEjectActorPilotEntry()
	{
		return FToolMenuEntry::InitDynamicEntry(
			"EjectActorPilotDynamicSection",
			FNewToolMenuSectionDelegate::CreateLambda(
				[](FToolMenuSection& InnerSection) -> void
				{
					UUnrealEdViewportToolbarContext* const ToolbarContext = InnerSection.FindContext<UUnrealEdViewportToolbarContext>();

					const TSharedPtr<SDialogPreviewViewport> DialogViewport = ToolbarContext ? StaticCastSharedPtr<SDialogPreviewViewport>(ToolbarContext->Viewport.Pin()) : nullptr;
					if (!DialogViewport.IsValid())
					{
						return;
					}

					FToolUIAction EjectActorPilotAction;

					EjectActorPilotAction.ExecuteAction = FToolMenuExecuteAction::CreateLambda(
						[DialogViewportWeak = DialogViewport.ToWeakPtr()](const FToolMenuContext& Context) -> void
						{
							if (TSharedPtr<::SDialogPreviewViewport> DialogViewport = DialogViewportWeak.Pin())
							{
								DialogViewport->OnActorLockToggleFromMenu();
							}
						}
					);

					EjectActorPilotAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateLambda(
						[DialogViewportWeak = DialogViewport.ToWeakPtr()](const FToolMenuContext& Context)
						{
							if (TSharedPtr<::SDialogPreviewViewport> DialogViewport = DialogViewportWeak.Pin())
							{
								return DialogViewport->IsAnyActorLocked();
							}
							return false;
						}
					);

					// We use this entry to gather its Name, Tooltip and Icon. See comment below as to why we cannot directly use this entry.
					FToolMenuEntry SourceEjectPilotEntry =
						FToolMenuEntry::InitMenuEntry(FDialogBuilder_EditorCommands::Get().EjectActorPilot);

					// We want to use SetShowInToolbarTopLevel to show the Eject entry in the Top Level only when piloting is active.
					// Currently, this will not work with Commands, e.g. AddMenuEntry(FDialogBuilder_EditorCommands::Get().EjectActorPilot).
					// So, we create the entry using FToolMenuEntry::InitMenuEntry, and we create our own Action to handle it.
					FToolMenuEntry EjectPilotActor = FToolMenuEntry::InitMenuEntry(
						"EjectActorPilot",
						LOCTEXT("EjectActorPilotLabel", "Stop Piloting Actor"),
						LOCTEXT(
							"EjectActorPilotTooltip", "Stop piloting an actor with the current viewport. Unlocks the viewport's position and orientation from the actor the viewport is currently piloting."
						),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelViewport.EjectActorPilot"),
						EjectActorPilotAction,
						EUserInterfaceActionType::Button
					);

					const TAttribute<bool> bShownInTopLevel = TAttribute<bool>::CreateLambda(
						[DialogViewportWeak = DialogViewport.ToWeakPtr()]() -> bool
						{
							if (TSharedPtr<::SDialogPreviewViewport> DialogViewport = DialogViewportWeak.Pin())
							{
								return DialogViewport->IsAnyActorLocked();
							}

							return true;
						}
					);

					EjectPilotActor.SetShowInToolbarTopLevel(bShownInTopLevel);

					InnerSection.AddEntry(EjectPilotActor);
				}
			)
		);
	}


	FToolMenuEntry CreatePilotSubmenu(TWeakPtr<::SDialogPreviewViewport> DialogViewportWeak)
	{
		FToolMenuEntry Entry = FToolMenuEntry::InitSubMenu(
			"PilotingSubmenu",
			LOCTEXT("PilotingSubmenu", "Pilot"),
			LOCTEXT("PilotingSubmenu_ToolTip", "Piloting cameras and actors"),
			FNewToolMenuDelegate::CreateLambda(
				[DialogViewportWeak](UToolMenu* InMenu)
				{
					FToolMenuSection& PilotSection = InMenu->FindOrAddSection("Pilot");

					bool bShowPilotSelectedActorEntry = false;

					AActor* SelectedActor = nullptr;
					if (TSharedPtr<::SDialogPreviewViewport> DialogViewport = DialogViewportWeak.Pin())
					{
						TArray<AActor*> SelectedActors;
						GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);

						if (!SelectedActors.IsEmpty() && !DialogViewport->IsSelectedActorLocked())
						{
							SelectedActor = SelectedActors[0];
							const FDialogBuilderViewportClient& ViewportClient = DialogViewport->GetDialogViewportClient();

							bShowPilotSelectedActorEntry = SelectedActor && ViewportClient.IsPerspective()
								&& !ViewportClient.IsLockedToCinematic();
						}
					}

					if (bShowPilotSelectedActorEntry)
					{
						// Pilot Selected Actor Entry
						PilotSection.AddMenuEntry(
							FDialogBuilder_EditorCommands::Get().PilotSelectedActor,
							FText::Format(LOCTEXT("PilotActor", "Pilot '{0}'"), FText::FromString(SelectedActor->GetActorLabel()))
						);
					}

					// Stop Piloting Entry
					PilotSection.AddEntry(UE::DialogEditor::Private::CreateEjectActorPilotEntry());

					// Exact Camera View Entry
					{
						FToolMenuEntry& ToggleCameraView =
							PilotSection.AddMenuEntry(FDialogBuilder_EditorCommands::Get().ToggleActorPilotCameraView);
						ToggleCameraView.Label = LOCTEXT("ToggleCameraViewLabel", "Exact Camera View");
						ToggleCameraView.Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelViewport.ToggleActorPilotCameraView");
						ToggleCameraView.SetShowInToolbarTopLevel(
							TAttribute<bool>::CreateLambda(
								[DialogViewportWeak]()
								{
									if (TSharedPtr<::SDialogPreviewViewport> EditorViewport = DialogViewportWeak.Pin())
									{
										return EditorViewport->IsAnyActorLocked();
									}
									return false;
								}
							)
						);
					}

					PilotSection.AddMenuEntry(FDialogBuilder_EditorCommands::Get().SelectPilotedActor);
				}
			),
			false,
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "DialogViewport.PilotSelectedActor")
		);

		Entry.Visibility = GetIsPerspectiveAttribute(DialogViewportWeak);

		return Entry;
	}

	FText GetCameraSubmenuLabelFromLevelViewport(const TWeakPtr<::SDialogPreviewViewport>& InDialogBuilderViewportClientWeak)
	{
		if (TSharedPtr<::SDialogPreviewViewport> DialogViewport = InDialogBuilderViewportClientWeak.Pin())
		{
			const FDialogBuilderViewportClient& DialogViewportClient = DialogViewport->GetDialogViewportClient();

			if (!DialogViewportClient.IsAnyActorLocked())
			{
				return UnrealEd::GetCameraSubmenuLabelFromViewportType(DialogViewportClient.GetViewportType());
			}
			else if (TStrongObjectPtr<AActor> ActorLock = DialogViewportClient.GetActiveActorLock().Pin())
			{
				return FText::FromString(ActorLock->GetActorNameOrLabel());
			}
		}

		return LOCTEXT("MissingActiveCameraLabel", "No Active Camera");
	}

	FSlateIcon GetCameraSubmenuIconFromLevelViewport(const TWeakPtr<::SDialogPreviewViewport>& InDialogBuilderViewportClientWeak)
	{
		if (TSharedPtr<::SDialogPreviewViewport> DialogViewport = InDialogBuilderViewportClientWeak.Pin())
		{
			const FDialogBuilderViewportClient& DialogViewportClient = DialogViewport->GetDialogViewportClient();
			if (!DialogViewportClient.IsAnyActorLocked())
			{
				const FName IconName =
					UnrealEd::GetCameraSubmenuIconFNameFromViewportType(DialogViewportClient.GetViewportType());
				return FSlateIcon(FAppStyle::GetAppStyleSetName(), IconName);
			}
			else if (TStrongObjectPtr<AActor> LockedActor = DialogViewportClient.GetActorLock().LockedActor.Pin())
			{
				if (!LockedActor->IsA<ACameraActor>() && !LockedActor->IsA<ASceneCapture>())
				{
					return FSlateIconFinder::FindIconForClass(LockedActor->GetClass());
				}
			}
		}

		return FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.CameraComponent");
	}

	void ExtendCameraSpeedSubmenu(FName InCameraSpeedSubmenuName)
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(InCameraSpeedSubmenuName);

		FToolMenuSection& SettingsSection = Menu->AddSection("Settings");
		SettingsSection.AddMenuEntry(
			"OpenPreferences",
			LOCTEXT("OpenCameraSpeedPreferencesLabel", "Show Camera Speed Preferences..."),
			LOCTEXT("OpenCameraSpeedPreferencesTooltip", "Opens the Editor Preferences page for camera speed."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorPreferences.TabIcon"),
			FExecuteAction::CreateLambda([]
				{
					ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
					if (SettingsModule)
					{
						SettingsModule->ShowViewer("Editor", "LevelEditor", "Viewport");
					}
				})
		);
	}

}

namespace UE::DialogEditor
{
	void ExtendCameraSubmenu(FName InCameraOptionsSubmenuName)
	{
		UToolMenu* const Submenu = UToolMenus::Get()->ExtendMenu(InCameraOptionsSubmenuName);
		if (!Submenu)
		{
			return;
		}

		Submenu->AddDynamicSection(
			"DialogEditorCameraExtensionDynamicSection",
			FNewToolMenuDelegate::CreateLambda(
				[](UToolMenu* InDynamicMenu)
				{
					UUnrealEdViewportToolbarContext* const ToolbarContext = InDynamicMenu->FindContext<UUnrealEdViewportToolbarContext>();

					const TSharedPtr<SDialogPreviewViewport> DialogViewport = ToolbarContext ? StaticCastSharedPtr<SDialogPreviewViewport>(ToolbarContext->Viewport.Pin()) : nullptr;
					if (!DialogViewport.IsValid())
					{
						return;
					}
					TWeakPtr<SDialogPreviewViewport> DialogViewportWeak = DialogViewport.ToWeakPtr();
					// Camera Selection elements
					{
						Private::AddCameraActorSelectSection(InDynamicMenu);
					}
					// Movement Menus
					{
						FToolMenuSection& MovementSection = InDynamicMenu->FindOrAddSection("Movement");

						MovementSection.AddEntry(Private::CreatePilotSubmenu(DialogViewportWeak));
						//MovementSection.AddEntry(Private::CreateCameraMovementSubmenu(DialogViewportWeak));
					}

					// Options Section
					{
						FToolMenuSection& OptionsSection =
							InDynamicMenu->FindOrAddSection("CameraOptions", LOCTEXT("OptionsLabel", "Options"));

						FToolMenuEntry ToggleGameView =
							FToolMenuEntry::InitMenuEntry(FDialogBuilder_EditorCommands::Get().ToggleGameView);
						ToggleGameView.UserInterfaceActionType = EUserInterfaceActionType::ToggleButton;
						OptionsSection.AddEntry(ToggleGameView);
					}
				}
			)
		);

		Private::ExtendCameraSpeedSubmenu(UToolMenus::JoinMenuPaths(InCameraOptionsSubmenuName, "CameraMovement.CameraSpeed"));
	}


	FToolMenuEntry CreateToolbarCameraSubmenu()
	{
		FToolMenuEntry Entry = FToolMenuEntry::InitDynamicEntry(
			"DynamicCameraOptions",
			FNewToolMenuSectionDelegate::CreateLambda(
				[](FToolMenuSection& InDynamicSection) -> void
				{
					UUnrealEdViewportToolbarContext* const ToolbarContext = InDynamicSection.FindContext<UUnrealEdViewportToolbarContext>();
					const TSharedPtr<SDialogPreviewViewport> DialogViewport = ToolbarContext ? StaticCastSharedPtr<SDialogPreviewViewport>(ToolbarContext->Viewport.Pin()) : nullptr;
					if (!DialogViewport.IsValid())
					{
						return;
					}
					TWeakPtr<SDialogPreviewViewport> DialogViewportWeak = DialogViewport.ToWeakPtr();

					if (ToolbarContext)
					{
						const TAttribute<FText> Label = TAttribute<FText>::CreateLambda(
							[ViewportWeak = DialogViewportWeak]()
							{
								return UE::DialogEditor::Private::GetCameraSubmenuLabelFromLevelViewport(ViewportWeak);
							}
						);

						const TAttribute<FSlateIcon> Icon = TAttribute<FSlateIcon>::CreateLambda(
							[ViewportWeak = DialogViewportWeak]()
							{
								return UE::DialogEditor::Private::GetCameraSubmenuIconFromLevelViewport(ViewportWeak);
							}
						);

						FToolMenuEntry& Entry = InDynamicSection.AddSubMenu(
							"Camera",
							Label,
							LOCTEXT("CameraSubmenuTooltip", "Camera options"),
							FNewToolMenuDelegate::CreateLambda(
								[](UToolMenu* Submenu) -> void
								{
									//UnrealEd::PopulateCameraMenu(Submenu, UnrealEd::FViewportCameraMenuOptions().ShowLensControls());
									UE::UnrealEd::PopulateCameraMenu(Submenu, UE::UnrealEd::FViewportCameraMenuOptions().ShowAll());

								}
							),
							false,
							Icon
						);
						Entry.ToolBarData.ResizeParams.ClippingPriority = 800;
					}
				}
			)
		);

		return Entry;
	}

	TSharedRef<SWidget> CreateViewportCameraModeWidget(TWeakPtr<FDialogBuilderEditor> InEditor)
	{
		auto MakeButton = [InEditor](EDialogViewportCameraMode Mode) -> TSharedRef<SWidget>
			{
				return SNew(SCheckBox)
					.Style(FAppStyle::Get(), "RadioButton")
					.ToolTipText_Lambda([Mode]() -> FText
						{
							switch (Mode)
							{
							case EDialogViewportCameraMode::Perspective:
								return LOCTEXT("ViewportCamModePerspectiveTip", "Regular perspective camera \n Press '1' to switch to this mode");

							case EDialogViewportCameraMode::DialogCameraLock:
								return LOCTEXT("ViewportCamModeDialogTip", "Lock camera to Dialog Camera \n Press '2' to switch to this mode");

							case EDialogViewportCameraMode::SequencerCameraCuts:
								return LOCTEXT("ViewportCamModeSequencerTip", "Toggle Sequencer camera cuts \n Press '3' to switch to this mode");

							default:
								return FText::GetEmpty();
							}
						})
					.IsChecked_Lambda([InEditor, Mode]() -> ECheckBoxState
						{
							if (const TSharedPtr<::FDialogBuilderEditor> EditorPinned = InEditor.Pin())
							{
								return EditorPinned->IsViewportCameraMode(Mode)
									? ECheckBoxState::Checked
									: ECheckBoxState::Unchecked;
							}
							return ECheckBoxState::Unchecked;
						})
					.OnCheckStateChanged_Lambda([InEditor, Mode](ECheckBoxState NewState)
						{
							if (NewState == ECheckBoxState::Checked)
							{
								if (const TSharedPtr<::FDialogBuilderEditor> EditorPinned = InEditor.Pin())
								{
									EditorPinned->SetViewportCameraMode(Mode);
								}
							}
						})
					[
						SNew(SImage)
							.Image(UE::DialogEditor::Private::GetViewportCameraModeBrush(Mode))
					];
			};

		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
			[
				MakeButton(EDialogViewportCameraMode::Perspective)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
			[
				MakeButton(EDialogViewportCameraMode::DialogCameraLock)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
			[
				MakeButton(EDialogViewportCameraMode::SequencerCameraCuts)
			];
	}
}

#undef LOCTEXT_NAMESPACE