#include "SDialogCameraPresets.h"

#include "DialogBuilderEditor.h"
#include "DialogBuilderGraph.h"
#include "DialogSequenceShot.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DialogCameraPresets"

namespace
{

	class SDialogCameraPresetItem : public SButton
	{
	public:
		SLATE_BEGIN_ARGS(SDialogCameraPresetItem) {}
			SLATE_ARGUMENT(FDialogCameraPresetItemPtr, Item)
			SLATE_ARGUMENT(FText, DisplayName)
			SLATE_EVENT(FOnClicked, OnClicked)
			SLATE_EVENT(FOnClicked, OnKeyClicked)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Item = InArgs._Item;
			OnKeyClicked = InArgs._OnKeyClicked;

			SButton::Construct(
				SButton::FArguments()
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(FMargin(0.0f))
				.OnClicked(InArgs._OnClicked)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("NoBorder"))
						.Padding(FMargin(4.0f, 2.0f))
						[
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
										.Text(InArgs._DisplayName)
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.0f))
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(FMargin(6.0f, 0.0f, 0.0f, 0.0f))
								[
									SAssignNew(KeyButton, SButton)
										.ButtonStyle(FAppStyle::Get(), "SimpleButton")
										.ContentPadding(FMargin(0.0f))
										.ToolTipText(LOCTEXT("KeyAtCurrentFrameTooltip", "Add key at current frame"))
										.OnClicked(OnKeyClicked)
										[
											SNew(SImage)
												.Visibility(EVisibility::All)
												.Image_Lambda([this]()
													{
														return (KeyButton.IsValid() && KeyButton->IsHovered())
															? FAppStyle::GetBrush("Sequencer.KeyDiamond")
															: FAppStyle::GetBrush("Sequencer.KeyDiamondBorder");
													})
										]
								]
						]
				]
			);
		}

	private:
		FDialogCameraPresetItemPtr Item;
		TSharedPtr<SButton> KeyButton;
		FOnClicked OnKeyClicked;
	};
}

void SDialogCameraPresets::Construct(const FArguments& InArgs, TWeakPtr<FDialogBuilderEditor> InDialogEditor)
{
	DialogEditorPtr = InDialogEditor;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(EOrientation::Orient_Vertical)

			+ SSplitter::Slot()
			.Value(0.35f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4.0f)
				[

					SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(4.0f)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
										.Text(LOCTEXT("CameraPresetListTitle", "Camera Presets"))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10.5f))
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SButton)
										.ButtonStyle(FAppStyle::Get(), "SimpleButton")
										.ContentPadding(FMargin(2.f))
										.ToolTipText(LOCTEXT("AddCameraPresetTooltip", "Create New Sequence Shot Class"))
										.OnClicked_Lambda([this]()
											{
												if (TSharedPtr<FDialogBuilderEditor> DialogEditor = DialogEditorPtr.Pin())
												{
													DialogEditor->HandleNewClassPicked(UDialogSequenceShot::StaticClass());
													RebuildCameraPresetList();
												}
												return FReply::Handled();
											})
										[
											SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
										]
								]
						]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(2.0f)
					[
						SAssignNew(CameraPresetListView, SListView<FDialogCameraPresetItemPtr>)
						.ListItemsSource(&CameraPresetItems)
						.OnGenerateRow(this, &SDialogCameraPresets::GenerateCameraPresetRow)
						.OnSelectionChanged(this, &SDialogCameraPresets::OnCameraPresetSelectionChanged)
						.SelectionMode(ESelectionMode::Single)
					]
				]
			]
		]
	];

	RebuildCameraPresetList();
}

SDialogCameraPresets::~SDialogCameraPresets()
{
	ChildSlot
	[
		SNullWidget::NullWidget
	];

	if (CameraPresetListView.IsValid())
	{
		CameraPresetListView->ClearSelection();
		CameraPresetListView.Reset();
	}

	UnbindShotPropertyChanged();
	ReleaseShotDuplicates();
}

void SDialogCameraPresets::Refresh()
{
	RebuildCameraPresetList();
}

