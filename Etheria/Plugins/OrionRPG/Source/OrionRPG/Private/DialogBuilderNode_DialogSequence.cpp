// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilderNode_DialogSequence.h"
#include "DialogBuilderNode_PlayerChoice.h"
#include "Decorator/OrionDecorator.h"
#include "DialogBuilderGraph.h"
#include "DialogSequence.h"
#include "DialogBuilderSetting.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimMontage.h"
#include "Sound/SoundBase.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "DialogComponent.h"
#include "DialogStage.h"

#define LOCTEXT_NAMESPACE "DialogNode_Sequence"



UDialogBuilderNode_DialogSequence::UDialogBuilderNode_DialogSequence()
{
}

void UDialogBuilderNode_DialogSequence::BeginNode()
{
	Super::BeginNode();

	if (!ShouldPlaySequence())
	{
		EvaluateNextNode();
		return;
	}

	GetOwningDialogGraph()->BeginDialogSequence(this);
}

void UDialogBuilderNode_DialogSequence::EvaluateNextNode()
{
	Super::EvaluateNextNode();
}

bool UDialogBuilderNode_DialogSequence::ShouldPlaySequence()
{
	return DialogSequence && DialogStage && GetSequenceDuration() > KINDA_SMALL_NUMBER;
}

void UDialogBuilderNode_DialogSequence::EnsureSequenceCreated()
{
	if (!DialogSequence)
	{
		DialogSequence = NewObject<UDialogSequence>(this, NAME_None, RF_Transactional);
		DialogSequence->OwningDialogGraph = GetOwningDialogGraph();	
		DialogSequence->MovieScene = NewObject<UMovieScene>(DialogSequence, FName("DialogSequence"), RF_Transactional);
		DialogSequence->MovieScene->SetDisplayRate(FFrameRate(30, 1));
		DialogSequence->RefreshSequence();
	}

	
}

float UDialogBuilderNode_DialogSequence::GetSequenceDuration()
{
	float Duration = 0.f;
	if (DialogSequence)
	{
		FFrameRate TickResolution = DialogSequence->MovieScene->GetTickResolution();
		float SequenceDuration = DialogSequence->MovieScene->GetPlaybackRange().GetUpperBoundValue() / TickResolution;
		Duration = FMath::Max(Duration, SequenceDuration);
	}
	return Duration;
}

void UDialogBuilderNode_DialogSequence::UseDialogStage(UDialogStage* InDialogStage)
{
	Modify();
	DialogStageToUse = InDialogStage;

	UpdateDialogStageData();

	UMovieScene* MovieScene = DialogSequence ? DialogSequence->GetMovieScene() : nullptr;
	if (!MovieScene)
	{
		return;
	}
	DialogSequence->Modify();
	MovieScene->Modify();
}

