/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: FlightComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlightComponent.generated.h"

UENUM()
enum class EFlightMode : uint8
{
	None,
	Glide,
	Dive
};

class UFlightModeBase;
class UGlideMode;
class UDiveMode;
class APlayerCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UFlightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlightComponent();

	void StartGlide();
	void StartDive();
	void StopMode();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	EFlightMode CurrentMode = EFlightMode::None;

	UPROPERTY()
	UGlideMode* GlideMode;

	UPROPERTY()
	UDiveMode* DiveMode;

	UPROPERTY()
	UFlightModeBase* ActiveMode;

	APlayerCharacter* Owner;
};
