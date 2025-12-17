// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RopeAttachPoint.generated.h"

UENUM(BlueprintType)
enum class ERopeAttachType : uint8
{
	Swing UMETA(DisplayName = "Swing"),
	Pull  UMETA(DisplayName = "Pull")
};

class USphereComponent;

UCLASS()
class ETHERIA_API ARopeAttachPoint : public AActor
{
	GENERATED_BODY()

public:
	ARopeAttachPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope")
	ERopeAttachType AttachType = ERopeAttachType::Swing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rope")
	float DetectionRadius = 150.f;

	FORCEINLINE ERopeAttachType GetAttachType() const { return AttachType; }

protected:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere)
	USphereComponent* DetectionSphere;
};
