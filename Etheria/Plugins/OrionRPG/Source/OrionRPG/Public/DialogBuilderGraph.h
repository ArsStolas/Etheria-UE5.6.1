// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Containers/Map.h"
#include "DialogData.h"
#include "DialogDefinition.h"
#include "Camera/PlayerCameraManager.h"
#include "DialogBuilderGraph.generated.h"

class UDialogComponent;
class UDialogDefinition;
class UDialogSequenceSlot;
class UDialogBuilderNode;
class UDialogBuilderEdge;
class AController;
class APawn;
class UDialogBuilderEdGraph;
class UDialogBuilderNode_DialogLine;



UCLASS(Blueprintable, BlueprintType, AutoExpandCategories = "Camera|Config", AutoExpandCategories = "Configuration|Dialog")
class ORIONRPG_API UDialogBuilderGraph : public UObject
{
	GENERATED_BODY()
public:
	UDialogBuilderGraph();
	virtual ~UDialogBuilderGraph();

	/** AssetID */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dialog")
		FName ID;

	/** Your Player Name*/
	UPROPERTY(BlueprintReadOnly, Category = "Configuration|Player")
		FText PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Configuration|Player", meta = (InlineEditConditionToggle))
		bool bOverrideTransform;

	UPROPERTY(BlueprintReadOnly, Category = "Configuration|Player", meta = (EditCondition = "bOverrideTransform"))
		FTransform PlayerTransformOverride;

	UPROPERTY(BlueprintReadOnly, Instanced, Category = "Configuration|Player", meta = (NoResetToDefault))
		TObjectPtr<class UDialogCameraShot> DefaultPlayerShot;

	UPROPERTY(BlueprintReadOnly, Instanced, Category = "Configuration|Player", meta = (NoResetToDefault))
		TObjectPtr<class UDialogCameraShot> DefaultSelectingChoiceShot;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true", Category = "Configuration|Player", DisplayThumbnail = "true", AllowedClasses = "/Script/Engine.Texture,/Script/Engine.MaterialInterface,/Script/Engine.SlateTextureAtlasInterface", DisallowedClasses = "/Script/MediaAssets.MediaTexture"))
		TObjectPtr<UObject> DefaultPlayerImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Dialog")
		EDialogType DialogType;
	
	/*
	* Rotate all participant to whoever is currently speaking
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Dialog")
		bool bAutoRotateParticipant;

	/**Auto advance dialog automatically when current dialog line has finished*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Dialog")
		bool bAutoAdvanceDialogLine;


	/**
	* Restore all actor transform after the dialog ends
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Dialog")
		bool bRestoreState;

	/**
	* Can this dialog be interrupted by another dialog (if another dialog is triggered while this one is active)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Dialog", meta = (DisplayName = "Can Interrupt this Dialog"))
		bool bCanInterruptDialog;

	/*
	* By default dialog camera will aim at the bone set  in this variable
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Dialog")
		FName DefaultBoneToTrack;


	/**Camera*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
		TSubclassOf<class UCameraShakeBase> DialogCameraShake;

	/**Disable camera cuts for dialog sequence*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
		bool bDisableCameraCuts;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
		bool bBlendCameraOnDialogEnd;

	/**time taken to blend*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (EditCondition = "bBlendCameraOnDialogEnd == true", HideEditConditionToggle, EditConditionHides))
		float BlendTime;

	/** Cubic, Linear etc functions for blending*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (EditCondition = "bBlendCameraOnDialogEnd == true", HideEditConditionToggle, EditConditionHides))
		TEnumAsByte<EViewTargetBlendFunction> BlendFunc;

	/**Exponent, used by certain blend functions to control the shape of the curve. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (EditCondition = "bBlendCameraOnDialogEnd == true", HideEditConditionToggle, EditConditionHides))
		float BlendExp;

	/**Fade camera before starting dialog*/
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
		bool bFadeCameraOnDialogBegin;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
		class USoundAttenuation* DialogSoundAttenuation;
	


	UPROPERTY(BlueprintReadWrite, Category = "DialogBuilderGraph")
		TObjectPtr<APlayerController> OwningController;

	UPROPERTY(BlueprintReadWrite, Category = "DialogBuilderGraph")
		TObjectPtr < UDialogComponent> DialogComponent;

	/** List of All Visited Node IDs in Sequence*/
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TArray<FName> VisitedNodeIDs;

	/*------------------------Dialog Sequence------------------------*/
	UPROPERTY(BlueprintReadOnly, Category = "Detail")
		TObjectPtr<class UDialogSequence> CurrentDialogSequence;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TArray<TObjectPtr<class UDialogParticipant>> ParticipantDefinitions;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TObjectPtr<class UDialogPlayerParticipant> PlayerParticipantDefinition;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TArray<TObjectPtr<class UDialogProp>> PropDefinitions;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TArray<TObjectPtr<class UDialogCamera>> CameraDefinitions;


