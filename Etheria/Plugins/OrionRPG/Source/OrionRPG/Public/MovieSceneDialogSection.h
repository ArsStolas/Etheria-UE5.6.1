// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneSection.h"
#include "DialogData.h"
#include "MovieSceneDialogSection.generated.h"

UCLASS(BlueprintType)
class ORIONRPG_API UMovieSceneDialogSection : public UMovieSceneSection
{
	GENERATED_BODY()

public:
	UMovieSceneDialogSection(const FObjectInitializer& ObjectInitializer);

	class UDialogSequence* GetOwningDialogSequence() const;
	class UDialogBuilderGraph* GetOwningDialogGraph() const;

	void RefreshDialogContent();
	void SyncRangeToDialogSoundDuration();
	bool TryGetDialogSoundDurationFrames(int32& OutDurationFrames) const;


	FOrionDialogLine GetDialogLine() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Section", meta = (ShowOnlyInnerProperties))
	FOrionDialogLine DialogLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Section")
	TObjectPtr<class UDialogParticipant> SpeakerParticipantDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Section")
	TObjectPtr<class UDialogParticipant> ListenerParticipantDefinition;

public:
#if WITH_EDITOR
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface
#endif
};