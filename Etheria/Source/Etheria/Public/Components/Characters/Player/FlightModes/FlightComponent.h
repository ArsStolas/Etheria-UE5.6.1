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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Glide", meta=(ClampMin="500", ClampMax="2000"))
	float GlideSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Glide", meta=(ClampMin="100", ClampMax="1000"))
	float GlideDescendRate = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Glide", meta=(ClampMin="0.5", ClampMax="10.0"))
	float GlideInterpSpeed = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Glide", meta=(ClampMin="0.5", ClampMax="10.0"))
	float GlideDescentInterpSpeed = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Glide", meta=(ClampMin="100", ClampMax="1000"))
	float GlideMinimumHeight = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="500", ClampMax="5000"))
	float DiveMaxSpeed = 2600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="100", ClampMax="1000"))
	float DiveMinSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="500", ClampMax="2000"))
	float DiveAcceleration = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="200", ClampMax="1500"))
	float DiveDeceleration = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="0", ClampMax="1500"))
	float DiveEntrySpeedBonus = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="500", ClampMax="20000"))
	float DiveWindStreamAcceleration = 2400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="100", ClampMax="1000"))
	float DiveMinimumHeight = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="15", ClampMax="90"))
	float DiveMaxPitch = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="10", ClampMax="90"))
	float DiveMaxRoll = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="30", ClampMax="180"))
	float DiveTurnRate = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive", meta=(ClampMin="0.1", ClampMax="2.0"))
	float DiveLiftFactor = 0.6f;

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
