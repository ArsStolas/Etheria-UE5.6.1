// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "DialogBuilderGraph.h"

#include "DialogComponent.h"
#include "MovieSceneDialogSection.h"
#include "MovieSceneDialogTrack.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderEdge.h"
#include "DialogCameraShot.h"
#include "DialogSequence.h"
#include "DialogStage.h"
#include "DialogBuilderNode_Root.h"
#include "DialogBuilderSetting.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "DialogBuilderNode_RerouteNode.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogBuilderNode_PlayerChoice.h"
#include "DialogBuilderNode_DialogSequence.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include <CineCameraActor.h>
#include <DefaultLevelSequenceInstanceData.h>
#include "Components/LocalLightComponent.h"
#include "Components/LightComponent.h"
#include <CineCameraComponent.h>
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraShakeBase.h"
#include "Components/LightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Engine/AssetManager.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"

#define LOCTEXT_NAMESPACE "DialogBuilderGraph"


UDialogBuilderGraph::UDialogBuilderGraph()
{
	bAutoRotateParticipant = false;
	bRestoreState = false;
	DefaultBoneToTrack = FName("head");
	bAutoAdvanceDialogLine = false;
	bCanInterruptDialog = true;
	bDisableCameraCuts = false;
	bFadeCameraOnDialogBegin = true;
	bBlendCameraOnDialogEnd = true;

	BlendTime = 1.25f;
	BlendFunc = VTBlend_Cubic;
	BlendExp = 2.0f;

	//Camera Default Setting
	bUsePostProcess = false;
	FocusMethod = ECameraFocusMethod::Tracking;
	FocalLength = 35.0f;
	Aperture = 2.8f;
	bOverride_CustomNearClippingPlane = true;
	CustomNearClippingPlane = 20.f;

	CropSettings.AspectRatio = 2.39f;
	bConstrainAspectRatio = true;

	Filmback.SensorWidth = 23.76f;
	Filmback.SensorHeight = 13.365f;
	Filmback.SensorAspectRatio = 1.777778f;

	LensSettings.MinFocalLength = 4.0f;
	LensSettings.MaxFocalLength = 1000.0f;
	LensSettings.MinFStop = 1.2f;
	LensSettings.MaxFStop = 22.0f;
	LensSettings.SqueezeFactor = 1.0f;
	LensSettings.DiaphragmBladeCount = 7;
}

UDialogBuilderGraph::~UDialogBuilderGraph()
{
	
}



bool UDialogBuilderGraph::Initialize(UDialogComponent* InitializingComp)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		//We need a valid Quest component to make a quest for 
		if (!InitializingComp)
		{
			return false;
		}
		OwningController = InitializingComp->GetOwningController();
		DialogComponent = InitializingComp;
		DialogComponent->OnDialogLineBegin.AddDynamic(this, &UDialogBuilderGraph::DialogLineBegin);


		VisitedNodeIDs.Empty();
		CachedActorMap.Empty();
		CachedActorOriginalTransforms.Empty();
		NodeMap.Empty();
		for (auto& Node : AllNodes)
		{
			if (Node)
			{
				Node->DialogGraph = this;
				Node->OwningController = InitializingComp->GetOwningController();
				Node->DialogComponent = InitializingComp;
				for (auto& Event : Node->Events)
				{
					if (Event)
					{
						Event->BeginSetup(DialogComponent->GetOwningController(), GetOwningPawn());
					}
				}
				NodeMap.Emplace(Node->ID, Node);
			}
		}

		bSetupCompleted = false;
		bPrerequisitesRequested = false;
		PrerequisiteDefinitions.Empty();
		OnDialogSetupFinished.Clear();

		

		if (OwningController && OwningController->IsLocalPlayerController())
		{
			if (DialogCameraShake)
			{
				OwningController->ClientStartCameraShake(DialogCameraShake);
			}
		}
		bAutoAdvanceDialogLine = ShouldLockPlayerMovement() ? bAutoAdvanceDialogLine : true; // if dialog is free movement, force auto advance dialog line -> true
		bFadeCameraOnDialogBegin = ShouldLockPlayerMovement() ? bFadeCameraOnDialogBegin : false;
		
	}
	return false;
}

void UDialogBuilderGraph::InitializeDialogActors()
{
	UWorld* World = GetWorld();
	check(World);
	APawn* PlayerPawn = GetOwningPawn();
	if(!OwningController || !PlayerPawn)
	{
		return;
	}

	//retrieve partcipant
	for (TPair<FGameplayTag, FParticipantInfo>& Pair : ParticipantInfoMap)
	{
		FGameplayTag ParticipantTag = Pair.Key;
		FParticipantInfo& ParticipantInfo = Pair.Value;
		if (AActor* ParticipantActor = RetrieveParticipant(ParticipantInfo))
		{
			const FName ParticipantId = ParticipantInfo.GetID();
			CachedActorMap.Emplace(ParticipantId, ParticipantActor);
			CacheOriginalActorTransform(ParticipantId, ParticipantActor);
		}
	}


	if (!SequencePivot)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.bNoFail = true;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* PivotActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
		
		if (PivotActor)
		{
			USceneComponent* PivotRoot = NewObject<USceneComponent>(PivotActor, TEXT("PivotRoot"));
			PivotActor->SetRootComponent(PivotRoot);
			PivotRoot->RegisterComponent();

			PivotActor->SetActorHiddenInGame(true);
			PivotActor->SetActorEnableCollision(false);

			SequencePivot = PivotActor;
			SpawnedDialogActors.Add(PivotActor);
		}
	}

	for (UDialogStage* DialogStage : DialogStages)
	{
		if (!DialogStage)
		{
			continue;
		}

		for (UDialogSequenceSlot* Slot : DialogStage->Slots)
		{
			//Find actor in world from slot information
			UDialogDefinition* DialogDefinition = Slot ? Slot->DialogDefinition : nullptr;
			if (!DialogDefinition) continue;

			FName SlotId = Slot->GetActorTag().GetTagName();

			AActor* OutActor = nullptr;
			TArray<AActor*> FoundActors;
			if (SlotId == TAG_Dialog_Participant_Player.GetTag().GetTagName())
			{
				CachedActorMap.Emplace(SlotId, PlayerPawn);
				continue;
			}

			UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), DialogDefinition->GetActorClass(), SlotId, FoundActors);

			if (FoundActors.Num() > 1 && GetOwningPawn())
			{
				float Dist;

				OutActor = UGameplayStatics::FindNearestActor(GetOwningPawn()->GetActorLocation(), FoundActors, Dist);
			}
			else if (FoundActors.IsValidIndex(0))
			{
				OutActor = FoundActors[0];
			}

			CachedActorMap.Emplace(SlotId, OutActor);

		}

	}


	
}