	/**List of dialog set used in this graph*/
	UPROPERTY(Instanced, BlueprintReadOnly, Category = "Detail")
		TArray<TObjectPtr<class UDialogStage>> DialogStages;

	UPROPERTY(Instanced, BlueprintReadOnly, Category = "Detail")
		TObjectPtr<class UDialogStage> CurrentDialogStage;

	UPROPERTY(Transient)
		TMap<UDialogSequenceSlot*, TWeakObjectPtr<AActor>> DialogSlotActors;

	/*------------------------End Dialog Sequence------------------------*/

	/** List of all participant Info in this dialog*/
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TMap<FGameplayTag, FParticipantInfo> ParticipantInfoMap;

	/**Cached actor reference of this dialog, each actor has their own unique tags*/
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TMap<FName, TObjectPtr<AActor>> CachedActorMap;

		/**List of spawned actor in dialog, this specific actors will be destroyed after dialog ended*/
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TArray<TObjectPtr<AActor>> SpawnedDialogActors;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TObjectPtr<AActor> SequencePivot;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TObjectPtr<AActor> SequenceCameraActor;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TObjectPtr<UDialogBuilderNode> CurrentNode;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TObjectPtr<class UDialogBuilderNode_DialogLine> CurrentLine;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		FOrionDialogLine CurrentDialogLine;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		TArray< TObjectPtr<UDialogBuilderNode>> RootNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		TArray< TObjectPtr<UDialogBuilderNode>> AllNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		TMap<FName, TObjectPtr<UDialogBuilderNode>> NodeMap;

	/**Latest node before entering selection node*/
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TObjectPtr<UDialogBuilderNode> LatestRootSelectionNode;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		TObjectPtr<class ALevelSequenceActor> DialogSequenceActor;

	//The cinecam spawned in by the sequence 
	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		TWeakObjectPtr<class ACineCameraActor> Cinecam;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		TWeakObjectPtr<class AActor> CachedLastSpeaker;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		TWeakObjectPtr<class UDialogCameraShot> CachedLastShot;
		
	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		class UAudioComponent* DialogAudio;


public:
	/*------------------------Default Camera Settings------------------------*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Config")
	ECameraFocusMethod FocusMethod;

	/** Current focal length of the camera (i.e. controls FoV, zoom) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Config")
	float FocalLength;

	/** Current aperture, in terms of f-stop (e.g. 2.8 for f/2.8) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Config")
	float Aperture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Camera|Config", meta = (InlineEditConditionToggle))
	bool bOverride_CustomNearClippingPlane;

	/** Set bOverride_CustomNearClippingPlane to true if you want to use a custom clipping plane instead of GNearClippingPlane. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Camera|Config", meta = (UIMin = "0.00001", ClampMin = "0.00001", editcondition = "bOverride_CustomNearClippingPlane"))
	float CustomNearClippingPlane;

	/** If bConstrainAspectRatio is true, black bars will be added if the destination view has a different aspect ratio than this camera requested. */
	UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Camera|Config", meta = (InlineEditConditionToggle))
	bool bConstrainAspectRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Config", meta = (EditCondition = "bConstrainAspectRatio"))
	struct FPlateCropSettings CropSettings;

	/** Controls the filmback of the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Config")
	FCameraFilmbackSettings Filmback;

	/** Controls the camera's lens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Config")
	FCameraLensSettings LensSettings;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle), Category = "Camera|Config")
	bool bUsePostProcess = false;

	/**Camera post process override*/
	UPROPERTY(Interp, BlueprintReadWrite, Category = "Camera|Config", meta = (EditCondition = "bUsePostProcess"))
	struct FPostProcessSettings PostProcessSettings;
/*------------------------End Default Camera Settings------------------------*/


	bool bCanAdvanceDialog = false;
public:
	FSimpleMulticastDelegate OnDialogSetupFinished;

	public:
	void StartSetupPrerequisites();
	void CheckDialogPrerequisites();

protected:
	void SetupPrerequisites();

protected:
	FTimerHandle TimerHandle_CheckPrerequisites;
	bool bSetupCompleted = false;
	bool bPrerequisitesRequested = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDialogDefinition>> PrerequisiteDefinitions;

protected:
	UFUNCTION()
	void HandleDialogSequenceFinished();

	void CollectDialogDefinitionsForSetup(TArray<UDialogDefinition*>& OutDefinitions) const;
	void OnSoftClassesLoaded();

	FTimerHandle DialogLineTimer;
public:
	virtual bool Initialize(UDialogComponent* InitializingComp);
	virtual void Deinitialize();
	virtual UWorld* GetWorld() const override;

	void StartFromRoot();


	UFUNCTION(BlueprintPure, Category = "Dialog")
	bool ShouldLockPlayerMovement();
	
