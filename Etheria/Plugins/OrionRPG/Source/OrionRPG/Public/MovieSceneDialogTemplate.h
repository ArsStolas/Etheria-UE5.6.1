// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "DialogData.h"
#include "MovieSceneDialogTemplate.generated.h"

class IMovieScenePlayer;
class UMovieSceneDialogSection;

USTRUCT()
struct ORIONRPG_API FMovieSceneDialogSectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	FMovieSceneDialogSectionTemplate()
		: DialogSection(nullptr)
	{
	}

	FMovieSceneDialogSectionTemplate(const UMovieSceneDialogSection& InSection);

private:
	virtual UScriptStruct& GetScriptStructImpl() const override
	{
		return *StaticStruct();
	}
	
	virtual void SetupOverrides() override
	{
		EnableOverrides(RequiresSetupFlag | RequiresTearDownFlag);
	}

	virtual void Evaluate(
		const FMovieSceneEvaluationOperand& Operand,
		const FMovieSceneContext& Context,
		const FPersistentEvaluationData& PersistentData,
		FMovieSceneExecutionTokens& ExecutionTokens) const override;

	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override;
	virtual void TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override;

	UPROPERTY()
	UMovieSceneDialogSection* DialogSection;

	UPROPERTY()
	FOrionDialogLine DialogLine;
};