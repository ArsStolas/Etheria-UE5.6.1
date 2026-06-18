// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogDefinition.h"
#include "CineCameraActor.h"
#include "Engine/PointLight.h"
#include "Misc/Guid.h"


#define LOCTEXT_NAMESPACE "DialogDefinition"

UDialogDefinition::UDialogDefinition()
{
	GetOrCreateID();
}

FGuid UDialogDefinition::GetOrCreateID()
{
	if(!ID.IsValid())
	{
		ID = FGuid::NewGuid();
	}

	return ID;
}

UDialogPlayerParticipant::UDialogPlayerParticipant()
{

}

UDialogParticipant::UDialogParticipant()
{
	DisplayName = FText::FromString("Participant");
}

UDialogProp::UDialogProp()
{
	DisplayName = FText::FromString("Prop");
}

UDialogCamera::UDialogCamera()
{
	DisplayName = FText::FromString("Camera");
	CameraClassSoft = ACineCameraActor::StaticClass();
	CameraClass = ACineCameraActor::StaticClass();
}

UDialogLight::UDialogLight()
{
	DisplayName = FText::FromString("Light");
	LightClassSoft = APointLight::StaticClass();
	LightClass = APointLight::StaticClass();
}
#undef LOCTEXT_NAMESPACE