void UDialogBuilderGraph::SetDialogStage(UDialogStage* InDialogStage)
{
	if (!CurrentDialogSequence) return;

	if (!InDialogStage || InDialogStage == CurrentDialogStage)
		return;

	const UMovieScene* MovieScene = CurrentDialogSequence ? CurrentDialogSequence->GetMovieScene() : nullptr;
	if (!MovieScene)
	{
		return;
	}

	

	SequencePivot->SetActorLocation(InDialogStage->Location);
	SequencePivot->SetActorRotation(InDialogStage->Rotation);
	UDialogStage* LastDialogStage = CurrentDialogStage;
	CurrentDialogStage = InDialogStage;


	//================================Begin reset slots================================
	if(LastDialogStage && CurrentDialogStage)
	{
		UDialogStage* OwningLastDialogStage = LastDialogStage->GetOwningDialogStage();
		UDialogStage* OwningCurrentDialogStage = CurrentDialogStage->GetOwningDialogStage();
		if(OwningLastDialogStage != OwningCurrentDialogStage)
		{
			for (auto& Slot : LastDialogStage->Slots)
			{
				if (!Slot) continue;
				FName SlotId = Slot->GetActorTag().IsValid() ? Slot->GetActorTag().GetTagName() : Slot->GetID();
				if (AActor* CachedActor = CachedActorMap.FindRef(SlotId))
				{
					//make sure only destroy actors that were spawned by this dialog
					if (!SpawnedDialogActors.Contains(CachedActor)) continue;
					SpawnedDialogActors.Remove(CachedActor);
					CachedActorMap.Remove(SlotId);
					DialogSlotActors.Remove(Slot);
					CachedActor->Destroy();
				}
			}
		}
		
	}

	//reset camera
	if (SequenceCameraActor)
	{
		SpawnedDialogActors.Remove(SequenceCameraActor);
		SequenceCameraActor->Destroy();
		SequenceCameraActor = nullptr;
	}

	//Reset last stage, destroy light setup
	for (auto& LightSlot : CurrentDialogStage->LightSlots)
	{
		if (!LightSlot) continue;
		const FName SlotId = LightSlot->GetID();
		if (AActor* CachedActor = CachedActorMap.FindRef(SlotId))
		{
			SpawnedDialogActors.Remove(CachedActor);
			CachedActorMap.Remove(SlotId);
			DialogSlotActors.Remove(LightSlot);
			CachedActor->Destroy();
		}
	}

	
	



	//================================Begin retrieve slots================================
	for (auto& Slot : CurrentDialogStage->Slots)
	{
		if (!Slot) continue;
		//Regular slot uses actor tag for binding
		FName SlotId = Slot->GetActorTag().IsValid() ? Slot->GetActorTag().GetTagName() : Slot->GetID();
		
		if (AActor* DialogActor = RetrieveActorFromSlot(Slot, SlotId))
		{
			CachedActorMap.Emplace(SlotId, DialogActor);
			DialogSlotActors.Emplace(Slot, DialogActor);
			CacheOriginalActorTransform(SlotId, DialogActor);
			
			CurrentDialogSequence->UnbindPossessableObjects(Slot->ID);
			CurrentDialogSequence->BindPossessableObject(Slot->ID, *DialogActor, DialogActor->GetClass());
		}
	}
		

	for (auto& CameraSlot : CurrentDialogStage->CameraSlots)
	{
		if (!CameraSlot) continue;

		if (!CameraSlot->DialogDefinition)
		{
			CameraSlot->DialogDefinition = NewObject<UDialogCamera>(this, NAME_None, RF_Transactional);
		}
		//Camera uses slot ID for binding
		const FName SlotId = CameraSlot->GetID();
		SequenceCameraActor = RetrieveCameraFromSlot(CameraSlot, SlotId);
		if (SequenceCameraActor)
		{
			CurrentDialogSequence->UnbindPossessableObjects(CameraSlot->ID);
			CurrentDialogSequence->BindPossessableObject(CameraSlot->ID, *SequenceCameraActor, SequenceCameraActor->GetClass());
		}
	}

	for (auto& LightSlot : CurrentDialogStage->LightSlots)
	{
		if (!LightSlot) continue;
		//Camera uses slot ID for binding
		const FName SlotId = LightSlot->GetID();
		if (AActor* DialogActor = RetrieveLightFromSlot(LightSlot, SlotId))
		{
			CachedActorMap.Emplace(SlotId, DialogActor);
			DialogSlotActors.Emplace(LightSlot, DialogActor);
		}
	}

	
}

void UDialogBuilderGraph::DestroyDialogSequenceActor()
{
	if (DialogSequenceActor)
	{
		if (ULevelSequencePlayer* SP = DialogSequenceActor->GetSequencePlayer())
		{
			SP->OnFinished.RemoveAll(this);
			SP->Stop();
		}
		DialogSequenceActor->Destroy();
		DialogSequenceActor = nullptr;
	}
}



void UDialogBuilderGraph::Deinitialize()
{
	VisitedNodeIDs.Empty();
	CurrentNode = nullptr;
	
	for (auto& Node : AllNodes)
	{
		Node->Deinitialize();
	}
}

UWorld* UDialogBuilderGraph::GetWorld() const
{
	if (DialogComponent)
	{
		return DialogComponent->GetWorld();
	}

	return nullptr;
}

bool UDialogBuilderGraph::ShouldLockPlayerMovement()
{
	bool bLockPlayerMovement = true;
	switch (DialogType)
	{
	case EDialogType::E_CinematicDialog:
		bLockPlayerMovement = true;
		break;
	case EDialogType::E_FreeMovementDialog:
		bLockPlayerMovement = false;
		break;
	}
	return bLockPlayerMovement;
}


void UDialogBuilderGraph::StartFromRoot()
{
	InitializeDialogActors();
	if (RootNodes.IsValidIndex(0))
	{
		BeginNode(RootNodes[0]);
	}
}

void UDialogBuilderGraph::StartSetupPrerequisites()
{
	bSetupCompleted = false;
	bPrerequisitesRequested = false;
	PrerequisiteDefinitions.Empty();

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(TimerHandle_CheckPrerequisites))
		{
			World->GetTimerManager().SetTimer(TimerHandle_CheckPrerequisites, this, &ThisClass::CheckDialogPrerequisites, 0.1f, true);
		}
	}

	CheckDialogPrerequisites();
}

void UDialogBuilderGraph::CheckDialogPrerequisites()
{
	if (!bPrerequisitesRequested)
	{
		SetupPrerequisites();
	}

	if (!bSetupCompleted)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle_CheckPrerequisites);
	}

	OnDialogSetupFinished.Broadcast();
	OnDialogSetupFinished.Clear();
}

void UDialogBuilderGraph::CollectDialogDefinitionsForSetup(TArray<UDialogDefinition*>& OutDefinitions) const
{
	for (UDialogParticipant* Participant : ParticipantDefinitions)
	{
		if (Participant)
		{
			OutDefinitions.AddUnique(Participant);
		}
	}

	for (UDialogProp* Prop : PropDefinitions)
	{
		if (Prop)
		{
			OutDefinitions.AddUnique(Prop);
		}
	}

	for (UDialogCamera* Camera : CameraDefinitions)
	{
		if (Camera)
		{
			OutDefinitions.AddUnique(Camera);
		}
	}

	

	
}

