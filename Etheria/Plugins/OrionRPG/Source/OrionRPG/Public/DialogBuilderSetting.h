// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DialogData.h"
#include "DialogBuilderSetting.generated.h"



UCLASS(config = Engine, defaultconfig, BlueprintType)
class ORIONRPG_API UDialogBuilderSetting : public UObject
{
	GENERATED_BODY()

	UDialogBuilderSetting();
	virtual ~UDialogBuilderSetting();

public:

	/**Determine minimum time the line should be displayed*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Dialog Options")
		float MinDialogLineDuration;

	/**Determine how long the line should be displayed*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Dialog Options")
		float DialogLineWordsPerSecond;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Dialog Options", noclear)
	TSubclassOf<class UCommonActivatableWidget> DefaultDialogWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, config, Category = "Dialog Options", noclear)
	TSubclassOf<class UCommonActivatableWidget> DefaultFreeMovementDialogWidget;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor DialogLineNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor PlayerLineNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor StateNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor RootNodeColor;
	
	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor EndDialogNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor RerouteNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor PlayerOptionNodeColor;

	UPROPERTY(EditAnywhere, config, Category = "Graph Style")
	FLinearColor SubNodeColor;

	UPROPERTY(config)
	bool bEdgeEnabled;

	UPROPERTY(config)
	bool bCanRenameNode;

	UPROPERTY(config)
	bool bCanBeCyclical;
	



};

