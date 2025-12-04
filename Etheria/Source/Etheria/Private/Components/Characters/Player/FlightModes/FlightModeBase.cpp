/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: FlightModeBase - Source
*/

#include "Components/Characters/Player/FlightModes/FlightModeBase.h"
#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UFlightModeBase::Initialize(APlayerCharacter* InOwner)
{
	Owner = InOwner;
	Move = Owner->GetCharacterMovement();
}

void UFlightModeBase::StoreMovementSettings()
{
	bOriRot     = Move->bOrientRotationToMovement;
	Gravity     = Move->GravityScale;
	AirCtrl     = Move->AirControl;
	Accel       = Move->MaxAcceleration;
	Decel       = Move->BrakingDecelerationFalling;
	MaxSpeed    = Move->MaxWalkSpeed;
	bDesiredRot = Move->bUseControllerDesiredRotation;
}

void UFlightModeBase::RestoreMovementSettings()
{
	Move->bOrientRotationToMovement  = bOriRot;
	Move->GravityScale               = Gravity;
	Move->AirControl                 = AirCtrl;
	Move->MaxAcceleration            = Accel;
	Move->BrakingDecelerationFalling = Decel;
	Move->MaxWalkSpeed               = MaxSpeed;
	Move->bUseControllerDesiredRotation = bDesiredRot;
	Move->SetMovementMode(MOVE_Falling);
}

void UFlightModeBase::Enter() {}
void UFlightModeBase::Exit() {}
void UFlightModeBase::TickMode(float DeltaTime) {}