TSharedRef<ITableRow> SDialogCameraPresets::GenerateCameraPresetRow(
	FDialogCameraPresetItemPtr InItem,
	const TSharedRef<STableViewBase>& InOwnerTable)
{
	return SNew(STableRow<FDialogCameraPresetItemPtr>, InOwnerTable)
		[
			SNew(SDialogCameraPresetItem)
				.Item(InItem)
				.DisplayName(InItem.IsValid() ? InItem->DisplayName : LOCTEXT("InvalidCameraPreset", "Invalid"))
				.OnClicked_Lambda([this, InItem]()
					{
						if (!InItem.IsValid() || !InItem->ShotInstance.IsValid())
						{
							return FReply::Handled();
						}

						if (CameraPresetListView.IsValid() && CameraPresetListView->IsItemSelected(InItem))
						{
							ApplyCameraPresetItem(InItem);
						}
						else if (CameraPresetListView.IsValid())
						{
							CameraPresetListView->SetSelection(InItem, ESelectInfo::Direct);
						}

						return FReply::Handled();
					})
				.OnKeyClicked_Lambda([this]()
					{
						const FScopedTransaction Transaction(LOCTEXT("AddCameraPresetKeyTransaction", "Add Camera Preset Key"));

						FDialogBuilderEditor* DialogEditor = DialogEditorPtr.Pin().Get();
						if (DialogEditor)
						{
							DialogEditor->AddKeyFromCameraPreset(-1, EMovieSceneKeyInterpolation::Constant);
						}
						return FReply::Handled();
					})
		];
}

void SDialogCameraPresets::OnCameraPresetSelectionChanged(FDialogCameraPresetItemPtr InItem, ESelectInfo::Type InSelectInfo)
{
	ApplyCameraPresetItem(InItem);
}

void SDialogCameraPresets::ApplyCameraPresetItem(FDialogCameraPresetItemPtr InItem)
{
	FDialogBuilderEditor* DialogEditor = DialogEditorPtr.Pin().Get();

	if (!InItem.IsValid() || !InItem->ShotInstance.IsValid())
	{
		ActiveShot.Reset();
		UnbindShotPropertyChanged();

		if (DialogEditor)
		{
			DialogEditor->SetDetailsObject(nullptr);
		}

		if (ShotDetailsView.IsValid())
		{
			ShotDetailsView->SetObject(nullptr);
		}

		return;
	}

	UDialogSequenceShot* Shot = InItem->ShotInstance.Get();
	ActiveShot = Shot;

	if (ShotDetailsView.IsValid())
	{
		ShotDetailsView->SetObject(Shot);
	}

	BindShotPropertyChanged(Shot);

	if (DialogEditor)
	{
		DialogEditor->SetDetailsObject(Shot);
		DialogEditor->ApplyCameraPreset(Shot);
		DialogEditor->SetViewportCameraMode(EDialogViewportCameraMode::DialogCameraLock);
	}
}

void SDialogCameraPresets::BindShotPropertyChanged(UDialogSequenceShot* InShot)
{
	UnbindShotPropertyChanged();

	if (!InShot)
	{
		return;
	}

	ShotPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(
		this,
		&SDialogCameraPresets::OnShotPropertyChanged);
}

void SDialogCameraPresets::UnbindShotPropertyChanged()
{
	if (ShotPropertyChangedHandle.IsValid())
	{
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(ShotPropertyChangedHandle);
		ShotPropertyChangedHandle.Reset();
	}
}

void SDialogCameraPresets::OnShotPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
	FDialogBuilderEditor* DialogEditor = DialogEditorPtr.Pin().Get();
	UDialogSequenceShot* Shot = ActiveShot.Get();

	if (!DialogEditor || !Shot || ObjectBeingModified != Shot)
	{
		return;
	}

	DialogEditor->ApplyCameraPreset(Shot);
}