void UDialogBuilderGraph::SetupPrerequisites()
{
	bPrerequisitesRequested = true;
	const TWeakObjectPtr<UDialogBuilderGraph> WeakThis(this);

	TArray<UDialogDefinition*> DefinitionsToLoad;
	CollectDialogDefinitionsForSetup(DefinitionsToLoad);

	PrerequisiteDefinitions.Reset();
	for (UDialogDefinition* DialogDefinition : DefinitionsToLoad)
	{
		PrerequisiteDefinitions.Add(DialogDefinition);
	}

	TArray<FSoftObjectPath> SoftPathsToLoad;

	for (UDialogDefinition* DialogDefinition : DefinitionsToLoad)
	{
		if (!DialogDefinition)
		{
			continue;
		}

		const TSoftClassPtr<AActor> ActorSoftClass = DialogDefinition->GetActorSoftClass();
		if (ActorSoftClass.IsNull())
		{
			continue;
		}


		SoftPathsToLoad.AddUnique(ActorSoftClass.ToSoftObjectPath());
	}

	for (TPair<FGameplayTag, FParticipantInfo>& Pair : ParticipantInfoMap)
	{
		FParticipantInfo& ParticipantInfo = Pair.Value;
		if (ParticipantInfo.ParticipantSetup != EParticipantSetup::E_SpawnParticipant)
		{
			continue;
		}

		const TSoftClassPtr<AActor> ParticipantSoftClass = ParticipantInfo.ParticipantToSpawnSoft;
		if (ParticipantSoftClass.IsNull())
		{
			continue;
		}


		SoftPathsToLoad.AddUnique(ParticipantSoftClass.ToSoftObjectPath());
	}

	if (SoftPathsToLoad.Num() == 0)
	{
		bSetupCompleted = true;
		return;
	}

	UAssetManager::GetStreamableManager().RequestAsyncLoad(
		SoftPathsToLoad,
		FStreamableDelegate::CreateLambda([WeakThis]
			{
				if (UDialogBuilderGraph* DialogGraph = WeakThis.Get())
				{
					DialogGraph->OnSoftClassesLoaded();
				}
			}));
}

void UDialogBuilderGraph::OnSoftClassesLoaded()
{
	for (UDialogDefinition* DialogDefinition : PrerequisiteDefinitions)
	{
		if (!DialogDefinition)
		{
			continue;
		}

		const TSoftClassPtr<AActor> ActorSoftClass = DialogDefinition->GetActorSoftClass();
		if (ActorSoftClass.IsNull())
		{
			continue;
		}

		if (UClass* LoadedClass = ActorSoftClass.Get())
		{
			DialogDefinition->SetActorClass(LoadedClass);
		}
	}

	for (TPair<FGameplayTag, FParticipantInfo>& Pair : ParticipantInfoMap)
	{
		FParticipantInfo& ParticipantInfo = Pair.Value;
		if (ParticipantInfo.ParticipantSetup != EParticipantSetup::E_SpawnParticipant)
		{
			continue;
		}

		const TSoftClassPtr<AActor> ParticipantSoftClass = ParticipantInfo.ParticipantToSpawnSoft;
		if (ParticipantSoftClass.IsNull())
		{
			continue;
		}

		if (UClass* LoadedClass = ParticipantSoftClass.Get())
		{
			ParticipantInfo.ParticipantToSpawn = LoadedClass;
		}
	}

	bSetupCompleted = true;
}

void UDialogBuilderGraph::DialogLineBegin(FOrionDialogLine InDialogLine)
{
	CurrentDialogLine = InDialogLine;
}

void UDialogBuilderGraph::BeginNode(UDialogBuilderNode* InNode, bool bLaunchEventOnLoad)
{
	if (InNode)
	{
		CurrentNode = InNode;
	}
	if (InNode == nullptr)  
	{  
		UE_LOG(LogTemp, Warning, TEXT("BeginNode: Node is null for Dialog ID: %s"), *ID.ToString());  
		return;  
	}
	
	if (UDialogBuilderNode_Root* RootNode = Cast<UDialogBuilderNode_Root>(InNode))
	{
		RootNode->Initialize();
		return;
	}

	//add visited node
	VisitedNodeIDs.Add(InNode->ID);
	
	//InitializeNode
	InNode->Initialize(bLaunchEventOnLoad);
}


void UDialogBuilderGraph::BeginDialogLine(UDialogBuilderNode_DialogLine* InDialogLine)
{
	if (InDialogLine == nullptr)
		return;
	CurrentLine = InDialogLine;

	AActor* Speaker = CachedActorMap.FindRef(InDialogLine->ParticipantInfo.GetID());
	AActor* Listener = CachedActorMap.FindRef(InDialogLine->ListenerTag.GetTagName());

	DetermineAdvanceDialogRule(InDialogLine);

	

	//Dialog Sound
	if (InDialogLine->DialogLineData.DialogSound)
	{
		if (Speaker && ShouldLockPlayerMovement())
		{
			DialogAudio = UGameplayStatics::SpawnSoundAttached(InDialogLine->DialogLineData.DialogSound, Speaker->GetRootComponent(), NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, false, 1.f, 1.f, 0.f, DialogSoundAttenuation);
		}
		else
		{
			DialogAudio = UGameplayStatics::SpawnSound2D(this, InDialogLine->DialogLineData.DialogSound);
			//make sure sound is pauseable
			DialogAudio->SetUISound(false);
		}
	}

	if (!ShouldLockPlayerMovement() && InDialogLine->ParticipantInfo.ParticipantTag == TAG_Dialog_Participant_Player)
		return;

	ApplyRotationSetting(InDialogLine, Speaker, Listener);
	DetermineDialogShot(InDialogLine, Speaker, Listener);

	//Dialog Animation 
	if (Speaker && InDialogLine->DialogLineData.DialogMontage)
	{
		UActorComponent* Mesh = Speaker->GetComponentByClass(USkeletalMeshComponent::StaticClass());
		if (USkeletalMeshComponent* BodyMesh = Cast<USkeletalMeshComponent>(Mesh))
		{
			if (BodyMesh->GetAnimInstance())
			{
				BodyMesh->GetAnimInstance()->Montage_Play(InDialogLine->DialogLineData.DialogMontage);
			}
		}
	}

	//Facial Animation
	if (Speaker && InDialogLine->DialogLineData.FacialAnimation)
	{
		TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		Speaker->GetComponents(SkeletalMeshComponents);

		USkeletalMeshComponent* TargetMesh = nullptr;
		const USkeleton* TargetSkeleton = InDialogLine->DialogLineData.FacialAnimation->GetSkeleton();

		for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
		{
			if (!SkeletalMeshComponent)
			{
				continue;
			}

			if (!TargetMesh)
			{
				TargetMesh = SkeletalMeshComponent;
			}

			const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
			if (SkeletalMesh && TargetSkeleton && SkeletalMesh->GetSkeleton() == TargetSkeleton)
			{
				TargetMesh = SkeletalMeshComponent;
				break;
			}
		}

		if (TargetMesh)
		{
			TargetMesh->PlayAnimation(InDialogLine->DialogLineData.FacialAnimation, false);
		}
	}

	CachedLastSpeaker = Speaker;
}

