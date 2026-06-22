#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FDialogBuilderEditor;
class IDetailsView;
class UDialogSequenceShot;
struct FPropertyChangedEvent;

struct FDialogCameraPresetItem
{
	FText DisplayName;
	FAssetData AssetData;
	TStrongObjectPtr<UDialogSequenceShot> ShotInstance;
};

using FDialogCameraPresetItemPtr = TSharedPtr<FDialogCameraPresetItem>;

class DIALOG_SYSTEM_EDITOR_API SDialogCameraPresets : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogCameraPresets) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FDialogBuilderEditor> InDialogEditor);
	virtual ~SDialogCameraPresets();

	void Refresh();

private:
	TSharedRef<ITableRow> GenerateCameraPresetRow(
		FDialogCameraPresetItemPtr InItem,
		const TSharedRef<STableViewBase>& InOwnerTable);

	void OnCameraPresetSelectionChanged(FDialogCameraPresetItemPtr InItem, ESelectInfo::Type InSelectInfo);
	void ApplyCameraPresetItem(FDialogCameraPresetItemPtr InItem);
	void BindShotPropertyChanged(UDialogSequenceShot* InShot);
	void UnbindShotPropertyChanged();
	void OnShotPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);

	void RebuildCameraPresetList();
	UDialogSequenceShot* CreateSelectedShotInstance(const FAssetData& AssetData) const;
	bool IsCameraPresetBlueprint(const FAssetData& AssetData) const;
	void ReleaseShotDuplicates();

public:
	TArray<UDialogSequenceShot*> GetShotDuplicates() const;
	static bool HasClassPathToken(const FString& InTagValue, const UClass* InClass);

private:
	TWeakPtr<FDialogBuilderEditor> DialogEditorPtr;

	TArray<FDialogCameraPresetItemPtr> CameraPresetItems;
	TArray<TStrongObjectPtr<UDialogSequenceShot>> ShotDuplicates;
	FSoftObjectPath SelectedPresetPath;
	bool bIsRebuildingCameraPresetList = false;

	TSharedPtr<SListView<FDialogCameraPresetItemPtr>> CameraPresetListView;
	TSharedPtr<IDetailsView> ShotDetailsView;
	TWeakObjectPtr<UDialogSequenceShot> ActiveShot;
	FDelegateHandle ShotPropertyChangedHandle;
};