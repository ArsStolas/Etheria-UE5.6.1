/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeAttachPoint - Source
*/

#include "World/Rope/RopeAttachPoint.h"
#include "DrawDebugHelpers.h"

TArray<TWeakObjectPtr<ARopeAttachPoint>> ARopeAttachPoint::AllAttachPoints;

ARopeAttachPoint::ARopeAttachPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void ARopeAttachPoint::BeginPlay()
{
	Super::BeginPlay();
	AllAttachPoints.Add(this);
}

void ARopeAttachPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AllAttachPoints.Remove(this);
	Super::EndPlay(EndPlayReason);
}
