/**
* Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: FlightModeBase - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FlightModeBase.generated.h"

class APlayerCharacter;
class UCharacterMovementComponent;

UCLASS(Abstract, Blueprintable)
class ETHERIA_API UFlightModeBase : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(APlayerCharacter* InOwner); 

	virtual void Enter();
	virtual void Exit();
	virtual void TickMode(float DeltaTime);

protected:
	// Utilities
	void StoreMovementSettings();
	void RestoreMovementSettings();

	APlayerCharacter* Owner = nullptr;
	UCharacterMovementComponent* Move = nullptr;

	// Backup des paramètres de mouvement
	bool bOriRot;
	float Gravity;
	float AirCtrl;
	float Accel;
	float Decel;
	float MaxSpeed;
	bool bDesiredRot;
	FRotator RotRate;
	EMovementMode MovementMode;
};