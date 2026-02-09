/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AISplinePath - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "AISplinePath.generated.h"

UCLASS()
class ETHERIA_API AAISplinePath : public AActor
{
	GENERATED_BODY()

public:
	AAISplinePath();

	USplineComponent* GetSpline() const { return Spline; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USplineComponent* Spline;
};
