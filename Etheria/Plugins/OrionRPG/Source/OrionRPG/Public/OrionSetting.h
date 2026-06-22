// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "Engine/DataTable.h"
#include "Interaction/InteractionData.h"
#include "UObject/NoExportTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "Templates/SubclassOf.h"
#include "OrionSetting.generated.h"

class UOrionSaveGame;
//General Setting for Orion Plugin
UCLASS(config = Engine, defaultconfig, BlueprintType)
class ORIONRPG_API UOrionSetting : public UObject
{
	GENERATED_BODY()

	UOrionSetting();
	virtual ~UOrionSetting();

public:
	//Runtime Config

	/**Show the common UI bottom bar bound action*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "UI")
	bool ShowUIBottomBarAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "UI", noclear)
	TSubclassOf<class UCommonActivatableWidget> DefaultOverlayWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Save")
	TSubclassOf<UOrionSaveGame> SaveGameClass;

	//How many slot for save games
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Save")
	int SaveSlotCapacity;

	//Will save your last level and travel when game loaded
	UPROPERTY(BlueprintReadOnly, config, Category = "Save")
	bool bSaveLevel;

	//Will save data layer and activate them on streaming map when game loaded
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Save")
	bool bSaveDataLayer;

	//Interaction input. add your own input here
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Interaction")
	FDataTableRowHandle InteractInput;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Interaction")
	EOrionInteractionWidgetType InteractionWidgetType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Interaction", noclear)
	TSubclassOf<class UCommonTextStyle> InteractTextStyle;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Interaction")
	bool bShowActionDisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Interaction", meta = (EditCondition = "bShowActionDisplayText == true", HideEditConditionToggle, EditConditionHides))
	FText DefaultActionDisplayText;
	
	UPROPERTY()
	bool bTrialVersion;
	
	UPROPERTY()
	int32 NodeLimit;

	UPROPERTY()
	FString TrialPurchaseURL;

};

