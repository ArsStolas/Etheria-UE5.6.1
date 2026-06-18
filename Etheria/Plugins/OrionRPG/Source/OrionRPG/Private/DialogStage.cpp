// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogStage.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderNode_DialogSequence.h"
#include "DialogDefinition.h"
#include "CineCameraActor.h"
#include "Misc/Guid.h"

#define LOCTEXT_NAMESPACE "DialogStage"

UDialogSequenceSlot::UDialogSequenceSlot()
{
}

UDialogStage* UDialogSequenceSlot::GetOwningDialogStage() const
{
	return GetTypedOuter<UDialogStage>();
}

FGameplayTag UDialogSequenceSlot::GetActorTag() const
{
	if (DialogDefinition)
	{
		return DialogDefinition->Tag;
	}
	return FGameplayTag::EmptyTag;
}

void UDialogSequenceSlot::SetDialogDefinition(UDialogDefinition* InDialogDefinition)
{
	Modify();
	DialogDefinition = InDialogDefinition;

	if(OwningDialogGraph)
	{
		TArray<UDialogBuilderNode_DialogSequence*> OutNodes;
		OwningDialogGraph->GetNodesOfType<UDialogBuilderNode_DialogSequence>(OutNodes);
		for (UDialogBuilderNode_DialogSequence* SequenceNode : OutNodes)
		{
			if (SequenceNode)
			{
				SequenceNode->Modify();
				SequenceNode->UpdateDialogStageData();
			}
		}
	}
}

UDialogSequenceSlot_Light::UDialogSequenceSlot_Light()
{
	LightClass = APointLight::StaticClass();
	
	Intensity = 5000;
	IntensityUnits = ELightUnits::Unitless;
	AttenuationRadius = 1000;
	LightColor = FColor::White;
	VolumetricScatteringIntensity = 1.0f;
	bAffectsWorld = true;
	CastShadows = true;
	CastStaticShadows = true;
	CastDynamicShadows = true;
	CastRaytracedShadow = ECastRayTracedShadow::UseProjectSetting;
	bAffectReflection = true;
	bAffectGlobalIllumination = true;
	
	DeepShadowLayerDistribution = 0.5f;
	Temperature = 6500.0f;
	bUseTemperature = false;
	IndirectLightingIntensity = 1.0f;
	

	bAffectTranslucentLighting = true;
	
}


#if WITH_EDITOR


void UDialogSequenceSlot::UpdateActorTemplate(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}

	if (ActorTemplate)
	{
		// Copy properties from the live actor back to the template
		UEngine::CopyPropertiesForUnrelatedObjects(InActor, ActorTemplate);
	}
	else
	{
		// First time initialization: Duplicate the actor into this slot
		ActorTemplate = Cast<AActor>(StaticDuplicateObject(InActor, this, NAME_None, RF_Transactional));
		if (ActorTemplate)
		{
			// Strip unwanted flags like RF_Transient since this needs to be saved
			ActorTemplate->ClearFlags(RF_Transient);
			ActorTemplate->SetFlags(RF_Public | RF_Transactional);
		}
	}

	MarkPackageDirty();
}

void UDialogSequenceSlot::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

UDialogStage::UDialogStage()
{
	bSnapToGround = true;
}

UDialogBuilderNode_DialogSequence* UDialogStage::GetOwningDialogSequenceNode() const
{
	return GetTypedOuter<UDialogBuilderNode_DialogSequence>();
}


UDialogStage* UDialogStage::GetOwningDialogStage() const
{
	UDialogBuilderNode_DialogSequence* SequenceNode = GetOwningDialogSequenceNode();
	if (SequenceNode)
	{
		return SequenceNode->DialogStage;
	}
	return nullptr;
}

void UDialogStage::MakeUniqueDialogStageName()
{
	if (!OwningDialogGraph)
	{
		return;
	}

	if (!OwningDialogGraph->DialogStages.Contains(this))
	{
		return;
	}

	const FString BaseName = Name.ToString();
	const FString CleanBaseName = BaseName.IsEmpty() ? TEXT("Stage") : BaseName;

	auto IsNameTaken = [this](const FString& InCandidateName) -> bool
		{
			if (!OwningDialogGraph)
			{
				return false;
			}

			for (const TObjectPtr<UDialogStage>& StagePtr : OwningDialogGraph->DialogStages)
			{
				const UDialogStage* OtherStage = StagePtr.Get();
				if (!OtherStage || OtherStage == this)
				{
					continue;
				}

				if (OtherStage->Name.ToString().Equals(InCandidateName, ESearchCase::CaseSensitive))
				{
					return true;
				}
			}

			return false;
		};

	if (!IsNameTaken(CleanBaseName))
	{
		return;
	}

	int32 Suffix = 2;
	FString UniqueName;

	do
	{
		UniqueName = FString::Printf(TEXT("%s%d"), *CleanBaseName, Suffix++);
	} while (IsNameTaken(UniqueName));

	Name = FText::FromString(UniqueName);
	MarkPackageDirty();
}

#if WITH_EDITOR
void UDialogStage::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty &&
		PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UDialogStage, Name))
	{
		MakeUniqueDialogStageName();
	}
}
#endif

#undef LOCTEXT_NAMESPACE