void UDialogBuilderNode_DialogSequence::UpdateDialogStageData()
{
	if (!DialogStageToUse)
	{
		UE_LOG(LogTemp, Warning, TEXT("DialogStageToUse is null, cannot update DialogStage data"));
		DialogStage = nullptr;
		return;
	}

	if (!DialogStage)
	{
		DialogStage = NewObject<UDialogStage>(this, NAME_None, RF_Transactional);
	}
	UMovieScene* MovieScene = DialogSequence ? DialogSequence->GetMovieScene() : nullptr;
	if (!MovieScene)
	{
		return;
	}


	DialogStage->OwningDialogGraph = GetOwningDialogGraph();
	DialogStage->Name = DialogStageToUse->Name;
	DialogStage->Location = DialogStageToUse->Location;
	DialogStage->Rotation = DialogStageToUse->Rotation;

	//Populate Dialog Definition Slots
	{
		const int32 SourceCount = DialogStageToUse->Slots.Num();
		const int32 SlotCount = DialogStage->Slots.Num();

		//remove possessable if there is excess slots
		if (SlotCount > SourceCount)
		{
			for (int32 Index = SlotCount - 1; Index >= SourceCount; --Index)
			{
				if (UDialogSequenceSlot* Slot = DialogStage->Slots[Index])
				{
					Slot->DialogDefinition = nullptr;
					//TODO: for now, dont remove possessable, as changing empty dialog set will delete all your progress
					//DialogSequence->MovieScene->RemovePossessable(Slot->ID);
				}

			}
		}
		
		if (SlotCount < SourceCount)
		{
			DialogStage->Slots.SetNum(SourceCount);
		}
		for (int32 idx = 0; idx < SourceCount; ++idx)
		{
			UDialogSequenceSlot* Source = DialogStageToUse->Slots[idx];
			if (!Source)
			{
				DialogStage->Slots[idx] = nullptr;
				continue;
			}

			UDialogSequenceSlot* Target = DialogStage->Slots[idx];
			if (!Target)
			{
				Target = NewObject<UDialogSequenceSlot>(DialogStage, NAME_None, RF_Transactional);
			}
			UDialogDefinition* SourceDialogDefinition = Source->DialogDefinition;
			Target->OwningDialogGraph = GetOwningDialogGraph();
			Target->DialogDefinition = SourceDialogDefinition;
			Target->SlotLocation = Source->SlotLocation;
			Target->SlotRotation = Source->SlotRotation	;

			DialogStage->Slots[idx] = Target;

			

		}
	}


	
	//Populate Light Slots
	{
		const int32 SourceCount = DialogStageToUse->LightSlots.Num();
		const int32 SlotCount = DialogStage->LightSlots.Num();


		DialogStage->LightSlots.SetNum(SourceCount);
		for (int32 idx = 0; idx < SourceCount; ++idx)
		{
			UDialogSequenceSlot_Light* Source = DialogStageToUse->LightSlots[idx];
			if (!Source)
			{
				DialogStage->LightSlots[idx] = nullptr;
				continue;
			}

			UDialogSequenceSlot_Light* Target = DialogStage->LightSlots[idx];
			if (!Target)
			{
				Target = NewObject<UDialogSequenceSlot_Light>(DialogStage, NAME_None, RF_Transactional);
			}
			if(!Target->ID.IsValid())
			{
				Target->ID = FGuid::NewGuid();
			}
			Target->OwningDialogGraph = GetOwningDialogGraph();
			Target->LightClass = Source->LightClass;
			Target->SlotLocation = Source->SlotLocation;
			Target->SlotRotation = Source->SlotRotation;
			Target->Intensity = Source->Intensity;
			Target->IntensityUnits = Source->IntensityUnits;
			Target->LightColor = Source->LightColor;
			Target->AttenuationRadius = Source->AttenuationRadius;
			Target->bUseTemperature = Source->bUseTemperature;
			Target->Temperature = Source->Temperature;
			Target->bAffectsWorld = Source->bAffectsWorld;
			Target->CastShadows = Source->CastShadows;
			Target->IndirectLightingIntensity = Source->IndirectLightingIntensity;
			Target->VolumetricScatteringIntensity = Source->VolumetricScatteringIntensity;
			Target->CastStaticShadows = Source->CastStaticShadows;
			Target->CastDynamicShadows = Source->CastDynamicShadows;
			Target->bAffectTranslucentLighting = Source->bAffectTranslucentLighting;
			Target->bCastVolumetricShadow = Source->bCastVolumetricShadow;
			Target->bCastDeepShadow = Source->bCastDeepShadow;
			Target->CastRaytracedShadow = Source->CastRaytracedShadow;
			Target->bAffectReflection = Source->bAffectReflection;
			Target->bAffectGlobalIllumination = Source->bAffectGlobalIllumination;
			Target->DeepShadowLayerDistribution = Source->DeepShadowLayerDistribution;
			DialogStage->LightSlots[idx] = Target;
		}
	}
}

void UDialogBuilderNode_DialogSequence::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

}


#if WITH_EDITOR

FText UDialogBuilderNode_DialogSequence::GetNodeTitle() const
{
    return NodeDisplayName.IsEmpty() ? FText::FromString("Dialog Sequence") : FText::FromString(NodeDisplayName);
}

void UDialogBuilderNode_DialogSequence::SetNodeTitle(const FText& NewTitle)
{
    ID = FName(NewTitle.ToString());
}

FText UDialogBuilderNode_DialogSequence::GetNodeDescription() const
{
	return DialogStageToUse
		? FText::Format(LOCTEXT("DialogStageDescription", "Stage: {0}"), DialogStageToUse->Name)
		: FText::FromString("Stage None");
}


void UDialogBuilderNode_DialogSequence::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UDialogBuilderNode_DialogSequence::PostLoad()
{
	Super::PostLoad();
	EnsureSequenceCreated();
}

#endif

void UDialogBuilderNode_DialogSequence::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}
#undef LOCTEXT_NAMESPACE

