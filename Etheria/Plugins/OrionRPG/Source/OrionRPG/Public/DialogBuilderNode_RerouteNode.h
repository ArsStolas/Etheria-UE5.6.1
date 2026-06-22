// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_RerouteNode.generated.h"

UENUM(BlueprintType)
enum class EDialogRerouteRule : uint8
{
	/**Go to the latest selection state*/
	E_BackToLastSelection	UMETA(DisplayName = "Back To Last Selection State"),
	/**Go to specific node*/
	E_GoToNode				UMETA(DisplayName = "Go To Specific Node"),
};

UCLASS(BlueprintType)
class ORIONRPG_API UDialogBuilderNode_RerouteNode : public UDialogBuilderNode
{
	GENERATED_BODY()
	
public:
	UDialogBuilderNode_RerouteNode();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RerouteRule")
	EDialogRerouteRule RerouteRule;

	/**Play the dialog line first before entering selection mode*/
	UPROPERTY(BlueprintReadOnly, Category = "RerouteRule", meta = (EditCondition = "RerouteRule == EDialogRerouteRule::E_BackToLastSelection", HideEditConditionToggle, EditConditionHides))
		bool bPlayDialogLine;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "RerouteRule", meta = (EditCondition = "RerouteRule == EDialogRerouteRule::E_GoToNode", HideEditConditionToggle, EditConditionHides, GetOptions="GetNodeListToReroute"))
		FName NodeID;

	virtual void BeginNode() override;

	UFUNCTION(CallInEditor)
	TArray<FName> GetNodeListToReroute();
public:
#if WITH_EDITOR
    virtual FText GetNodeTitle() const override;
    virtual void SetNodeTitle(const FText& NewTitle);
    virtual FText GetNodeDescription() const override;
#endif
};
