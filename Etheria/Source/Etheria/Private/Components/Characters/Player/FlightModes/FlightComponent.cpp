/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: FlightComponent - Source
*/

#include "Components/Characters/Player/FlightModes/FlightComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/Characters/Player/FlightModes/Dive/DiveMode.h"
#include "Components/Characters/Player/FlightModes/Glide/GlideMode.h"

UFlightComponent::UFlightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFlightComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<APlayerCharacter>(GetOwner());

	GlideMode = NewObject<UGlideMode>(this);
	DiveMode  = NewObject<UDiveMode>(this);

	GlideMode->Initialize(Owner);
	DiveMode->Initialize(Owner);
}

void UFlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ActiveMode)
		ActiveMode->TickMode(DeltaTime);
}

void UFlightComponent::StartGlide()
{
	StopMode();
	CurrentMode = EFlightMode::Glide;
	ActiveMode = GlideMode;
	ActiveMode->Enter();
}

void UFlightComponent::StartDive()
{
	StopMode();
	CurrentMode = EFlightMode::Dive;
	ActiveMode = DiveMode;
	ActiveMode->Enter();
}

void UFlightComponent::StopMode()
{
	if (ActiveMode)
		ActiveMode->Exit();

	ActiveMode = nullptr;
	CurrentMode = EFlightMode::None;
}
