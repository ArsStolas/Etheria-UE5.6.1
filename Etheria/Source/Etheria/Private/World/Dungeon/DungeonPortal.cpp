/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ADungeonPortal" - Source
 */

#include "World/Dungeon/DungeonPortal.h"

#include "World/Dungeon/DungeonTravelComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"

ADungeonPortal::ADungeonPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	root = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = root;

	interactionBox = CreateDefaultSubobject<UBoxComponent>("InteractionBox");
	interactionBox->SetupAttachment(root);

	returnPlane = CreateDefaultSubobject<UStaticMeshComponent>("ReturnPlane");
	returnPlane->SetupAttachment(root);

	returnArrow = CreateDefaultSubobject<UArrowComponent>("ReturnArrow");
	returnArrow->SetupAttachment(returnPlane);
}

void ADungeonPortal::BeginPlay()
{
	Super::BeginPlay();
}

UDungeonTravelComponent* ADungeonPortal::GetTravelComponentFrom(AActor* Target) const
{
	return Target ? Target->FindComponentByClass<UDungeonTravelComponent>() : nullptr;
}

void ADungeonPortal::Interact_Implementation(AActor* Target)
{
	// Your interaction system calls this. Keep using it.
	StartPortalInteraction(Target);
}

void ADungeonPortal::CanReceiveTrace_Implementation()
{
	// Compatibility with your interaction system (highlight checks).
}

void ADungeonPortal::StartPortalInteraction(AActor* Target)
{
	UDungeonTravelComponent* TravelComp = GetTravelComponentFrom(Target);
	if (!TravelComp)
	{
		return;
	}

	// IMPORTANT:
	// - Enter portal (main world) must be remembered so that "OnArriveBackToOrigin" runs on THIS actor,
	//   not on the dungeon exit portal (which gets unloaded).
	if (bIsReturnPortal)
	{
		TravelComp->SetExitPortalActor(this);
	}
	else
	{
		TravelComp->SetEnterPortalActor(this);

		const FTransform ReturnTransform = returnPlane ? returnPlane->GetComponentTransform() : GetActorTransform();
		const FRotator ReturnControlRot = returnArrow ? returnArrow->GetComponentRotation() : GetActorRotation();

		TravelComp->BeginPreloadFromPortal(destinationLevel, dungeonInstanceLocation, ReturnTransform, ReturnControlRot);
	}

	// Interaction just completed: capture the character's visible appearance and
	// start the fade to black right away (held until the teleport finishes).
	TravelComp->NotifyPortalInteractionStarted();

	OnPortalSequenceStart(Target);
}

void ADungeonPortal::NotifySequenceFinished(AActor* Target)
{
	UDungeonTravelComponent* TravelComp = GetTravelComponentFrom(Target);
	if (!TravelComp)
	{
		return;
	}

	// Safety: if BP calls NotifySequenceFinished without going through StartPortalInteraction,
	// keep portal references up to date.
	if (bIsReturnPortal)
	{
		TravelComp->SetExitPortalActor(this);
		TravelComp->CommitReturnToOrigin();
	}
	else
	{
		TravelComp->SetEnterPortalActor(this);
		TravelComp->CommitEnterDungeon(destinationSpawnTag);
	}
}