void SDialogCameraPresets::RebuildCameraPresetList()
{
	ReleaseShotDuplicates();
	CameraPresetItems.Empty();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets);

	for (const FAssetData& AssetData : Assets)
	{
		if (!IsCameraPresetBlueprint(AssetData))
		{
			continue;
		}

		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			continue;
		}

		TStrongObjectPtr<UDialogSequenceShot> SelectedShot(CreateSelectedShotInstance(AssetData));
		if (!SelectedShot.IsValid())
		{
			continue;
		}

		ShotDuplicates.Add(MoveTemp(SelectedShot));

		FDialogCameraPresetItemPtr Item = MakeShared<FDialogCameraPresetItem>();
		Item->DisplayName = !Blueprint->BlueprintDisplayName.IsEmpty()
			? FText::FromString(Blueprint->BlueprintDisplayName)
			: FText::FromName(AssetData.AssetName);
		Item->AssetData = AssetData;
		Item->ShotInstance = ShotDuplicates.Last();

		CameraPresetItems.Add(Item);
	}

	CameraPresetItems.Sort([](const FDialogCameraPresetItemPtr& A, const FDialogCameraPresetItemPtr& B)
	{
		const FString AName = A.IsValid() ? A->DisplayName.ToString() : FString();
		const FString BName = B.IsValid() ? B->DisplayName.ToString() : FString();
		return AName < BName;
	});

	if (CameraPresetListView.IsValid())
	{
		CameraPresetListView->RequestListRefresh();

		if (CameraPresetItems.Num() > 0)
		{
			CameraPresetListView->SetSelection(CameraPresetItems[0], ESelectInfo::Direct);
		}
	}
}

bool SDialogCameraPresets::IsCameraPresetBlueprint(const FAssetData& AssetData) const
{
	FString ParentClassPath;

	if (!AssetData.GetTagValue(FName(TEXT("NativeParentClassPath")), ParentClassPath) &&
		!AssetData.GetTagValue(FName(TEXT("ParentClass")), ParentClassPath))
	{
		return false;
	}

	return HasClassPathToken(ParentClassPath, UDialogSequenceShot::StaticClass());
}

bool SDialogCameraPresets::HasClassPathToken(const FString& InTagValue, const UClass* InClass)
{
	if (!InClass)
	{
		return false;
	}

	const FString ClassName = InClass->GetName();
	const FString ClassPath = InClass->GetPathName();

	return InTagValue.Contains(ClassName) || InTagValue.Contains(ClassPath);
}

UDialogSequenceShot* SDialogCameraPresets::CreateSelectedShotInstance(const FAssetData& AssetData) const
{
	UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return nullptr;
	}

	UDialogSequenceShot* SourceShot = Cast<UDialogSequenceShot>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!SourceShot)
	{
		return nullptr;
	}

	UDialogSequenceShot* OutShot = NewObject<UDialogSequenceShot>(
		GetTransientPackage(), Blueprint->GeneratedClass, NAME_None, RF_Transient | RF_Transactional);

	FDialogBuilderEditor* DialogEditor = DialogEditorPtr.Pin().Get();
	UDialogBuilderGraph* DialogGraph = DialogEditor ? DialogEditor->GetDialogBuilderGraph() : nullptr;

	OutShot->OwningDialogGraph = DialogGraph;
	return OutShot;
}

TArray<UDialogSequenceShot*> SDialogCameraPresets::GetShotDuplicates() const
{
	TArray<UDialogSequenceShot*> Result;
	Result.Reserve(ShotDuplicates.Num());

	for (const TStrongObjectPtr<UDialogSequenceShot>& Shot : ShotDuplicates)
	{
		if (Shot.IsValid())
		{
			Result.Add(Shot.Get());
		}
	}

	return Result;
}

void SDialogCameraPresets::ReleaseShotDuplicates()
{
	UnbindShotPropertyChanged();

	if (ShotDetailsView.IsValid())
	{
		ShotDetailsView->SetObject(nullptr);
	}

	ActiveShot.Reset();
	ShotDuplicates.Empty();
	CameraPresetItems.Empty();
}

#undef LOCTEXT_NAMESPACE