	UFUNCTION()
	virtual void DialogLineBegin(FOrionDialogLine InDialogLine);

	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void BeginNode(UDialogBuilderNode* InNode, bool bLaunchEventOnLoad = false);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void BeginDialogLine(class UDialogBuilderNode_DialogLine* InDialogLine);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void ApplyRotationSetting(class UDialogBuilderNode_DialogLine* InDialogLine, AActor* Speaker, AActor* Listener);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void DetermineDialogShot(class UDialogBuilderNode_DialogLine* InDialogLine, AActor* Speaker, AActor* Listener);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void BeginChoiceSelection(class UDialogBuilderNode_PlayerChoice* InPlayerChoice);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void BeginDialogSequence(class UDialogBuilderNode_DialogSequence* InDialogSequence);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void PlayDialogShot(class UDialogCameraShot* InDialogShot, AActor* InSpeaker);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void PlaySequence(class ULevelSequence* InSequence, FMovieSceneSequencePlaybackSettings InPlaybackSetting);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void JumpToNextDialogSection();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	FText GetParticipantName(FGameplayTag ParticipantTag) const;

	UFUNCTION(BlueprintCallable, Category = "Dialog")
	int GetLevelNum() const;

	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void GetNodesByLevel(int Level, TArray<UDialogBuilderNode*>& Nodes);

	void ClearGraph();
	
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	APlayerController* GetOwningController();

	UFUNCTION(BlueprintCallable, Category = "Dialog")
	APawn* GetOwningPawn();

	UFUNCTION(BlueprintCallable, Category = "Dialog")
	UDialogComponent* GetDialogComponent();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsChoiceSelectionActive() const;

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool CanAdvanceDialog() const;

	UFUNCTION(BlueprintCallable, Category = "Dialog")
		virtual void AdvanceDialogLine();

	UFUNCTION(BlueprintCallable, Category = "Dialog")
		virtual void StopCurrentDialogLine();

	UFUNCTION(BlueprintCallable, Category = "Dialog")
		virtual void EndDialog();

	UFUNCTION(BlueprintCallable, Category = "Dialog")
		AActor* GetDialogDefinitionActor(UDialogDefinition* InDialogDefinition);

	UFUNCTION(BlueprintCallable, Category = "Dialog")
	UDialogDefinition* GetDialogDefinitionByTag(FGameplayTag InTag);

	UFUNCTION()
	virtual void DetermineAdvanceDialogRule(class UDialogBuilderNode* InDialogNode);

	UFUNCTION()
	virtual FVector GetSpeakerBoneLocation(class AActor* Actor, bool bOverrideTrackedBone = false, FName BoneOverride = NAME_None) const;

	/**Ensure player definition exist for this graph, create a new one if null*/
	UFUNCTION()
	void EnsurePlayerDefinition();

protected:
	virtual AActor* RetrieveParticipant(FParticipantInfo& Info);
	virtual AActor* RetrieveActorFromSlot(class UDialogSequenceSlot* InSlot, FName SlotID);
	virtual AActor* RetrieveCameraFromSlot(class UDialogSequenceSlot* InSlot, FName SlotID);
	virtual AActor* RetrieveLightFromSlot(class UDialogSequenceSlot_Light* InSlot, FName SlotID);
	void UpdateLightSlotProperty(class UDialogSequenceSlot_Light* InLightSlot, AActor* InActor);

	void InitializeDialogActors();
	void SetDialogStage(class UDialogStage* InDialogStage);	
	void DestroyDialogSequenceActor();

	/** Cached actor original transforms, used to restore actor transforms after dialog ends */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Dialog")
	TMap<FName, FTransform> CachedActorOriginalTransforms;

protected:
	void CacheOriginalActorTransform(const FName& ActorId, AActor* Actor);
	void RestoreCachedActorTransforms();

#if WITH_EDITORONLY_DATA
public:
	UPROPERTY()
	FTransform CachedCameraTransform;

	UPROPERTY()
	TObjectPtr<class UDialogBuilderNode_DialogSequence> CurrentEditingSequenceNode;
	/** Set of dialog-graph pages */
	UPROPERTY()
	TArray<TObjectPtr<UEdGraph>> DialogGraphPages;

	/** Set of documents that were being edited in this blueprint, so we can open them right away */
	UPROPERTY()
	TArray<struct FEditedDocumentInfo> LastEditedDocuments;

	/** Whether or not this blueprint is newly created, and hasn't been opened in an editor yet */
	UPROPERTY(/*transient*/ BlueprintReadOnly, Category = "DialogBuilderGraph")
	bool bIsNewlyCreated = true;


	/** Get all graphs in this DialogEditor */
	void GetAllGraphs(TArray<UEdGraph*>& Graphs) const;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	template<typename TNodeType>
	void GetNodesOfType(TArray<TNodeType*>& OutNodes) const
	{
		OutNodes.Reset();

		for (UDialogBuilderNode* Node : AllNodes)
		{
			if (TNodeType* TypedNode = Cast<TNodeType>(Node))
			{
				OutNodes.Add(TypedNode);
			}
		}
	}

	void GetNodesOfType(TSubclassOf<UDialogBuilderNode> NodeType, TArray<UDialogBuilderNode*>& Nodes) const;
};













