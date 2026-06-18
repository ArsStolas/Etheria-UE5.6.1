// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderNode.h"
#include "DialogData.h"
#include <MovieSceneSequencePlayer.h>
#include <CineCameraSettings.h>
#include "LevelSequencePlayer.h"
#include "DialogBuilderNode_DialogLine.generated.h"

class ORIONRPG_API IDialogNodeSharedDataHelper
{
public:
	void MakeSureGuidExists(UDialogBuilderNode_DialogLine* Node);

protected:
	virtual bool CheckIfNodesShouldShareData(const UDialogBuilderNode_DialogLine* NodeA, const UDialogBuilderNode_DialogLine* NodeB) = 0;
	virtual bool CheckIfHasDataToShare(const UDialogBuilderNode_DialogLine* Node) = 0;
	virtual void ShareData(UDialogBuilderNode_DialogLine* NodeWhoWantsToShare, const UDialogBuilderNode_DialogLine* ShareFrom) = 0;
	virtual FString& AccessShareDataName(UDialogBuilderNode_DialogLine* Node) = 0;
	virtual FGuid& AccessShareDataGuid(UDialogBuilderNode_DialogLine* Node) = 0;
};

//////////////////////////////////////////////////////////////////////////
// FDialogSharedParticipantNodeHelper

class ORIONRPG_API FDialogSharedParticipantNodeHelper : public IDialogNodeSharedDataHelper
{
protected:
	virtual bool CheckIfNodesShouldShareData(const UDialogBuilderNode_DialogLine* NodeA, const UDialogBuilderNode_DialogLine* NodeB) override;
	virtual bool CheckIfHasDataToShare(const UDialogBuilderNode_DialogLine* Node) override;
	virtual void ShareData(UDialogBuilderNode_DialogLine* NodeWhoWantsToShare, const UDialogBuilderNode_DialogLine* ShareFrom) override;
	virtual FString& AccessShareDataName(UDialogBuilderNode_DialogLine* Node) override;
	virtual FGuid& AccessShareDataGuid(UDialogBuilderNode_DialogLine* Node) override;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSharedParticipantSignature);

UCLASS(Blueprintable, BlueprintType, AutoExpandCategories = "Shot Override")
class ORIONRPG_API UDialogBuilderNode_DialogLine : public UDialogBuilderNode
{
	GENERATED_BODY()

public:
	UDialogBuilderNode_DialogLine();

	UPROPERTY()
		bool bIsPlayerLine = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog", meta = (EditCondition = "bIsPlayerLine == false", HideEditConditionToggle, EditConditionHides, DisplayAfter = "TimeLimit"))
		FParticipantInfo ParticipantInfo;

	UPROPERTY()
		FString SharedParticipantName;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detail", meta = (ShowOnlyInnerProperties))
		FDialogLineData DialogLineData;

	/**Rotate the speaker to face the listener.
	* Useful for some shot that require rotation for speaker like [Over the shoulder shot], etc.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation Settings")
		bool bRotateToListener;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation Settings", meta = (EditCondition = "bRotateToListener == true", HideEditConditionToggle, EditConditionHides, Categories = "Dialog.Participant"))
	FGameplayTag ListenerTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detail")
		bool bCanSkipDialogLine;

	/** The participant data of this node may be shared */
	UPROPERTY()
		bool bSharedParticipant;

	UPROPERTY()
		FGuid SharedParticipantGuid;

	UPROPERTY()
		int32 SharedParticipantIdx;
	UPROPERTY()
	FSharedParticipantSignature OnSharedParticipantChanged;
	
public:
	virtual void BeginNode() override;

	virtual void EvaluateNextNode() override;


	UFUNCTION(BlueprintPure, Category="DialogLine")
	UObject* GetParticipantImage();

	UFUNCTION(BlueprintPure, Category = "DialogLine")
	float GetLineDuration();

public:
	virtual void Serialize(FArchive& Ar) override;


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialog")
	FOrionDialogLine GetDialogLine();

	//Participant Sharing
	void MakeParticipantShareable(FString ShareName);
	void UnshareParticipant();
	void UseSharedParticipant(const UDialogBuilderNode_DialogLine* Node);

	void CopyParticipantSettings(const UDialogBuilderNode_DialogLine* SrcNode);
	void PropagateParticipantSettings();

public:
#if WITH_EDITOR

	//Node Setting
	virtual FText GetNodeTitle() const override;
	virtual void SetNodeTitle(const FText& NewTitle);
	virtual FText GetNodeDescription() const override;


	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
	//~ End UObject Interface
#endif
};
