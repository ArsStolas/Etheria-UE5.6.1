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

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	MeshComponent->SetSimulatePhysics(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

	// Point d'ancrage positionnable librement dans l'éditeur
	AnchorPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AnchorPoint"));
	AnchorPoint->SetupAttachment(MeshComponent);
	AnchorPoint->SetRelativeLocation(FVector::ZeroVector);
}

FVector ARopeAttachPoint::GetAnchorLocation() const
{
	if (AnchorPoint)
	{
		return AnchorPoint->GetComponentLocation();
	}
	return GetActorLocation();
}

void ARopeAttachPoint::BeginPlay()
{
	Super::BeginPlay();

	AllAttachPoints.Add(this);

	if (AttachType == ERopeAttachType::Pull)
	{
		MeshComponent->SetSimulatePhysics(true);
		MeshComponent->SetEnableGravity(true);
	}
}

void ARopeAttachPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AllAttachPoints.Remove(this);
	Super::EndPlay(EndPlayReason);
}