void UDialogBuilderGraph::ApplyRotationSetting(UDialogBuilderNode_DialogLine* InDialogLine, AActor* Speaker, AActor* Listener)
{
	//Rotation Setting
	if (Speaker && bAutoRotateParticipant)
	{
		for (auto& Participant : CachedActorMap)
		{
			if (Participant.Value && Participant.Value->IsValidLowLevel())
			{
				if (Participant.Value == Speaker)
					continue;
				FVector ToSpeaker = Speaker->GetActorLocation() - Participant.Value->GetActorLocation();
				ToSpeaker.Z = 0; // Ignore pitch for yaw-only rotation
				if (!ToSpeaker.IsNearlyZero())
				{
					FRotator LookAtRotation = ToSpeaker.Rotation();
					if (GetOwningController() && GetOwningController()->GetPawn() == Participant.Value)
					{
						GetOwningController()->SetControlRotation(LookAtRotation);
					}

					Participant.Value->SetActorRotation(LookAtRotation);
					
				}
			}
		}
	}

	if (Speaker && Listener && InDialogLine->bRotateToListener)
	{
		FVector ToListener = Listener->GetActorLocation() - Speaker->GetActorLocation();
		ToListener.Z = 0; // Ignore pitch for yaw-only rotation
		if (!ToListener.IsNearlyZero())
		{
			FRotator LookAtRotation = ToListener.Rotation();
			Speaker->SetActorRotation(LookAtRotation);
		}

		FVector ToSpeaker = Speaker->GetActorLocation() - Listener->GetActorLocation();
		ToSpeaker.Z = 0; // Ignore pitch for yaw-only rotation
		if (!ToSpeaker.IsNearlyZero())
		{
			FRotator LookAtRotation = ToSpeaker.Rotation();
			Listener->SetActorRotation(LookAtRotation);
		}
	}
}

void UDialogBuilderGraph::DetermineDialogShot(UDialogBuilderNode_DialogLine* InDialogLine, AActor* Speaker, AActor* Listener)
{
	if (bDisableCameraCuts) return;
	if (!ShouldLockPlayerMovement()) return;
	

	UDialogCameraShot* DialogShot = nullptr;

	FDialogLineData& DialogLineData = InDialogLine->DialogLineData;
	FParticipantInfo& ParticipantInfo = InDialogLine->ParticipantInfo;

	//Determine the shot
	switch (DialogLineData.DialogCameraMode)
	{
	case EDialogCameraMode::E_GeneratedCameraShot:
		DialogShot = ParticipantInfo.DefaultShot;
		if (!DialogShot ||DialogLineData.ShotOverride)
		{
			//if default shot not valid, try use override shot data instead
			DialogShot = DialogLineData.ShotOverride;
		}

		PlayDialogShot(DialogShot, Speaker);

		break;
	case EDialogCameraMode::E_Sequence:
		PlaySequence(DialogLineData.SequenceToPlay, DialogLineData.PlaybackSettings);
		break;
	}
	CachedLastShot = DialogShot;
}

void UDialogBuilderGraph::BeginChoiceSelection(UDialogBuilderNode_PlayerChoice* InPlayerChoice)
{
	if (!ShouldLockPlayerMovement())
	{
		EndDialog();
		return;
	}
	if (InPlayerChoice == nullptr)
		return;

	bool bShouldUseSequence = InPlayerChoice->SelectionType == EDialogSelectionType::E_Sequence && InPlayerChoice->ShouldPlaySequence();

	UDialogCameraShot* DialogShot = DefaultSelectingChoiceShot;
	AActor* Speaker = CachedActorMap.FindRef(TAG_Dialog_Participant_Player.GetTag().GetTagName());
	Speaker = Speaker ? Speaker : GetOwningPawn();

	if (!DialogShot || InPlayerChoice->SelectingChoiceShotOverride)
	{
		//if default shot not valid, try use override shot data instead
		DialogShot = InPlayerChoice->SelectingChoiceShotOverride;
	}

	if (bShouldUseSequence)
	{
		CurrentDialogSequence = InPlayerChoice->DialogSequence;
		InPlayerChoice->UpdateDialogStageData();

		SetDialogStage(InPlayerChoice->DialogStage);

		PlaySequence(Cast<ULevelSequence>(InPlayerChoice->DialogSequence.Get()), InPlayerChoice->PlaybackSettings);
	}
	else
	{
		PlayDialogShot(DialogShot, Speaker);
	}

	CachedLastSpeaker = Speaker;
	CachedLastShot = DialogShot;
}

void UDialogBuilderGraph::BeginDialogSequence(UDialogBuilderNode_DialogSequence* InDialogSequence)
{
	if (!InDialogSequence)
		return;
	if(!InDialogSequence->DialogSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginDialogSequence: DialogSequence is null for Node ID: %s"), *InDialogSequence->ID.ToString());
		return;
	}
	DetermineAdvanceDialogRule(InDialogSequence);

	CurrentDialogSequence = InDialogSequence->DialogSequence;
	InDialogSequence->UpdateDialogStageData();

	SetDialogStage(InDialogSequence->DialogStage);

	PlaySequence(Cast<ULevelSequence>(InDialogSequence->DialogSequence.Get()), InDialogSequence->PlaybackSettings);
}

void UDialogBuilderGraph::JumpToNextDialogSection()
{
	if (!CurrentDialogSequence || !DialogSequenceActor)
	{
		return;
	}

	const UMovieSceneDialogTrack* DialogTrack = CurrentDialogSequence->FindDialogTrack();
	if (!DialogTrack)
	{
		return;
	}

	ULevelSequencePlayer* SequencePlayer = DialogSequenceActor->GetSequencePlayer();
	if (!SequencePlayer)
	{
		return;
	}
	const UMovieScene* MovieScene = CurrentDialogSequence ? CurrentDialogSequence->GetMovieScene() : nullptr;
	if (!MovieScene)
	{
		return;
	}
	const FFrameRate TickResolution = MovieScene->GetTickResolution();
	const FFrameRate DisplayRate = MovieScene->GetDisplayRate();


	TArray<UMovieSceneSection*> SortedSections = DialogTrack->GetAllSections();
	SortedSections.RemoveAll([](UMovieSceneSection* Section)
		{
			return Section == nullptr;
		});

	SortedSections.Sort([](const UMovieSceneSection& A, const UMovieSceneSection& B)
		{
			const TRange<FFrameNumber> RangeA = A.GetRange();
			const TRange<FFrameNumber> RangeB = B.GetRange();

			const FFrameNumber StartA = RangeA.HasLowerBound() ? RangeA.GetLowerBoundValue() : FFrameNumber(0);
			const FFrameNumber StartB = RangeB.HasLowerBound() ? RangeB.GetLowerBoundValue() : FFrameNumber(0);

			return StartA < StartB;
		});

	const FFrameNumber CurrentFrame = SequencePlayer->GetCurrentTime().Time.FloorToFrame();

	for (UMovieSceneSection* Section : SortedSections)
	{
		if (!Section)
		{
			continue;
		}

		const TRange<FFrameNumber> SectionRange = Section->GetRange();
		if (!SectionRange.HasLowerBound())
		{
			continue;
		}
		else
		{
			const FFrameNumber StartTick = SectionRange.GetLowerBoundValue();
			const double StartSeconds = TickResolution.AsSeconds(FFrameTime(StartTick));
			const FFrameTime StartTime = FFrameRate::TransformTime(
				FFrameTime(StartTick),
				TickResolution,
				DisplayRate);


			const FFrameNumber SectionStart = StartTime.FloorToFrame();
			if (SectionStart > CurrentFrame)
			{
				SequencePlayer->SetPlaybackPosition(
					FMovieSceneSequencePlaybackParams(
						FFrameTime(SectionStart),
						EUpdatePositionMethod::Jump));

				return;
			}
		}
		
	}
	
	SequencePlayer->GoToEndAndStop();
	HandleDialogSequenceFinished();

}

