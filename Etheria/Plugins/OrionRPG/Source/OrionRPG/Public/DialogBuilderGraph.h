// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Containers/Map.h"
#include "DialogData.h"
#include "Camera/PlayerCameraManager.h"
#include "DialogBuilderGraph.generated.h"

class UDialogComponent;
class UDialogBuilderNode;
class UDialogBuilderEdge;
class AController;
class APawn;
class UDialogBuilderEdGraph;
class UDialogBuilderNode_DialogLine;



UCLASS(Blueprintable, BlueprintType, AutoExpandCategories = "Configuration|Player", AutoExpandCategories = "Configuration|Dialog")
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
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Configuration|Player")
		FText PlayerName;

	UPROPERTY(EditAnywhere, Category = "Configuration|Player", meta = (InlineEditConditionToggle))
		bool bOverrideTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Player", meta = (EditCondition = "bOverrideTransform"))
		FTransform PlayerTransformOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Configuration|Player", meta = (NoResetToDefault))
		TObjectPtr<class UDialogCameraShot> DefaultPlayerShot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Configuration|Player", meta = (NoResetToDefault))
		TObjectPtr<class UDialogCameraShot> DefaultSelectingChoiceShot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", Category = "Configuration|Player", DisplayThumbnail = "true", AllowedClasses = "/Script/Engine.Texture,/Script/Engine.MaterialInterface,/Script/Engine.SlateTextureAtlasInterface", DisallowedClasses = "/Script/MediaAssets.MediaTexture"))
		TObjectPtr<UObject> DefaultPlayerImage;

	/*
	* Rotate all participant to whoever is currently speaking
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Dialog")
		bool bAutoRotateParticipant;

	/**Auto advance dialog automatically when current dialog line has finished*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Dialog")
		bool bAutoAdvanceDialogLine;

	/**Lock player movement when dialog in progress.
	* If set to false, auto advance dialog and won't show mouse cursor.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Dialog")
		bool bLockPlayerMovement;

	/**
	* Keep participant position after the dialog ends
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configuration|Dialog")
		bool bKeepPosition;

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

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
		bool bUseDialogCameraFade;

	UPROPERTY(BlueprintReadOnly, Category = "Camera", meta = (EditCondition = "bUseDialogCameraFade == true", HideEditConditionToggle, EditConditionHides))
		float FadeDuration;

	UPROPERTY(BlueprintReadOnly, Category = "Camera", meta = (EditCondition = "bUseDialogCameraFade == true", HideEditConditionToggle, EditConditionHides))
		FLinearColor FadeColor;

	UPROPERTY(BlueprintReadOnly, Category = "Audio", meta = (EditCondition = "bUseDialogCameraFade == true", HideEditConditionToggle, EditConditionHides))
		class USoundAttenuation* DialogSoundAttenuation;
	


	UPROPERTY(BlueprintReadWrite, Category = "DialogBuilderGraph")
		TObjectPtr<APlayerController> OwningController;

	UPROPERTY(BlueprintReadWrite, Category = "DialogBuilderGraph")
		TObjectPtr < UDialogComponent> DialogComponent;

	/** List of All Visited Node IDs in Sequence*/
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TArray<FName> VisitedNodeIDs;

	/** List of all participant Info in this dialog*/
	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TMap<FGameplayTag, FParticipantInfo> ParticipantInfoMap;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TMap<FGameplayTag, TObjectPtr<AActor>> CachedParticipants;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TArray<TObjectPtr<AActor>> SpawnedParticipants;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TObjectPtr<UDialogBuilderNode> CurrentNode;

	UPROPERTY(BlueprintReadWrite, Category = "Dialog")
		TObjectPtr<class UDialogBuilderNode_DialogLine> CurrentLine;

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
		TObjectPtr<class ALevelSequenceActor> DialogSequencer;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		bool bOptionSelectionActive;

	//The cinecam spawned in by the sequence 
	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		TWeakObjectPtr<class ACineCameraActor> Cinecam;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		TWeakObjectPtr<class AActor> CachedLastSpeaker;

	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		TWeakObjectPtr<class UDialogCameraShot> CachedLastShot;
		
	UPROPERTY(BlueprintReadOnly, Category = "Dialog")
		class UAudioComponent* DialogAudio;

	protected:
		FTimerHandle DialogLineTimer;
public:
	virtual bool Initialize(UDialogComponent* InitializingComp);
	virtual void Deinitialize();
	virtual UWorld* GetWorld() const override;

	void StartFromRoot();

	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void BeginNode(UDialogBuilderNode* InNode, bool bLaunchEventOnLoad = false);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void BeginDialogLine(class UDialogBuilderNode_DialogLine* InDialogLine);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void ApplyRotationSetting(class UDialogBuilderNode_DialogLine* InDialogLine, AActor* Speaker, AActor* Listener);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void DetermineDialogShot(class UDialogBuilderNode_DialogLine* InDialogLine, AActor* Speaker, AActor* Listener);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void BeginChoiceSelection(class UDialogBuilderNode_DialogLine* InDialogLine);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void PlayDialogShot(class UDialogCameraShot* InDialogShot, AActor* InSpeaker);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void PlayDialogSequence(const FDialogLineData& InDialogLineData);
	
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

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
		virtual void AdvanceDialogLine();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
		virtual void StopCurrentDialogLine();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
		virtual void EndDialog();

	UFUNCTION()
	virtual void DetermineAdvanceDialogRule(class UDialogBuilderNode_DialogLine* InDialogLine);

	UFUNCTION()
	virtual FVector GetSpeakerBoneLocation(class AActor* Actor, bool bOverrideTrackedBone = false, FName BoneOverride = NAME_None) const;

protected:
	virtual AActor* RetrieveParticipant(const FParticipantInfo& Info);
	
#if WITH_EDITORONLY_DATA
public:
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

};


