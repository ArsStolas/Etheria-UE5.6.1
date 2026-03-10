/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BaseBoss - Source
*/

#include "Characters/AI/Enemies/Enemies_Type/BaseBoss.h"
#include "Components/Inventory/EquipmentComponent.h"
#include "Data/Weapons/WeaponData.h"

ABaseBoss::ABaseBoss()
{
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("Equipment"));
	AIType = EAIType::Hostile;
}

void ABaseBoss::BeginPlay()
{
	Super::BeginPlay();

	if (EquipmentComponent && DefaultBossWeaponData)
	{
		EquipmentComponent->SetOverrideWeaponDataForAI(DefaultBossWeaponData);
	}
}

void ABaseBoss::ApplyWeaponDataForPhase(int32 Phase)
{
	if (!EquipmentComponent) return;

	UWeaponData* ToApply = DefaultBossWeaponData;
	if (Phase == 2 && Phase2WeaponData)
	{
		ToApply = Phase2WeaponData;
	}
	else if (Phase == 3 && Phase3WeaponData)
	{
		ToApply = Phase3WeaponData;
	}
	else if (Phase == 3 && Phase2WeaponData)
	{
		ToApply = Phase2WeaponData;
	}

	if (ToApply)
	{
		EquipmentComponent->SetOverrideWeaponDataForAI(ToApply);
	}
}
