/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: DiveMode - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/Characters/Player/FlightModes/FlightModeBase.h"
#include "DiveMode.generated.h"

UCLASS()
class ETHERIA_API UDiveMode : public UFlightModeBase
{
	GENERATED_BODY()

public:
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void TickMode(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere)
	float MaxDiveSpeed = 2400.f;

	UPROPERTY(EditAnywhere)
	float MinDiveSpeed = 300.f;

	UPROPERTY(EditAnywhere)
	float DiveAcceleration = 1200.f;

	float CurrentSpeed = 400.f;

	UPROPERTY(EditAnywhere)
	float MaxPitch = 45.f;

	UPROPERTY(EditAnywhere)
	float MaxRoll = 30.f;

	UPROPERTY(EditAnywhere)
	float TurnRate = 120.f;

	UPROPERTY(EditAnywhere)
	float LiftFactor = 0.6f;
};
