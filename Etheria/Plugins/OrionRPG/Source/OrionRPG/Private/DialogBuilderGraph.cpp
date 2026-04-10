// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderGraph.h"
#include "DialogComponent.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderEdge.h"
#include "DialogCameraShot.h"
#include "DialogBuilderNode_Root.h"
#include "DialogBuilderSetting.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "DialogBuilderNode_RerouteNode.h"
#include "DialogBuilderNode_DialogLine.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include <CineCameraActor.h>
#include <DefaultLevelSequenceInstanceData.h>
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

#define LOCTEXT_NAMESPACE "DialogBuilderGraph"


UDialogBuilderGraph::UDialogBuilderGraph()
{
	bAutoRotateParticipant = false;
	bKeepPosition = false;
	DefaultBoneToTrack = FName("head");
	bAutoAdvanceDialogLine = false;
	bLockPlayerMovement = true;
	bCanInterruptDialog = false;
	bUseDialogCameraFade = false;
	FadeDuration = 0.25f;
	FadeColor = FLinearColor::Black;

	BlendTime = 1.25f;
	BlendFunc = VTBlend_Cubic;
	BlendExp = 2.0f;
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
		VisitedNodeIDs.Empty();
		CachedParticipants.Empty();
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

		for (TPair<FGameplayTag, FParticipantInfo>& Pair : ParticipantInfoMap)
		{
			FGameplayTag ParticipantTag = Pair.Key;
			FParticipantInfo& ParticipantInfo = Pair.Value;
			if(AActor* ParticipantActor = RetrieveParticipant(ParticipantInfo))
			{
				CachedParticipants.Emplace(ParticipantInfo.ParticipantTag, ParticipantActor);
				ParticipantInfo.InitialTransform = ParticipantActor->GetTransform();

			}
		}


		if (OwningController && OwningController->IsLocalPlayerController())
		{
			if (DialogCameraShake)
			{
				OwningController->ClientStartCameraShake(DialogCameraShake);
			}
		}
		bAutoAdvanceDialogLine = bLockPlayerMovement ? bAutoAdvanceDialogLine : true; // if dialog is free movement, force auto advance dialog line -> true
		bUseDialogCameraFade = bLockPlayerMovement ? bUseDialogCameraFade : false;
		

		
	}
	return false;
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

void UDialogBuilderGraph::StartFromRoot()
{
	if (RootNodes.IsValidIndex(0))
	{
		BeginNode(RootNodes[0]);
	}
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

	AActor* Speaker = CachedParticipants.FindRef(InDialogLine->ParticipantInfo.ParticipantTag);
	AActor* Listener = CachedParticipants.FindRef(InDialogLine->ListenerTag);

	DetermineAdvanceDialogRule(InDialogLine);

	//Dialog Sound
	if (InDialogLine->DialogLineData.DialogSound)
	{
		if (Speaker && bLockPlayerMovement)
		{
			DialogAudio = UGameplayStatics::SpawnSoundAttached(InDialogLine->DialogLineData.DialogSound, Speaker->GetRootComponent(), NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, false, 1.f, 1.f, 0.f, DialogSoundAttenuation);
		}
		else
		{
			DialogAudio = UGameplayStatics::SpawnSound2D(this, InDialogLine->DialogLineData.DialogSound);
		}
	}

	if (!bLockPlayerMovement && InDialogLine->ParticipantInfo.ParticipantTag == TAG_Dialog_Participant_Player)
		return;

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

	
	
	CachedLastSpeaker = Speaker;
	
}

