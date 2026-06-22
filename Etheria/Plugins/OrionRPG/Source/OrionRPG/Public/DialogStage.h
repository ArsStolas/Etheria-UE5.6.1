// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogData.h"
#include "NativeGameplayTags.h"
#include "Engine/PointLight.h"
#include "UObject/NoExportTypes.h"
#include "DialogStage.generated.h"

class UDialogBuilderGraph;

UCLASS(Blueprintable, BlueprintType, EditInlineNew, AutoExpandCategories = "Detail")
class ORIONRPG_API UDialogSequenceSlot : public UObject
{
	GENERATED_BODY()

public:
	UDialogSequenceSlot();

	UPROPERTY(BlueprintReadOnly, Category = "Detail")
	TObjectPtr<UDialogBuilderGraph> OwningDialogGraph;

	//id used for binding reference in sequence
	UPROPERTY(BlueprintReadOnly, Category = "Detail", meta = (DisplayPriority = -1))
	FGuid ID;

	UPROPERTY(BlueprintReadOnly, Category = "Detail")
	TObjectPtr<class UDialogDefinition> DialogDefinition;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detail")
	FVector SlotLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detail")
	FRotator SlotRotation;
	
    	

	FName GetID() const
	{
		FName OutName = FName(*ID.ToString());
		return OutName;
	}

	UDialogStage* GetOwningDialogStage() const;

	FGameplayTag GetActorTag() const;

	void SetDialogDefinition(UDialogDefinition* InDialogDefinition);

	// The serialized template object that stores saved properties
	UPROPERTY(BlueprintReadOnly, Instanced, Category = "Detail")
	TObjectPtr<AActor> ActorTemplate;

	// Helper to create or update the template
	void UpdateActorTemplate(AActor* InActor);

#if WITH_EDITOR
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface
#endif
};

UCLASS(Blueprintable, BlueprintType, EditInlineNew, AutoExpandCategories = "Detail")
class ORIONRPG_API UDialogSequenceSlot_Light : public UDialogSequenceSlot
{
	GENERATED_BODY()

public:
	UDialogSequenceSlot_Light();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detail")
	TSubclassOf<class ALight> LightClass;

	/** 
	 * Total energy that the light emits.  
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category=Light, meta=(DisplayName = "Intensity", ShouldShowInViewport = true))
	float Intensity;
	
	/** 
		 * Units used for the intensity. 
		 * The peak luminous intensity is measured in candelas, while the luminous flux is measured in lumens.
		 * When the units are set in Nits, the light's power is also determined by the size of the light source (larger sources will emit more light).
		 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, meta=(DisplayName="Intensity Units"))
	ELightUnits IntensityUnits;

	/** 
	 * Filter color of the light.
	 * Note that this can change the light's effective intensity.
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category=Light, meta=(HideAlphaChannel, ShouldShowInViewport = true))
	FColor LightColor;

	/**
	 * Bounds the light's visible influence.  
	 * This clamping of the light's influence is not physically correct but very important for performance, larger lights cost more.
	 */
	UPROPERTY(interp, BlueprintReadOnly, Category=Light, meta=(UIMin = "8.0", UIMax = "16384.0", SliderExponent = "5.0", ShouldShowInViewport = true))
	float AttenuationRadius;
	
	
	/** false: use white (D65) as illuminant. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, meta=(DisplayName = "Use Temperature", ShouldShowInViewport = true))
	uint32 bUseTemperature : 1;

	/**
	* Color temperature in Kelvin of the blackbody illuminant.
	* White (D65) is 6500K.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, interp, Category = Light, meta = (UIMin = "1700.0", UIMax = "12000.0", ShouldShowInViewport = true, DisplayAfter ="bUseTemperature"))
	float Temperature;
	
	
	/** 
	 * Whether the light can affect the world, or whether it is disabled.
	 * A disabled light will not contribute to the scene in any way.  This setting cannot be changed at runtime and unbuilds lighting when changed.
	 * Setting this to false has the same effect as deleting the light, so it is useful for non-destructive experiments.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, meta = (ShouldShowInViewport = true))
	uint32 bAffectsWorld:1;

	/**
	 * Whether the light should cast any shadows.
	 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light)
	uint32 CastShadows:1;

	/** 
	 * Scales the indirect lighting contribution from this light. 
	 * A value of 0 disables any GI from this light. Default is 1.
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category=Light, meta=(UIMin = "0.0", UIMax = "6.0"))
	float IndirectLightingIntensity;

	/** Intensity of the volumetric scattering from this light.  This scales Intensity and LightColor. */
	UPROPERTY(BlueprintReadOnly, interp, Category=Light, meta=(UIMin = "0.25", UIMax = "4.0"))
	float VolumetricScatteringIntensity;
	
