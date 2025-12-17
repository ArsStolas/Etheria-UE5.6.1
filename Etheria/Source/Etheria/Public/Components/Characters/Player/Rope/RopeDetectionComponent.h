// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeDetectionComponent.generated.h"

class APlayerCharacter;
class ARopeAttachPoint;
class UCameraComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URopeDetectionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	/** Distance max de détection */
	UPROPERTY(EditAnywhere, Category = "Detection")
	float MaxDetectionDistance = 2000.f;

	/** Angle du cône (degrés) */
	UPROPERTY(EditAnywhere, Category = "Detection")
	float DetectionHalfAngle = 25.f;

	// --- Camera cache (per tick) ---
	FVector CachedCameraLocation;
	FVector CachedCameraForward;

	// --- Validation thresholds ---
	UPROPERTY(EditAnywhere, Category = "Detection|Validation")
	float MinCameraDot = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Detection|Validation")
	float MinHeightAboveCamera = -50.f;

	/** Debug */
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugDraw = true;

private:
	APlayerCharacter* OwnerCharacter = nullptr;
	UCameraComponent* Camera = nullptr;

	/** Point actuellement détecté */
	TWeakObjectPtr<ARopeAttachPoint> CurrentPoint;

	void DetectAttachPoint();
	bool IsValidPoint(ARopeAttachPoint* Point, FString& OutFailReason) const;
};
