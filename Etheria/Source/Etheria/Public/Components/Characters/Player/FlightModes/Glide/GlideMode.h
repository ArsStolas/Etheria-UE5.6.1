/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GlideMode - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/Characters/Player/FlightModes/FlightModeBase.h"
#include "GlideMode.generated.h"

UCLASS()
class ETHERIA_API UGlideMode : public UFlightModeBase
{
	GENERATED_BODY()

public:
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void TickMode(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere)
	float GlideSpeed = 1200.f;

	UPROPERTY(EditAnywhere)
	float DescendRate = 300.f;

	UPROPERTY(EditAnywhere)
	float GlideInterp = 2.f;

	UPROPERTY(EditAnywhere)
	float DescentInterp = 3.f;
};
