/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: ArsStolas
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Glide", meta=(ClampMin="500", ClampMax="20000",
		ToolTip="How quickly wind streams ramp the glider toward their requested target speed."))
	float GlideWindStreamAcceleration = 2400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Glide", meta=(ClampMin="0.1", ClampMax="1.0",
		ToolTip="Minimum sideways input (perpendicular to the stream) required to start escaping a wind stream. Below this, sideways input is ignored."))
	float GlideWindEscapeInputThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Glide", meta=(ClampMin="0.05", ClampMax="3.0",
		ToolTip="Seconds of sustained sideways input before the player gains full authority to leave a wind stream."))
	float GlideWindEscapeHoldTime = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="500", ClampMax="5000",
		ToolTip="Maximum autonomous dive speed in cm/s. Wind streams can temporarily push above this."))
	float DiveMaxSpeed = 2600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="100", ClampMax="1000",
		ToolTip="Minimum retained dive speed; keeps climb stalls controllable instead of killing movement."))
	float DiveMinSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="500", ClampMax="5000",
		ToolTip="Speed the dive returns toward when the player is neither pitching down nor pulling up."))
	float DiveCruiseSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="500", ClampMax="3000",
		ToolTip="Acceleration gained when the player dives downward."))
	float DiveAcceleration = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="200", ClampMax="2500",
		ToolTip="Main deceleration budget used by climb, neutral drag, and banking drag."))
	float DiveDeceleration = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="0", ClampMax="1500",
		ToolTip="Extra speed added when entering dive, on top of current horizontal fall/glide momentum."))
	float DiveEntrySpeedBonus = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Wind", meta=(ClampMin="500", ClampMax="20000",
		ToolTip="How quickly wind streams ramp the dive toward their requested target speed."))
	float DiveWindStreamAcceleration = 2400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Validation", meta=(ClampMin="100", ClampMax="1000",
		ToolTip="Minimum height in cm required to enter dive, preventing immediate ground collisions."))
	float DiveMinimumHeight = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation", meta=(ClampMin="15", ClampMax="90",
		ToolTip="Maximum pitch angle. Forward input dives down; backward input pulls up."))
	float DiveMaxPitch = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation", meta=(ClampMin="10", ClampMax="90",
		ToolTip="Maximum left/right bank angle used by steering and animation."))
	float DiveMaxRoll = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation", meta=(ClampMin="30", ClampMax="180",
		ToolTip="Base yaw rate from banking; current speed scales it up slightly."))
	float DiveTurnRate = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Vertical", meta=(ClampMin="0.1", ClampMax="2.0",
		ToolTip="Multiplier for how much retained speed can become climb lift. 0.6 is neutral/default."))
	float DiveLiftFactor = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation", meta=(ClampMin="1.0", ClampMax="12.0",
		ToolTip="Pitch smoothing. Higher values feel snappier and more arcade."))
	float DivePitchResponse = 5.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation", meta=(ClampMin="1.0", ClampMax="12.0",
		ToolTip="Roll smoothing. Higher values make left/right banking react faster."))
	float DiveRollResponse = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="0.1", ClampMax="10.0",
		ToolTip="How quickly neutral dive returns toward DiveCruiseSpeed."))
	float DiveCruiseInterpSpeed = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="0.0", ClampMax="2.0",
		ToolTip="Extra speed loss while banking hard."))
	float DiveTurnDrag = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Vertical", meta=(ClampMin="50", ClampMax="1500",
		ToolTip="Normal downward speed in cm/s with no pitch input."))
	float DiveNeutralSinkSpeed = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Vertical", meta=(ClampMin="500", ClampMax="4000",
		ToolTip="Downward speed in cm/s when fully diving downward."))
	float DiveMaxSinkSpeed = 2300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Vertical", meta=(ClampMin="100", ClampMax="1500",
		ToolTip="Upward speed in cm/s when pulling up with enough speed."))
	float DiveMaxClimbSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Vertical", meta=(ClampMin="0", ClampMax="1500",
		ToolTip="Extra sink applied to climb attempts at low speed to prevent infinite flight."))
	float DiveLowSpeedClimbSink = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Speed", meta=(ClampMin="0.5", ClampMax="4.0",
		ToolTip="Speed-loss multiplier while pulling up."))
	float DiveClimbSpeedCostMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Rotation",
		meta=(ToolTip="If enabled, movement inputs also pitch/roll the character. If disabled, inputs affect physics only."))
	bool bDiveUseInputAttitude = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight|Dive|Debug",
		meta=(ToolTip="Enables dive logs and on-screen debug messages in non-shipping builds."))
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
