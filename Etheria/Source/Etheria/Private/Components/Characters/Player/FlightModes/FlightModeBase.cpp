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
	if (Owner)
		Move = Owner->GetCharacterMovement();
}

void UFlightModeBase::StoreMovementSettings()
{
	if (!Move) return;

	bOriRot     = Move->bOrientRotationToMovement;
	Gravity     = Move->GravityScale;
	AirCtrl     = Move->AirControl;
	Accel       = Move->MaxAcceleration;
	Decel       = Move->BrakingDecelerationFalling;
	MaxSpeed    = Move->MaxWalkSpeed;
	bDesiredRot = Move->bUseControllerDesiredRotation;
	RotRate     = Move->RotationRate;
	MovementMode = Move->MovementMode;
}

void UFlightModeBase::RestoreMovementSettings()
{
	if (!Move) return;

	Move->bOrientRotationToMovement     = bOriRot;
	Move->GravityScale                  = Gravity;
	Move->AirControl                    = AirCtrl;
	Move->MaxAcceleration               = Accel;
	Move->BrakingDecelerationFalling    = Decel;
	Move->MaxWalkSpeed                  = MaxSpeed;
	Move->bUseControllerDesiredRotation = bDesiredRot;
	Move->RotationRate                  = RotRate;

	// Retour au mode correct selon la situation
	if (Move->IsMovingOnGround())
		Move->SetMovementMode(MOVE_Walking);
	else
		Move->SetMovementMode(MOVE_Falling);
}

void UFlightModeBase::Enter() {}
void UFlightModeBase::Exit() {}
void UFlightModeBase::TickMode(float DeltaTime) {}