void UDialogBuilderGraph::PlayDialogShot(UDialogCameraShot* InDialogShot, AActor* InSpeaker)
{
	if(!InDialogShot || !InSpeaker)
	{
		return;
	}
	//Shot method changed, make sure sequencer is destroyed
	DestroyDialogSequenceActor();

	// Spawn cinecam if it does not exist
	if (!Cinecam.IsValid())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ACineCameraActor* NewCinecam = GetWorld()->SpawnActor<ACineCameraActor>(ACineCameraActor::StaticClass(), SpawnParams);
		Cinecam = NewCinecam;
	}

	InDialogShot->Play(this, InSpeaker);
}

void UDialogBuilderGraph::PlaySequence(class ULevelSequence* InSequence, FMovieSceneSequencePlaybackSettings InPlaybackSetting)
{
	DestroyDialogSequenceActor();

	if (!DialogSequenceActor)
	{
		ALevelSequenceActor* OutActor = nullptr;
		InPlaybackSetting.bDisableCameraCuts = bDisableCameraCuts;
		ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), InSequence, InPlaybackSetting, OutActor);
		DialogSequenceActor = OutActor;
		if (DialogSequenceActor)
		{
			DialogSequenceActor->SetSequence(nullptr);
		}
	}
	if (DialogSequenceActor)
	{
		if (ULevelSequencePlayer* SP = DialogSequenceActor->GetSequencePlayer())
		{	
			SP->OnFinished.AddDynamic(this, &ThisClass::HandleDialogSequenceFinished);


			if (InSequence)
			{
				DialogSequenceActor->PlaybackSettings = InPlaybackSetting;
				DialogSequenceActor->SetSequence(InSequence);
			}

			SP->Play();
		}
	}
}

void UDialogBuilderGraph::HandleDialogSequenceFinished()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DialogLineTimer);
	}
	
	//set view target when sequence ends, this will provide better blend when the sequence ends
	OwningController->SetViewTarget(SequenceCameraActor);
	
	DestroyDialogSequenceActor();
	
	
	if (CurrentNode)
	{
		CurrentNode->EvaluateNextNode();
	}
}

FText UDialogBuilderGraph::GetParticipantName(FGameplayTag ParticipantTag) const
{
	if(!ParticipantTag.IsValid())
	{
		return FText();
	}
	for (auto& Participant : ParticipantDefinitions)
	{
		if (Participant->Tag == ParticipantTag)
		{
			return Participant->DisplayName;
		}
	}

	FParticipantInfo ParticipantInfo;
	if (const FParticipantInfo* FoundInfo = ParticipantInfoMap.Find(ParticipantTag))
	{
		ParticipantInfo = *FoundInfo;
	}
	else
	{
		return FText();
	}
	return ParticipantInfo.ParticipantName.IsEmpty() ? FText() : ParticipantInfo.ParticipantName;
}

int UDialogBuilderGraph::GetLevelNum() const
{
	int Level = 0;
	TArray<UDialogBuilderNode*> CurrLevelNodes = RootNodes;
	TArray<UDialogBuilderNode*> NextLevelNodes;

	while (CurrLevelNodes.Num() != 0)
	{
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UDialogBuilderNode* Node = CurrLevelNodes[i];
			check(Node != nullptr);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}

	return Level;
}

void UDialogBuilderGraph::GetNodesByLevel(int Level, TArray<UDialogBuilderNode*>& Nodes)
{
	int CurrLEvel = 0;
	TArray<UDialogBuilderNode*> NextLevelNodes;

	Nodes = RootNodes;

	while (Nodes.Num() != 0)
	{
		if (CurrLEvel == Level)
			break;

		for (int i = 0; i < Nodes.Num(); ++i)
		{
			UDialogBuilderNode* Node = Nodes[i];
			check(Node != nullptr);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		Nodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++CurrLEvel;
	}
}

void UDialogBuilderGraph::ClearGraph()
{
	for (int i = 0; i < AllNodes.Num(); ++i)
	{
		UDialogBuilderNode* Node = AllNodes[i];
		if (Node)
		{
			Node->ParentNodes.Empty();
			Node->ChildrenNodes.Empty();
			Node->Edges.Empty();
		}
	}

	ParticipantInfoMap.Empty();
	VisitedNodeIDs.Empty();
	AllNodes.Empty();
	RootNodes.Empty();
	NodeMap.Empty();
}

AActor* UDialogBuilderGraph::GetDialogDefinitionActor(UDialogDefinition* InDialogDefinition)
{
	AActor* OutActor = nullptr;
	for (const TPair<UDialogSequenceSlot*, TWeakObjectPtr<AActor>>& Pair : DialogSlotActors)
	{
		if (UDialogSequenceSlot* Slot = Pair.Key)
		{
			if (Slot->DialogDefinition == InDialogDefinition)
			{
				OutActor = Pair.Value.Get();
				break;
			}
		}
	}
	return OutActor;
}

APlayerController* UDialogBuilderGraph::GetOwningController()
{
	return OwningController;
}

APawn* UDialogBuilderGraph::GetOwningPawn()
{
	if (DialogComponent)
	{
		return DialogComponent->GetOwningPawn();
	}
	return nullptr;
}

UDialogComponent* UDialogBuilderGraph::GetDialogComponent()
{
	return DialogComponent;
}

bool UDialogBuilderGraph::IsChoiceSelectionActive() const
{
	return CurrentNode ? CurrentNode->IsA(UDialogBuilderNode_PlayerChoice::StaticClass()) : false;
}

bool UDialogBuilderGraph::CanAdvanceDialog() const
{
	return bCanAdvanceDialog;
}

void UDialogBuilderGraph::AdvanceDialogLine()
{
	if(!CanAdvanceDialog())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (World->IsPaused())
		{
			return;
		}
	}

	if(IsChoiceSelectionActive())
	{
		return;
	}

	if (DialogSequenceActor)
	{
		JumpToNextDialogSection();
		return;
	}
	

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DialogLineTimer);
	}
	StopCurrentDialogLine();
	if (CurrentNode)
	{
		CurrentNode->EvaluateNextNode();
	}
}

