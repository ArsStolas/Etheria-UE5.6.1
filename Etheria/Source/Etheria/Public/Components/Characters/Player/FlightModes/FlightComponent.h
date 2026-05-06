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

UENUM(BlueprintType)
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
	UFUNCTION(BlueprintPure, Category="Flight")
	EFlightMode GetCurrentMode() const { return CurrentMode; }
	UFUNCTION(BlueprintPure, Category="Flight|Dive")
	FVector2D GetDiveDirection() const { return DiveDirection; }
	UFUNCTION(BlueprintPure, Category="Flight|Dive")
	float GetDiveBankDirection() const { return DiveDirection.X; }
	UFUNCTION(BlueprintPure, Category="Flight|Dive")
	float GetDivePitchDirection() const { return DiveDirection.Y; }

	void StartGlide();
	void StartDive();
	void StopMode();
	void HandleLandingState();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	FORCEINLINE UGlideMode* GetGlideMode() const { return GlideMode; }
	UFUNCTION(BlueprintPure, Category="Flight|Dive")
	UDiveMode* GetDiveMode() const { return DiveMode; }

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

	/** Maximum autonomous dive speed in cm/s. Wind streams can temporarily push above this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="500", ClampMax="5000"))
	float DiveMaxSpeed = 2600.f;

	/** Minimum retained dive speed; keeps climb stalls controllable instead of killing movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="100", ClampMax="1000"))
	float DiveMinSpeed = 300.f;

	/** Speed the dive returns toward when the player is neither pitching down nor pulling up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="500", ClampMax="5000"))
	float DiveCruiseSpeed = 1500.f;

	/** Acceleration gained when the player dives downward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="500", ClampMax="3000"))
	float DiveAcceleration = 1400.f;

	/** Main deceleration budget used by climb, neutral drag, and banking drag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="200", ClampMax="2500"))
	float DiveDeceleration = 600.f;

	/** Extra speed added when entering dive, on top of current horizontal fall/glide momentum. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="0", ClampMax="1500"))
	float DiveEntrySpeedBonus = 250.f;

	/** How quickly wind streams ramp the dive toward their requested target speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Wind", meta=(ClampMin="500", ClampMax="20000"))
	float DiveWindStreamAcceleration = 2400.f;

	/** Minimum height in cm required to enter dive, preventing immediate ground collisions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Validation", meta=(ClampMin="100", ClampMax="1000"))
	float DiveMinimumHeight = 300.f;

	/** Maximum pitch angle. Forward input dives down; backward input pulls up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation", meta=(ClampMin="15", ClampMax="90"))
	float DiveMaxPitch = 60.f;

	/** Maximum left/right bank angle used by steering and animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation", meta=(ClampMin="10", ClampMax="90"))
	float DiveMaxRoll = 30.f;

	/** Base yaw rate from banking; current speed scales it up slightly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation", meta=(ClampMin="30", ClampMax="180"))
	float DiveTurnRate = 90.f;

	/** Multiplier for how much retained speed can become climb lift. 0.6 is neutral/default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Vertical", meta=(ClampMin="0.1", ClampMax="2.0"))
	float DiveLiftFactor = 0.6f;

	/** Pitch smoothing. Higher values feel snappier and more arcade. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation", meta=(ClampMin="1.0", ClampMax="12.0"))
	float DivePitchResponse = 5.5f;

	/** Roll smoothing. Higher values make left/right banking react faster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation", meta=(ClampMin="1.0", ClampMax="12.0"))
	float DiveRollResponse = 7.f;

	/** How quickly neutral dive returns toward DiveCruiseSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="0.1", ClampMax="10.0"))
	float DiveCruiseInterpSpeed = 1.4f;

	/** Extra speed loss while banking hard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="0.0", ClampMax="2.0"))
	float DiveTurnDrag = 0.28f;

	/** Normal downward speed in cm/s with no pitch input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Vertical", meta=(ClampMin="50", ClampMax="1500"))
	float DiveNeutralSinkSpeed = 260.f;

	/** Downward speed in cm/s when fully diving downward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Vertical", meta=(ClampMin="500", ClampMax="4000"))
	float DiveMaxSinkSpeed = 2300.f;

	/** Upward speed in cm/s when pulling up with enough speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Vertical", meta=(ClampMin="100", ClampMax="1500"))
	float DiveMaxClimbSpeed = 1200.f;

	/** Extra sink applied to climb attempts at low speed to prevent infinite flight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Vertical", meta=(ClampMin="0", ClampMax="1500"))
	float DiveLowSpeedClimbSink = 180.f;

	/** Speed-loss multiplier while pulling up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="0.5", ClampMax="4.0"))
	float DiveClimbSpeedCostMultiplier = 1.2f;

	/** If enabled, movement inputs also pitch/roll the character. If disabled, inputs affect physics only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation")
	bool bDiveUseInputAttitude = false;

	/** Enables dive logs and on-screen debug messages in non-shipping builds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Debug")
	bool bDiveDebugMode = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Flight|Dive")
	FVector2D DiveDirection = FVector2D::ZeroVector;

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
