// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderNode_PlayerLine.h"
#include "DialogBuilderGraph.h"
#include "DialogComponent.h"
#include "Event/OrionEvent.h"


UDialogBuilderNode_PlayerLine::UDialogBuilderNode_PlayerLine()
{
	bIsPlayerLine = true;
	ParticipantInfo.ParticipantTag = TAG_Dialog_Participant_Player;
}

void UDialogBuilderNode_PlayerLine::BeginNode()
{
	ParticipantInfo.ParticipantTag = TAG_Dialog_Participant_Player;
	ParticipantInfo.ParticipantName = GetOwningDialogGraph()->PlayerName;
	ParticipantInfo.DefaultShot = GetOwningDialogGraph()->DefaultPlayerShot;
	ParticipantInfo.ParticipantImage = GetOwningDialogGraph()->DefaultPlayerImage;
	GetOwningDialogGraph()->BeginDialogLine(this);
	GetDialogComponent()->OnDialogLineBegin.Broadcast(GetDialogLine());
	GetDialogComponent()->OnDialogUpdated.Broadcast(this);
}

#if WITH_EDITOR

FText UDialogBuilderNode_PlayerLine::GetNodeTitle() const
{
	if (GetOwningDialogGraph())
	{
		return GetOwningDialogGraph()->PlayerName.IsEmpty() ? FText::FromString("Player") : GetOwningDialogGraph()->PlayerName;
	}
	return FText::FromString("");
}
void UDialogBuilderNode_PlayerLine::SetNodeTitle(const FText& NewTitle)
{
	ID = FName(NewTitle.ToString());
}

FText UDialogBuilderNode_PlayerLine::GetNodeDescription() const
{
	return DialogLineData.Line;
}

#endif