void UDialogBuilderGraph::StopCurrentDialogLine()
{
	if (DialogAudio)
	{
		//Remove any previously added bindings
		DialogAudio->OnAudioFinished.RemoveAll(this);
		DialogAudio->Stop();
		DialogAudio->DestroyComponent();
		DialogAudio = nullptr;
	}

	UDialogBuilderNode_DialogLine* CurrentDialogLineNode = Cast<UDialogBuilderNode_DialogLine>(CurrentNode);
	if (CurrentDialogLineNode && CurrentDialogLineNode->DialogLineData.DialogMontage && CachedLastSpeaker.IsValid())
	{
		UActorComponent* Mesh = CachedLastSpeaker->GetComponentByClass(USkeletalMeshComponent::StaticClass());
		if (USkeletalMeshComponent* BodyMesh = Cast<USkeletalMeshComponent>(Mesh))
		{
			if (BodyMesh->GetAnimInstance())
			{
				const float BlendOutTime = CurrentDialogLineNode->DialogLineData.DialogMontage->BlendOut.GetBlendTime();
				BodyMesh->GetAnimInstance()->Montage_Stop(BlendOutTime, CurrentDialogLineNode->DialogLineData.DialogMontage);
			}
		}
	}

	if (CurrentDialogLineNode && CurrentDialogLineNode->DialogLineData.FacialAnimation && CachedLastSpeaker.IsValid())
	{
		TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		CachedLastSpeaker->GetComponents(SkeletalMeshComponents);

		USkeletalMeshComponent* TargetMesh = nullptr;
		const USkeleton* TargetSkeleton = CurrentDialogLineNode->DialogLineData.FacialAnimation->GetSkeleton();

		for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
		{
			if (!SkeletalMeshComponent)
			{
				continue;
			}

			if (!TargetMesh)
			{
				TargetMesh = SkeletalMeshComponent;
			}

			const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
			if (SkeletalMesh && TargetSkeleton && SkeletalMesh->GetSkeleton() == TargetSkeleton)
			{
				TargetMesh = SkeletalMeshComponent;
				break;
			}
		}

		if (TargetMesh)
		{
			TargetMesh->Stop();
		}
	}
}

void UDialogBuilderGraph::EndDialog()
{
	//destroy spawned dialog actor
	for (auto& SpawnedActor : SpawnedDialogActors)
	{
		if(SpawnedActor->IsPendingKillPending() )
			continue;
		SpawnedActor->Destroy();
	}

	if (ShouldLockPlayerMovement())
	{
		if (bRestoreState)
		{
			RestoreCachedActorTransforms();
		}
        
	}
	
	
	//clear shot and sequence
	CurrentDialogSequence = nullptr;
	SequencePivot = nullptr;

	if (bBlendCameraOnDialogEnd)
	{
		OwningController->SetViewTargetWithBlend(
			GetOwningPawn(),
			BlendTime,
			BlendFunc,
			BlendExp,
			true // bLockOutgoing
		);
	}
	else
	{
		OwningController->SetViewTarget(GetOwningPawn());
	}
	
	if (SequenceCameraActor)
	{
		SequenceCameraActor->SetLifeSpan(BlendTime);
		SequenceCameraActor = nullptr;
	}
	
	if (Cinecam.IsValid())
	{
		Cinecam->SetLifeSpan(BlendTime);
		Cinecam = nullptr;
	}
	
	
	DestroyDialogSequenceActor();
	
	if (DialogCameraShake && OwningController)
	{
		OwningController->ClientStopCameraShake(DialogCameraShake);
	}

	DialogSlotActors.Empty();
	VisitedNodeIDs.Empty();
	CachedActorMap.Empty();
	CachedActorOriginalTransforms.Empty();
	NodeMap.Empty();

	StopCurrentDialogLine();
	GetDialogComponent()->OnEndDialog.Broadcast(this);
	GetWorld()->GetTimerManager().ClearTimer(DialogLineTimer);
}

UDialogDefinition* UDialogBuilderGraph::GetDialogDefinitionByTag(FGameplayTag InTag)
{
	// Search ParticipantDefinitions
	for (auto&  Participant : ParticipantDefinitions)
	{
		if (!Participant) continue;
		// Matches tag?
		if (Participant->Tag == InTag)
		{
			return Participant;
		}
	}

	for (auto& Prop : PropDefinitions)
	{
		if (!Prop) continue;
		// Matches tag?
		if (Prop->Tag == InTag)
		{
			return Prop;
		}
	}

	return nullptr;
}

void UDialogBuilderGraph::DetermineAdvanceDialogRule(UDialogBuilderNode* InDialogNode)
{
	if(!GetWorld())
		return;

	UDialogBuilderNode_DialogLine* InDialogLine = Cast<UDialogBuilderNode_DialogLine>(InDialogNode);


	//Determine each dialog time before advance to the next line
	FTimerDelegate TimerDel;

	const TWeakObjectPtr<UDialogBuilderGraph> WeakThis(this);
	TimerDel = FTimerDelegate::CreateLambda([WeakThis, InDialogLine]()
		{
			if (!WeakThis.IsValid()) return;

			WeakThis->bCanAdvanceDialog = true;

			if(WeakThis->DialogComponent)
			{
				WeakThis->DialogComponent->OnDialogLineEnded.Broadcast(InDialogLine->GetDialogLine());
			}

			if (WeakThis->bAutoAdvanceDialogLine)
			{
				WeakThis->AdvanceDialogLine();
			}
		});

	

	if (InDialogLine)
	{
		bCanAdvanceDialog =  InDialogLine->GetDialogLine().bCanSkipDialogLine;
		GetWorld()->GetTimerManager().SetTimer(DialogLineTimer, TimerDel, InDialogLine->GetLineDuration(), false);
	}

}

FVector UDialogBuilderGraph::GetSpeakerBoneLocation(AActor* Actor, bool bOverrideTrackedBone, FName BoneOverride) const
{
	if (!Actor)
	{
		return FVector::ZeroVector;
	}

	FVector BoneLoc;
	FRotator BoneRot;


	for (auto& ActorSkelMesh : Actor->GetComponents())
	{
		if (USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(ActorSkelMesh))
		{
			BoneLoc = SkelMesh->GetBoneLocation(DefaultBoneToTrack);
			break;
		}
	}

	if (bOverrideTrackedBone)
	{
		for (auto& ActorSkelMesh : Actor->GetComponents())
		{
			if (USkeletalMeshComponent* SkelMesh = Cast<USkeletalMeshComponent>(ActorSkelMesh))
			{
				FVector FoundedLocation;
				FoundedLocation = SkelMesh->GetBoneLocation(BoneOverride);
				if(!FoundedLocation.IsNearlyZero())
				{
					BoneLoc = FoundedLocation;
				}
				break;
			}
		}
	}
	

	if (BoneLoc == FVector::ZeroVector)
	{
		Actor->GetActorEyesViewPoint(BoneLoc, BoneRot);
	}

	return BoneLoc;
}