void UDialogBuilderGraph::ApplyRotationSetting(UDialogBuilderNode_DialogLine* InDialogLine, AActor* Speaker, AActor* Listener)
{
	//Rotation Setting
	if (Speaker && bAutoRotateParticipant)
	{
		for (auto& Participant : CachedParticipants)
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
	if (!bLockPlayerMovement)
		return;

	ApplyRotationSetting(InDialogLine, Speaker, Listener);

	UDialogCameraShot* DialogShot = nullptr;

	//Determine the shot
	switch (InDialogLine->DialogLineData.DialogCameraMode)
	{
	case EDialogCameraMode::E_GeneratedCameraShot:
		DialogShot = InDialogLine->ParticipantInfo.DefaultShot;
		if (!DialogShot || InDialogLine->DialogLineData.ShotOverride)
		{
			//if default shot not valid, try use override shot data instead
			DialogShot = InDialogLine->DialogLineData.ShotOverride;
		}

		PlayDialogShot(DialogShot, Speaker);

		break;
	case EDialogCameraMode::E_Sequence:
		PlayDialogSequence(InDialogLine->DialogLineData);
		break;
	}
	CachedLastShot = DialogShot;
}

void UDialogBuilderGraph::BeginChoiceSelection(UDialogBuilderNode_DialogLine* InDialogLine)
{
	if (!bLockPlayerMovement)
	{
		EndDialog();
		return;
	}
	if (InDialogLine == nullptr)
		return;

	UDialogCameraShot* DialogShot = DefaultSelectingChoiceShot;
	AActor* Speaker = CachedParticipants.FindRef(TAG_Dialog_Participant_Player);
	Speaker = Speaker ? Speaker : GetOwningPawn();

	if (!DialogShot || InDialogLine->SelectingChoiceShotOverride)
	{
		//if default shot not valid, try use override shot data instead
		DialogShot = InDialogLine->SelectingChoiceShotOverride;
	}

	PlayDialogShot(DialogShot, Speaker);

	CachedLastSpeaker = Speaker;
	CachedLastShot = DialogShot;
}

void UDialogBuilderGraph::PlayDialogShot(UDialogCameraShot* InDialogShot, AActor* InSpeaker)
{
	if(!InDialogShot || !InSpeaker)
	{
		return;
	}
	//Shot method changed, make sure sequencer is destroyed
	if (DialogSequencer)
	{
		if (ULevelSequencePlayer* SP = DialogSequencer->GetSequencePlayer())
		{
			SP->Stop();
		}
		DialogSequencer->Destroy();
	}

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

void UDialogBuilderGraph::PlayDialogSequence(const FDialogLineData& InDialogLineData)
{
	//Shot method changed, make sure cinecam is destroyed
	if (Cinecam.IsValid())
	{
		Cinecam->Destroy();
		Cinecam = nullptr;
	}

	if (!DialogSequencer)
	{
		ALevelSequenceActor* OutActor = nullptr;
		ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), InDialogLineData.SequenceToPlay, InDialogLineData.PlaybackSettings, OutActor);
		DialogSequencer = OutActor;
		if (DialogSequencer)
		{
			DialogSequencer->SetSequence(nullptr);
		}
	}
	if (DialogSequencer)
	{
		if (ULevelSequencePlayer* SP = DialogSequencer->GetSequencePlayer())
		{
			SP->OnFinished.RemoveAll(this);
			if (SP->IsPlaying())
			{
				SP->Stop();
			}
			if (InDialogLineData.SequenceToPlay)
			{
				DialogSequencer->PlaybackSettings = InDialogLineData.PlaybackSettings;
				DialogSequencer->SetSequence(InDialogLineData.SequenceToPlay);
			}
			SP->Play();


		}
	}
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

void UDialogBuilderGraph::AdvanceDialogLine()
{
	if (CurrentLine && !CurrentLine->bCanSkipDialogLine 
		&& DialogLineTimer.IsValid() 
		&& GetWorld() && GetWorld()->GetTimerManager().GetTimerRemaining(DialogLineTimer) > .0f)
	{
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

	UDialogBuilderNode_DialogLine* CurrentDialogLine = Cast<UDialogBuilderNode_DialogLine>(CurrentNode);
	if (CurrentDialogLine && CurrentDialogLine->DialogLineData.DialogMontage && CachedLastSpeaker.IsValid())
	{
		UActorComponent* Mesh = CachedLastSpeaker->GetComponentByClass(USkeletalMeshComponent::StaticClass());
		if (USkeletalMeshComponent* BodyMesh = Cast<USkeletalMeshComponent>(Mesh))
		{
			if (BodyMesh->GetAnimInstance())
			{
				const float BlendOutTime = CurrentDialogLine->DialogLineData.DialogMontage->BlendOut.GetBlendTime();
				BodyMesh->GetAnimInstance()->Montage_Stop(BlendOutTime, CurrentDialogLine->DialogLineData.DialogMontage);
			}
		}

	}
}

void UDialogBuilderGraph::EndDialog()
{

	if (bLockPlayerMovement)
	{
		if (bKeepPosition)
		{
			//Send all participants back to their initial transforms
			for (const TPair<FGameplayTag, TObjectPtr<AActor>>& Pair : CachedParticipants)
			{
				FGameplayTag ParticipantTag = Pair.Key;
				AActor* ParticipantActor = Pair.Value;
				FTransform ParticipantInitialTransform = ParticipantInfoMap.FindRef(ParticipantTag).InitialTransform;

				ParticipantActor->SetActorTransform(ParticipantInitialTransform);
			}
		}
        
	}
	
	
	//clear shot and sequence
	if (DialogSequencer)
	{
		if (ULevelSequencePlayer* SP = DialogSequencer->GetSequencePlayer())
		{
			SP->Stop();
		}

		DialogSequencer->Destroy();
	}

	if (Cinecam.IsValid())
	{
		if (bBlendCameraOnDialogEnd)
		{
			OwningController->SetViewTargetWithBlend(
				GetOwningPawn(),
				BlendTime,
				BlendFunc,
				BlendExp,
				true // bLockOutgoing
			);
			Cinecam->SetLifeSpan(BlendTime);
		}
		else
		{
			Cinecam->Destroy();
		}
		Cinecam = nullptr;
	}

	if (DialogCameraShake && OwningController)
	{
		OwningController->ClientStopCameraShake(DialogCameraShake);
	}

	//destroy spawned participant
	for (auto& SpawnedParticipant : SpawnedParticipants)
	{
		SpawnedParticipant->Destroy();
	}

	StopCurrentDialogLine();
	GetDialogComponent()->OnEndDialog.Broadcast(this);
	GetWorld()->GetTimerManager().ClearTimer(DialogLineTimer);
}

void UDialogBuilderGraph::DetermineAdvanceDialogRule(UDialogBuilderNode_DialogLine* InDialogLine)
{
	if(!GetWorld())
		return;
	//Determine each dialog time before advance to the next line
	if (bAutoAdvanceDialogLine)
	{
		GetWorld()->GetTimerManager().SetTimer(DialogLineTimer, this, &ThisClass::AdvanceDialogLine, InDialogLine->GetLineDuration(), false);	
	}
	else
    {		
		FTimerDelegate TimerDel;
		TimerDel = FTimerDelegate::CreateLambda([this]()
		{
			//empty implementation, just to make sure timer is valid
		});
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

AActor* UDialogBuilderGraph::RetrieveParticipant(const FParticipantInfo& Info)
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
		if (!CachedParticipants.Contains(Info.ParticipantTag) && 
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
				SpawnedParticipants.AddUnique(SpawnedParticipant);
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

#if WITH_EDITORONLY_DATA

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
#endif // WITH_EDITORONLY_DATA

#undef LOCTEXT_NAMESPACE