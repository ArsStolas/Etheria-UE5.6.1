// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogBuilderNode.h"
#include "DialogData.h"
#include <MovieSceneSequencePlayer.h>
#include <CineCameraSettings.h>
#include "LevelSequencePlayer.h"
#include "DialogBuilderNode_DialogSequence.generated.h"

class UDialogSequence;
class UDialogStage;
 
UCLASS(Blueprintable, BlueprintType, AutoExpandCategories = "Shot Override")
class ORIONRPG_API UDialogBuilderNode_DialogSequence : public UDialogBuilderNode
{
	GENERATED_BODY()

public:
	UDialogBuilderNode_DialogSequence();
	
public:
	virtual void BeginNode() override;
	virtual void EvaluateNextNode() override;

	void EnsureSequenceCreated();

	UFUNCTION(BlueprintPure, Category = "Dialog Sequence")
	float GetSequenceDuration();

	void UseDialogStage(UDialogStage* InDialogStage);
	void UpdateDialogStageData();
	bool ShouldPlaySequence();

public:
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;

	UPROPERTY(BlueprintReadOnly, Category = "Detail")
	TObjectPtr<UDialogSequence> DialogSequence = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detail")
	FString NodeDisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog Sequence")
	FMovieSceneSequencePlaybackSettings PlaybackSettings;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog Sequence")
	TObjectPtr<UDialogStage> DialogStageToUse;

	/**Dialog Set for this sequence node*/
	UPROPERTY(Instanced, BlueprintReadOnly, Category = "Dialog Sequence")
	TObjectPtr<UDialogStage> DialogStage;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog Sequence", meta = (DisplayThumbnail = "true"))
	TObjectPtr<class UTexture2D> Thumbnail = nullptr;

#if WITH_EDITORONLY_DATA
	TWeakPtr<ISequencer> Sequencer;
#endif

public:
#if WITH_EDITOR
	virtual FText GetNodeTitle() const override;
	virtual void SetNodeTitle(const FText& NewTitle);
	virtual FText GetNodeDescription() const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
#endif
};
