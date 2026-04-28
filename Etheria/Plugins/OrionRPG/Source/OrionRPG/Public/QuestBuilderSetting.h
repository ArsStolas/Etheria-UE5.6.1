// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "QuestBuilderSetting.generated.h"



UCLASS(config = Engine, defaultconfig, BlueprintType)
class ORIONRPG_API UQuestBuilderSetting : public UObject
{
	GENERATED_BODY()

	UQuestBuilderSetting();
	virtual ~UQuestBuilderSetting();

public:
	//Runtime Config

	/**Show Objective Updated Notification*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Quest Options")
	bool ShowObjectiveUpdatedNotification;

	/**Show Objective Indicator/ Quest Marker on level*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Quest Options")
	bool ShowObjectiveIndicator;

	/**Show the navigated quest overlay on HUD*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Quest Options")
	bool ShowNavigatedQuestOverlay;

	//Graph Setting
	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor FailedNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor SuccessNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor RootNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor ObjectiveNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor CheckpointNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor SubNodeColor;

	UPROPERTY(config)
	bool bEdgeEnabled;

	UPROPERTY(config)
	bool bCanRenameNode;

	UPROPERTY(config)
	bool bCanBeCyclical;

};

