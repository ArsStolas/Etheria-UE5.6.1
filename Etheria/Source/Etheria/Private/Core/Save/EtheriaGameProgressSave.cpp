/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "EtheriaGameProgressSave - Source"
 * Notes: Helpers for clearing all data maps and building FSaveSlotInfo.
 */

#include "Core/Save/EtheriaGameProgressSave.h"

UEtheriaGameProgressSave::UEtheriaGameProgressSave()
{
	SlotType       = ESaveSlotType::Auto;
	SlotIndex      = 0;
	SaveTimestamp  = FDateTime::UtcNow();
	LocationLabel  = TEXT("");
	PlayerLevel    = 1;
	PlayTime       = 0.f;
	Era            = EGameEra::Present;
	PlayerTransform = FTransform::Identity;
	LastCampfireID = NAME_None;
}

void UEtheriaGameProgressSave::ClearAllData()
{
	BoolData.Empty();
	IntData.Empty();
	FloatData.Empty();
	StringData.Empty();
	NameData.Empty();
	VectorData.Empty();
	RotatorData.Empty();
	TransformData.Empty();
	ObjectPathData.Empty();
	ClassPathData.Empty();
	GuidData.Empty();
	BlobData.Empty();
}

FSaveSlotInfo UEtheriaGameProgressSave::BuildSlotInfo() const
{
	FSaveSlotInfo Info;
	Info.bExists       = true;
	Info.SlotType      = SlotType;
	Info.SlotIndex     = SlotIndex;
	Info.Timestamp     = SaveTimestamp;
	Info.LocationLabel = LocationLabel;
	Info.PlayerLevel   = PlayerLevel;
	Info.PlayTime      = PlayTime;
	Info.Era           = Era;
	return Info;
}
