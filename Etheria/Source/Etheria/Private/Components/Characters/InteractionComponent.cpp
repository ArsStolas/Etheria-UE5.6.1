// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Characters/InteractionComponent.h"
#include "Interfaces/Interaction.h"
#include "Camera/CameraComponent.h"

// Sets default values for this component's properties
UInteractionComponent::UInteractionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	Camera = Owner->FindComponentByClass<UCameraComponent>();
}


// Called every frame
void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UInteractionComponent::CheckInteraction();
}

void UInteractionComponent::CheckInteraction()
{
	if (GetOwner() && Camera != nullptr)
	{
		FVector CameraLocation = Camera->GetComponentLocation();
		FVector CameraForward = Camera->GetForwardVector();

		FVector End = CameraLocation + CameraForward * InteractDistance;

		FHitResult HitResult;

		FCollisionQueryParams TraceParams(FName(TEXT("LineTrace")), true, GetOwner());
		TraceParams.bReturnPhysicalMaterial = false;
		TraceParams.bTraceComplex = true;

		bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			CameraLocation,
			End,
			ECC_Visibility,
			TraceParams
		);

		if (bHit) {
			AActor* HitActor = HitResult.GetActor();
			if (HitActor)
			{
				if (HitActor->GetClass()->ImplementsInterface(UInteraction::StaticClass()))
				{
					UE_LOG(LogTemp, Log, TEXT("L’acteur %s est interactif !"), *HitActor->GetName());

					//IInteractionInterface::Execute_Interact(HitActor, GetOwner());
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("%s n’a pas l’interface Interaction."), *HitActor->GetName());
				}
			}
		}
	}
}

