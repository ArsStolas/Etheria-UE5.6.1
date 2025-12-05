/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: WindStreamZone - Header
*/

#include "World/Environment/WindStreamZone.h"
#include "Components/BoxComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Components/Characters/Player/FlightModes/FlightComponent.h"
#include "Engine/Engine.h"

AWindStreamZone::AWindStreamZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerZone = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerZone"));
	RootComponent = TriggerZone;

	TriggerZone->SetCollisionProfileName(TEXT("Trigger"));
	TriggerZone->SetGenerateOverlapEvents(true);
	TriggerZone->OnComponentBeginOverlap.AddDynamic(this, &AWindStreamZone::OnOverlapBegin);
	TriggerZone->OnComponentEndOverlap.AddDynamic(this, &AWindStreamZone::OnOverlapEnd);
}

void AWindStreamZone::BeginPlay()
{
	Super::BeginPlay();

	// Debug visuel
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

void AWindStreamZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (!Player) return;
	if (bOneTimeUse && ActivePlayers.Num() > 0) return;

	// Optionnel : check dive mode
	if (bAffectOnlyDive && !Player->GetFlightComponent()->IsInMode(EFlightMode::Dive))
		return;

	// Timer répétitif pour appliquer le mouvement
	if (!ActivePlayers.Contains(Player))
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(this, [this, Player]()
		{
			if (!Player || !IsValid(Player))
			{
				if (ActivePlayers.Contains(Player))
				{
					GetWorldTimerManager().ClearTimer(ActivePlayers[Player]);
					ActivePlayers.Remove(Player);
				}
				return;
			}
			ApplyStreamMovement(Player);
		}), ApplyInterval, true);

		ActivePlayers.Add(Player, TimerHandle);

		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Wind Stream Active!"));
	}
}

void AWindStreamZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (!Player) return;

	if (FTimerHandle* Handle = ActivePlayers.Find(Player))
	{
		GetWorldTimerManager().ClearTimer(*Handle);
		ActivePlayers.Remove(Player);
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan, TEXT("Wind Stream Ended"));
	}
}

void AWindStreamZone::ApplyStreamMovement(APlayerCharacter* Player)
{
	if (!Player) return;
	UCharacterMovementComponent* MoveComp = Player->GetCharacterMovement();
	if (!MoveComp) return;

	FVector Dir = StreamDirection.GetSafeNormal();
	FVector Vel = MoveComp->Velocity;

	// Boost constant dans la direction du courant
	Vel += Dir * BoostForce * ApplyInterval;

	// Optionnel : verrouille l’orientation vers la direction
	Player->SetActorRotation(Dir.Rotation());

	MoveComp->Velocity = Vel;
}
