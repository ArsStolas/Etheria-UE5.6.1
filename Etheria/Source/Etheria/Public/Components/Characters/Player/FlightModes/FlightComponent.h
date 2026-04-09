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

// ---- Delegates ----
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGlideEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDiveEvent);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UFlightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlightComponent();

	bool IsInMode(const EFlightMode Mode) const { return CurrentMode == Mode; }

	void StartGlide();
	void StartDive();
	void StopMode();
	void HandleLandingState();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	FORCEINLINE UGlideMode* GetGlideMode() const { return GlideMode; }
	FORCEINLINE UDiveMode* GetDiveMode() const { return DiveMode; }

	UPROPERTY(BlueprintAssignable, Category="Flight")
	FGlideEvent OnGlideStart;

	UPROPERTY(BlueprintAssignable, Category="Flight")
	FGlideEvent OnGlideStop;

	UPROPERTY(BlueprintAssignable, Category="Flight")
	FDiveEvent OnDiveStart;

	UPROPERTY(BlueprintAssignable, Category="Flight")
	FDiveEvent OnDiveStop;

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
