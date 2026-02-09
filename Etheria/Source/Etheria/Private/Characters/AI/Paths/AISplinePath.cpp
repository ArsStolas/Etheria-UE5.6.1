/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: AISplinePath - Source
*/

#include "Characters/AI/Paths/AISplinePath.h"

AAISplinePath::AAISplinePath()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>("Spline");
	RootComponent = Spline;
}