void UDialogBuilderGraph::EnsurePlayerDefinition()
{
	if (!PlayerParticipantDefinition)
	{
		PlayerParticipantDefinition = NewObject<UDialogPlayerParticipant>(this, UDialogPlayerParticipant::StaticClass(), NAME_None, RF_Transactional);
		PlayerParticipantDefinition->DisplayName = LOCTEXT("Player", "Player");
	}

	PlayerParticipantDefinition->Tag = TAG_Dialog_Participant_Player;
	const int32 PlayerDefinitionIndex = ParticipantDefinitions.IndexOfByKey(PlayerParticipantDefinition);
	if (PlayerDefinitionIndex == INDEX_NONE)
	{
		ParticipantDefinitions.Insert(PlayerParticipantDefinition, 0);
	}
	else if (PlayerDefinitionIndex != 0)
	{
		ParticipantDefinitions.RemoveAt(PlayerDefinitionIndex);
		ParticipantDefinitions.Insert(PlayerParticipantDefinition, 0);
	}
}

AActor* UDialogBuilderGraph::RetrieveParticipant(FParticipantInfo& Info)
{
	APawn* PlayerPawn = GetOwningPawn();
	
	//retrieve player
	if (PlayerPawn && Info.ParticipantTag == TAG_Dialog_Participant_Player)
	{
		if (bOverrideTransform)
		{
			PlayerPawn->SetActorTransform(PlayerTransformOverride);
		}
		return PlayerPawn;
	}

	//Retrieve other participant
	if(!Info.ParticipantTag.GetTagName().IsNone())
	{
		if (!CachedActorMap.Contains(Info.GetID()) && 
			IsValid(Info.ParticipantToSpawn) &&
			Info.ParticipantSetup == EParticipantSetup::E_SpawnParticipant)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.bNoFail = true;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Owner = OwningController;

			
			if (UWorld* World = GetWorld())
			{
				AActor* SpawnedParticipant = World->SpawnActor(Info.ParticipantToSpawn, &Info.ParticipantTransform, SpawnParams);
				SpawnedDialogActors.AddUnique(SpawnedParticipant);
				return SpawnedParticipant;
			}
		}
		else
		{
			TArray<AActor*> FoundActors;

			for (FActorIterator It(GetWorld()); It; ++It)
			{
				AActor* Actor = *It;

				if (Actor && Actor->ActorHasTag(Info.ParticipantTag.GetTagName()))
				{
					FoundActors.Add(Actor);
				}
			}

			if (FoundActors.Num() > 1 && GetOwningPawn())
			{
				float Dist;

				return UGameplayStatics::FindNearestActor(GetOwningPawn()->GetActorLocation(), FoundActors, Dist);
			}
			else if (FoundActors.IsValidIndex(0))
			{
				return FoundActors[0];
			}
		}
	}
	

	return nullptr;
}


AActor* UDialogBuilderGraph::RetrieveActorFromSlot(UDialogSequenceSlot* InSlot, FName SlotID)
{
	if (!InSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("RetrieveParticipant: Slot is null for Dialog ID: %s"), *SlotID.ToString());
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UDialogDefinition* InDialogDefinition = InSlot->DialogDefinition;
	if (!InDialogDefinition)
	{
		return nullptr;
	}
	
	if (InDialogDefinition->IsA(UDialogPlayerParticipant::StaticClass()) && DialogType == EDialogType::E_FreeMovementDialog)
	{
		//we dont need to bind player to sequence if dialog type is free movement
		return nullptr;
	}

	const TSubclassOf<AActor> ActorClass = InDialogDefinition->GetActorClass();
	if (!ActorClass)
	{
		return nullptr;
	}


	// Look up by the slot's ID (FName) which is the map's key type
	if (TObjectPtr<AActor> ExistingActor = CachedActorMap.FindRef(SlotID))
	{
		ExistingActor->AttachToActor(SequencePivot, FAttachmentTransformRules::KeepRelativeTransform);
		ExistingActor->SetActorRelativeLocation(InSlot->SlotLocation);
		ExistingActor->SetActorRelativeRotation(InSlot->SlotRotation);
		return ExistingActor;
	}


	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoFail = true;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = OwningController;

	AActor* NewActor = World->SpawnActor<AActor>(ActorClass, FTransform::Identity, SpawnParams);
	if (NewActor)
	{
		NewActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		NewActor->AttachToActor(SequencePivot, FAttachmentTransformRules::KeepWorldTransform);
		NewActor->SetActorRelativeLocation(InSlot->SlotLocation);
		NewActor->SetActorRelativeRotation(InSlot->SlotRotation);

		SpawnedDialogActors.AddUnique(NewActor);
	}

	return NewActor;
}

AActor* UDialogBuilderGraph::RetrieveCameraFromSlot(class UDialogSequenceSlot* InSlot, FName SlotID)
{
	if (!InSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("RetrieveParticipant: Slot is null for Dialog ID: %s"), *SlotID.ToString());
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UDialogDefinition* InDialogDefinition = InSlot->DialogDefinition;
	if (!InDialogDefinition)
	{
		return nullptr;
	}

	const TSubclassOf<AActor> ActorClass = InDialogDefinition->GetActorClass();
	if (!ActorClass)
	{
		return nullptr;
	}


	// Look up by the slot's ID (FName) which is the map's key type
	if (TObjectPtr<AActor> ExistingActor = CachedActorMap.FindRef(SlotID))
	{
		ExistingActor->AttachToActor(SequencePivot, FAttachmentTransformRules::KeepRelativeTransform);
		ExistingActor->SetActorRelativeLocation(InSlot->SlotLocation);
		ExistingActor->SetActorRelativeRotation(InSlot->SlotRotation);
		return ExistingActor;
	}


	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoFail = true;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = OwningController;

	AActor* NewActor = World->SpawnActor<AActor>(ActorClass, FTransform::Identity, SpawnParams);
	if (NewActor)
	{
		NewActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		
		if (ACineCameraActor* CineCam = Cast<ACineCameraActor>(NewActor))
		{
			if (UCineCameraComponent* CinecamComp = CineCam->GetCineCameraComponent())
			{
				CinecamComp->CropSettings = CropSettings;
				CinecamComp->SetCurrentFocalLength(FocalLength);
				CinecamComp->SetCurrentAperture(Aperture);
				CinecamComp->SetFilmback(Filmback);
				CinecamComp->SetLensSettings(LensSettings);
				CinecamComp->SetConstraintAspectRatio(bConstrainAspectRatio);
				CinecamComp->bOverride_CustomNearClippingPlane = bOverride_CustomNearClippingPlane;
				CinecamComp->CustomNearClippingPlane = CustomNearClippingPlane;

				CinecamComp->FocusSettings.bSmoothFocusChanges = true;
				CinecamComp->FocusSettings.FocusSmoothingInterpSpeed = 15.0f;
				CinecamComp->FocusSettings.FocusMethod = FocusMethod;
				if (bUsePostProcess)
				{
					CinecamComp->PostProcessSettings = PostProcessSettings;
				}
				else
				{
					CinecamComp->PostProcessSettings = FPostProcessSettings();
				}
			}
		}

		NewActor->AttachToActor(SequencePivot, FAttachmentTransformRules::KeepWorldTransform);
		NewActor->SetActorRelativeLocation(InSlot->SlotLocation);
		NewActor->SetActorRelativeRotation(InSlot->SlotRotation);
	}

	return NewActor;
}

