// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractionComponent.generated.h"

class UCameraComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ETHERIA_API UInteractionComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractionComponent();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category="Interaction")
	float InteractDistance = 50.0f;

	UPROPERTY(EditAnywhere, Category="Interaction")
	UMaterialInterface* OverlayMaterial;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	void CheckInteraction();
	void Interact();

	UCameraComponent* Camera;
	AActor* InteractableActor;
};
