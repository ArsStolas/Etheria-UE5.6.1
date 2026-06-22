#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "DialogBuilderEditor.h"

class IDetailsView;
class UDialogBuilderGraph;
class UDialogStage;
class UDialogSequenceSlot;
class UDialogSequenceSlot_Light;
class FUICommandList;
class SComboButton;
struct FAssetData;

struct FDialogStageListItem
{
	TWeakObjectPtr<UDialogStage> DialogStage;
};

struct FDialogSlotListItem
{
	TWeakObjectPtr<UDialogSequenceSlot> Slot;
	bool bIsCameraSlot = false;
	bool bIsLightSlot = false;
	int32 SlotIndex = INDEX_NONE;
};

struct FDialogStageAssetItem
{
	FAssetData AssetData;
	FText DisplayName;
};

using FDialogStageListItemPtr = TSharedPtr<FDialogStageListItem>;
using FDialogSlotListItemPtr = TSharedPtr<FDialogSlotListItem>;
using FDialogStageAssetItemPtr = TSharedPtr<FDialogStageAssetItem>;

class DIALOG_SYSTEM_EDITOR_API SDialogStageManager : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogStageManager) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FDialogBuilderEditor> InDialogEditor, UDialogBuilderGraph* InDialogGraph);

	// Keyboard commands (Delete)
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	void Refresh();

private:
	// DialogStage list
	TSharedRef<ITableRow> GenerateDialogStageRow(FDialogStageListItemPtr InItem, const TSharedRef<STableViewBase>& InOwnerTable);
	void OnDialogStageSelectionChanged(FDialogStageListItemPtr InItem, ESelectInfo::Type InSelectInfo);
	FReply OnAddDialogStage();
	void OnRemoveDialogStage(UDialogStage* InDialogStage);
	void OnDialogStageNameCommitted(const FText& InText, ETextCommit::Type InCommitType, TWeakObjectPtr<UDialogStage> InDialogStage);
	TSharedPtr<SWidget> OnDialogStageContextMenuOpening();

	// Import DialogStage assets
	TSharedRef<SWidget> BuildImportDialogStageMenu();
	void RefreshDialogStageAssetList();
	TSharedRef<ITableRow> GenerateDialogStageAssetRow(FDialogStageAssetItemPtr InItem, const TSharedRef<STableViewBase>& InOwnerTable);
	void OnDialogStageAssetSelected(FDialogStageAssetItemPtr InItem, ESelectInfo::Type InSelectInfo);
	void ImportDialogStageAsset(const FAssetData& AssetData);
	void ResetDialogStageSlotDefinitions(UDialogStage* InStage);

	// Slot list
	TSharedRef<ITableRow> GenerateSlotRow(FDialogSlotListItemPtr InItem, const TSharedRef<STableViewBase>& InOwnerTable);
	void OnSlotSelectionChanged(FDialogSlotListItemPtr InItem, ESelectInfo::Type InSelectInfo);
	FReply OnAddSlot();
	FReply OnAddCameraSlot();
	void OnRemoveSlot(UDialogSequenceSlot* InSlot, int32 InSlotIndex, bool bIsCameraSlot);
	TSharedPtr<SWidget> OnSlotContextMenuOpening();

	// Light slot list
	TSharedRef<ITableRow> GenerateLightSlotRow(FDialogSlotListItemPtr InItem, const TSharedRef<STableViewBase>& InOwnerTable);
	void OnLightSlotSelectionChanged(FDialogSlotListItemPtr InItem, ESelectInfo::Type InSelectInfo);
	FReply OnAddLightSlot();
	void OnRemoveLightSlot(UDialogSequenceSlot_Light* InSlot, int32 InSlotIndex);
	TSharedPtr<SWidget> OnLightSlotContextMenuOpening();

	// Delete command helpers
	void OnDeleteSelection();
	bool CanDeleteSelection() const;
	bool IsSelectedSlotCamera() const;
	bool IsSelectedSlotLight() const;

	// Refresh
	void RefreshDialogStageList();
	void RefreshSlotList();
	void RefreshLightSlotList();
	void MarkGraphDirty() const;

private:
	TWeakObjectPtr<UDialogBuilderGraph> DialogGraph;
	TWeakObjectPtr<UDialogStage> SelectedDialogStage;
	TWeakObjectPtr<UDialogSequenceSlot> SelectedSlot;

	TArray<FDialogStageListItemPtr> DialogStageItems;
	TArray<FDialogSlotListItemPtr> SlotItems;
	TArray<FDialogSlotListItemPtr> LightSlotItems;
	TArray<FDialogStageAssetItemPtr> DialogStageAssetItems;

	TSharedPtr<SListView<FDialogStageListItemPtr>> DialogStageListView;
	TSharedPtr<SListView<FDialogSlotListItemPtr>> SlotListView;
	TSharedPtr<SListView<FDialogSlotListItemPtr>> LightSlotListView;
	TSharedPtr<SListView<FDialogStageAssetItemPtr>> DialogStageAssetListView;

	TSharedPtr<IDetailsView> DialogStageDetailsView;
	TSharedPtr<IDetailsView> SlotDetailsView;

	TSharedPtr<FUICommandList> CommandList;

	TSharedPtr<SComboButton> ImportDialogStageComboButton;

	int32 SelectedSlotIndex = INDEX_NONE;

	/** Pointer back to the Dialog editor that owns us */
	TWeakPtr<FDialogBuilderEditor> DialogEditorPtr;

	bool bSelectedSlotIsLight = false;

private:
	void RestoreSelection(const FName& InDialogStageObjectName, const FGuid& InSlotId);
	void SelectDialogStageByObjectName(const FName& InDialogStageObjectName);
	void SelectSlotById(const FGuid& InSlotId);
	FDialogStageListItemPtr FindDialogStageItemByObjectName(const FName& InDialogStageObjectName) const;
	FDialogSlotListItemPtr FindSlotItemById(const FGuid& InSlotId) const;

	bool IsDialogStageBlueprint(const FAssetData& AssetData) const;
	static bool HasClassPathToken(const FString& InTagValue, const UClass* InClass);

private:
	FReply OnDialogStageItemClicked(FDialogStageListItemPtr InItem);
	FReply OnSlotItemClicked(FDialogSlotListItemPtr InItem);
	FReply OnLightSlotItemClicked(FDialogSlotListItemPtr InItem);

	void ApplyDialogStageSelection(FDialogStageListItemPtr InItem);
	void ApplySlotSelection(FDialogSlotListItemPtr InItem);
};