AActor* UDialogBuilderGraph::RetrieveLightFromSlot(UDialogSequenceSlot_Light* InSlot, FName SlotID)
{
	if (!InSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("RetrieveParticipant: Slot is null for Dialog ID: %s"), *SlotID.ToString());
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const TSubclassOf<AActor> ActorClass = InSlot->LightClass;
	if (!ActorClass)
	{
		return nullptr;
	}


	// Look up by the slot's ID (FName) which is the map's key type
	if (TObjectPtr<AActor> ExistingActor = CachedActorMap.FindRef(SlotID))
	{
		ExistingActor->AttachToActor(SequencePivot, FAttachmentTransformRules::KeepRelativeTransform);
		ExistingActor->SetActorRelativeLocation(InSlot->SlotLocation);
		ExistingActor->SetActorRelativeRotation(InSlot->SlotRotation);
		UpdateLightSlotProperty(InSlot, ExistingActor);
		return ExistingActor;
	}


	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoFail = true;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = OwningController;

	AActor* NewActor = World->SpawnActor<AActor>(ActorClass, FTransform::Identity, SpawnParams);
	if (NewActor)
	{
		NewActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);

		NewActor->AttachToActor(SequencePivot, FAttachmentTransformRules::KeepWorldTransform);
		NewActor->SetActorRelativeLocation(InSlot->SlotLocation);
		NewActor->SetActorRelativeRotation(InSlot->SlotRotation);

		UpdateLightSlotProperty(InSlot, NewActor);
		SpawnedDialogActors.AddUnique(NewActor);
	}

	return NewActor;
}

void UDialogBuilderGraph::UpdateLightSlotProperty(class UDialogSequenceSlot_Light* InLightSlot, AActor* InActor)
{
	if (!InLightSlot || !InActor) return;
	
	if (ULightComponent* LightComponent =  Cast<ULightComponent>(InActor->GetComponentByClass(ULightComponent::StaticClass())))
	{
		LightComponent->SetIntensity(InLightSlot->Intensity);
		LightComponent->SetLightColor(InLightSlot->LightColor);
		LightComponent->SetUseTemperature(InLightSlot->bUseTemperature);
		LightComponent->SetTemperature(InLightSlot->Temperature);
		LightComponent->bAffectsWorld = InLightSlot->bAffectsWorld;
		LightComponent->SetCastShadows(InLightSlot->CastShadows);
		LightComponent->SetIndirectLightingIntensity(InLightSlot->IndirectLightingIntensity);
		LightComponent->SetVolumetricScatteringIntensity(InLightSlot->VolumetricScatteringIntensity);
		LightComponent->CastStaticShadows = InLightSlot->CastStaticShadows;
		LightComponent->CastDynamicShadows = InLightSlot->CastDynamicShadows;
		LightComponent->SetAffectTranslucentLighting(InLightSlot->bAffectTranslucentLighting);
		LightComponent->SetCastVolumetricShadow(InLightSlot->bCastVolumetricShadow);
		LightComponent->SetCastDeepShadow(InLightSlot->bCastDeepShadow);
		LightComponent->CastRaytracedShadow = InLightSlot->CastRaytracedShadow;
		LightComponent->SetAffectReflection(InLightSlot->bAffectReflection);
		LightComponent->SetAffectGlobalIllumination(InLightSlot->bAffectGlobalIllumination);
		LightComponent->DeepShadowLayerDistribution = InLightSlot->DeepShadowLayerDistribution;
		
	}
	if (ULocalLightComponent* LocalLightComponent = Cast<ULocalLightComponent>(InActor->GetComponentByClass(ULocalLightComponent::StaticClass())))
	{
		LocalLightComponent->SetAttenuationRadius(InLightSlot->AttenuationRadius);
		LocalLightComponent->SetIntensityUnits(InLightSlot->IntensityUnits); 
	}
	
	
}

void UDialogBuilderGraph::CacheOriginalActorTransform(const FName& ActorId, AActor* Actor)
{
	if (!Actor || ActorId.IsNone())
	{
		return;
	}

	if (CachedActorOriginalTransforms.Contains(ActorId))
	{
		return;
	}

	if (SpawnedDialogActors.Contains(Actor))
	{
		return;
	}

	CachedActorOriginalTransforms.Emplace(ActorId, Actor->GetActorTransform());
}

void UDialogBuilderGraph::RestoreCachedActorTransforms()
{
	for (const TPair<FName, TObjectPtr<AActor>>& Pair : CachedActorMap)
	{
		AActor* Actor = Pair.Value;
		if (!Actor)
		{
			continue;
		}

		if (SpawnedDialogActors.Contains(Actor))
		{
			continue;
		}

		const FTransform* OriginalTransform = CachedActorOriginalTransforms.Find(Pair.Key);
		if (!OriginalTransform)
		{
			continue; 
		}

		Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Actor->SetActorTransform(*OriginalTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	CachedActorOriginalTransforms.Empty();
}

#if WITH_EDITOR

void UDialogBuilderGraph::GetAllGraphs(TArray<UEdGraph*>& Graphs) const
{
	

	for (int32 i = 0; i < DialogGraphPages.Num(); ++i)
	{
		UEdGraph* Graph = DialogGraphPages[i];
		if (Graph)
		{
			Graphs.Add(Graph);
			Graph->GetAllChildrenGraphs(Graphs);
		}
	}
	
}
void UDialogBuilderGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	FName DialogTypeProperty = FName("DialogType");

	if (DialogTypeProperty == PropertyName)
	{
		if (DialogType == EDialogType::E_FreeMovementDialog)
		{
			bDisableCameraCuts = true;
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITORONLY_DATA

#undef LOCTEXT_NAMESPACE

void UDialogBuilderGraph::GetNodesOfType(TSubclassOf<UDialogBuilderNode> NodeType, TArray<UDialogBuilderNode*>& Nodes) const
{
	Nodes.Reset();

	if (!*NodeType)
	{
		return;
	}

	for (UDialogBuilderNode* Node : AllNodes)
	{
		if (Node && Node->IsA(NodeType))
		{
			Nodes.Add(Node);
		}
	}
}
