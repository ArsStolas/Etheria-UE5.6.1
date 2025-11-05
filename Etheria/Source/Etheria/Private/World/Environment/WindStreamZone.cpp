/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: WindStreamZone - Header
*/

#include "World/Environment/WindStreamZone.h"
#include "Components/BoxComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/Characters/Player/Glider/GliderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"

AWindStreamZone::AWindStreamZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerZone = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerZone"));
	RootComponent = TriggerZone;

	TriggerZone->SetCollisionProfileName(TEXT("Trigger"));
	TriggerZone->SetGenerateOverlapEvents(true);
}

void AWindStreamZone::BeginPlay()
{
	Super::BeginPlay();
	TriggerZone->OnComponentBeginOverlap.AddDynamic(this, &AWindStreamZone::OnOverlapBegin);

	DrawDebugBox(
		GetWorld(),
		GetActorLocation(),
		TriggerZone->GetScaledBoxExtent(),
		GetActorRotation().Quaternion(),
		DebugColor,
		true,
		-1.0f,
		0,
		2.0f
	);
}

void AWindStreamZone::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bUsed && bOneTimeUse)
		return;

	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (!Player)
		return;

	UGliderComponent* GliderComp = Player->FindComponentByClass<UGliderComponent>();
	if (!GliderComp || !GliderComp->IsGliding())
		return;

	UCharacterMovementComponent* MoveComp = Player->GetCharacterMovement();
	if (!MoveComp)
		return;

	FVector ForwardDir = Player->GetActorForwardVector();
	FVector Velocity = MoveComp->Velocity;

	Velocity += ForwardDir * BoostForce;
	MoveComp->Velocity = Velocity;

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Glider Speed Boost!"));

	if (bOneTimeUse)
		bUsed = true;
}

