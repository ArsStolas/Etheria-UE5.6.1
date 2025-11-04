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
					if (GEngine != nullptr) {
						GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Cyan, FString("L’acteur %s est interactif !"));
					}

					InteractableActor = HitActor;

					if (OverlayMaterial != nullptr) {
						UMeshComponent* StaticMesh = InteractableActor->FindComponentByClass<UMeshComponent>();
						if (StaticMesh != nullptr) {
							StaticMesh->SetOverlayMaterial(OverlayMaterial);
						}
					}

					return;
				}
				else
				{
					if (GEngine != nullptr) {
						GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Cyan, FString("%s n’a pas l’interface Interaction."));
					}
				}
			}
		}


		if (InteractableActor != nullptr) {
			UMeshComponent* StaticMesh = InteractableActor->FindComponentByClass<UMeshComponent>();
			if (StaticMesh != nullptr) {
				StaticMesh->SetOverlayMaterial(nullptr);
			}
		}

		InteractableActor = nullptr;
	}
}

void UInteractionComponent::Interact() {
	if (InteractableActor != nullptr && InteractableActor->GetClass()->ImplementsInterface(UInteraction::StaticClass())) {
		IInteraction::Execute_Interact(InteractableActor);
	}
}