	/**
	 * Whether the light should cast shadows from static objects.  Also requires Cast Shadows to be set to True.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, AdvancedDisplay)
	uint32 CastStaticShadows:1;

	/**
	 * Whether the light should cast shadows from dynamic objects.  Also requires Cast Shadows to be set to True.
	 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, AdvancedDisplay)
	uint32 CastDynamicShadows:1;

	/** Whether the light affects translucency or not.  Disabling this can save GPU time when there are many small lights. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, AdvancedDisplay)
	uint32 bAffectTranslucentLighting:1;

	/** Whether the light shadows volumetric fog.  Disabling this can save GPU time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light, AdvancedDisplay)
	uint32 bCastVolumetricShadow : 1;

	/**
	 * Whether the light should cast high quality hair-strands self-shadowing. When this option is enabled, an extra GPU cost for this light. 
	 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light, AdvancedDisplay)
	uint32 bCastDeepShadow : 1;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light, meta = (DisplayName = "Cast Ray Traced Shadows"), AdvancedDisplay)
	TEnumAsByte<ECastRayTracedShadow::Type> CastRaytracedShadow;

	/** Whether the light affects objects in reflections, when ray-traced reflection is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light, AdvancedDisplay, meta = (DisplayName = "Affect Ray Tracing Reflections"))
	uint32 bAffectReflection : 1;

	/** Whether the light affects global illumination, when ray-traced global illumination is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light, AdvancedDisplay, meta = (DisplayName = "Affect Ray Tracing Global Illumination"))
	uint32 bAffectGlobalIllumination : 1;

	/**
	 *Change the deep shadow layers distribution 0:linear distribution (uniform layer distribution), 1:exponential (more details on near small details).
	 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Light, AdvancedDisplay, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DeepShadowLayerDistribution;
};


UCLASS(Blueprintable, BlueprintType, EditInlineNew, AutoExpandCategories = "Detail")
class ORIONRPG_API UDialogStage : public UObject
{
	GENERATED_BODY()

public:
	UDialogStage();


	UPROPERTY(BlueprintReadOnly, Category = "Detail")
	TObjectPtr<UDialogBuilderGraph> OwningDialogGraph;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Detail")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detail")
	FVector Location;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detail")
	FRotator Rotation;

	/**Try to find a safe ground placement when dialog begins*/
	UPROPERTY(BlueprintReadOnly, Category = "Detail")
	bool bSnapToGround;
	
	UPROPERTY()
	bool bIsTemplate = false;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Detail", meta = (EditCondition = "bIsTemplate == true", HideEditConditionToggle, EditConditionHides))
	TArray<TObjectPtr<class UDialogSequenceSlot>> Slots;

	UPROPERTY(Instanced, BlueprintReadOnly, Category = "Detail")
	TArray<TObjectPtr<class UDialogSequenceSlot>> CameraSlots;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Detail", meta = (EditCondition = "bIsTemplate == true", HideEditConditionToggle, EditConditionHides))
	TArray<TObjectPtr<class UDialogSequenceSlot_Light>> LightSlots;

public:
	class UDialogBuilderNode_DialogSequence* GetOwningDialogSequenceNode() const;
	UDialogStage* GetOwningDialogStage() const;
	void MakeUniqueDialogStageName();


#if WITH_EDITOR
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface
#endif
};

