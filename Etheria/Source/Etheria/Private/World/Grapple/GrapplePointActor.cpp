/**
* Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: GrapplePointActor - Source
 */

#include "World/Grapple/GrapplePointActor.h"
#include "Components/StaticMeshComponent.h"

AGrapplePointActor::AGrapplePointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Mesh visible dans le monde
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// Point d’attache réel de la corde
	AttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("AttachPoint"));
	AttachPoint->SetupAttachment(Mesh);
	AttachPoint->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
}

void AGrapplePointActor::BeginPlay()
{
	Super::BeginPlay();
}