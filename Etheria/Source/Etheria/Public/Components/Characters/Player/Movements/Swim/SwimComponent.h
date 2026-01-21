/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "USwimComponent" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SwimComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;

/**
 * Swim mode state for player.
 * - None: walking / not swimming.
 * - Surface: swimming on surface (camera yaw for forward).
 * - Underwater: fully underwater (optionally uses camera pitch for forward).
 */
UENUM(BlueprintType)
enum class EPlayerSwimMode : uint8
{
	None       UMETA(DisplayName="None"),
	Surface    UMETA(DisplayName="Surface"),
	Underwater UMETA(DisplayName="Underwater"),
};

/** Fired when the player enters/exits water (PhysicsVolume bWaterVolume). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInWaterChanged, bool, bNowInWater);

/** Fired when swim mode changes (None/Surface/Underwater). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSwimModeChanged, EPlayerSwimMode, Previous, EPlayerSwimMode, NewMode);

/**
 * Blueprint-friendly swim system (GTA-like) driven by CharacterMovement.
 * - Auto activates swim when the character is inside a Water PhysicsVolume and has no foot (no walkable floor).
 * - Can dive underwater with an input (toggle or hold).
 * - Sprint increases swim speed.
 *
 * IMPORTANT:
 * This component does NOT change your existing movement bindings by itself.
 * You can either:
 * 1) Call Input_SetMoveAxis() from your input axis events (recommended for BP), OR
 * 2) Call ApplyCachedSwimMovementInput() yourself while swimming.
 */
UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class ETHERIA_API USwimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USwimComponent();

#pragma region BP_Getters
	/** True if the owner is currently inside a Water PhysicsVolume (bWaterVolume). */
	UFUNCTION(BlueprintPure, Category="Swim")
	bool IsInWater() const { return bInWater; }

	/** True if swim state is active (Surface or Underwater). */
	UFUNCTION(BlueprintPure, Category="Swim")
	bool IsSwimmingActive() const { return CurrentMode != EPlayerSwimMode::None; }

	/** True if current mode is Underwater. */
	UFUNCTION(BlueprintPure, Category="Swim")
	bool IsUnderwater() const { return CurrentMode == EPlayerSwimMode::Underwater; }

	/** Get current swim mode. */
	UFUNCTION(BlueprintPure, Category="Swim")
	EPlayerSwimMode GetSwimMode() const { return CurrentMode; }

	/** Get whether sprint swim is enabled. */
	UFUNCTION(BlueprintPure, Category="Swim")
	bool IsSwimSprinting() const { return bSwimSprinting; }
#pragma endregion

#pragma region BP_Input
	/**
	 * Cache movement axes for swimming.
	 * Call this from your input axis events (MoveForward/MoveRight) while in water.
	 * The component will apply the movement each Tick when swimming (if bAutoApplyCachedInput is enabled).
	 */
	UFUNCTION(BlueprintCallable, Category="Swim|Input")
	void Input_SetMoveAxis(float ForwardAxis, float RightAxis);

	/** Press Dive: enter Underwater (toggle/hold depending on bDiveToggle). */
	UFUNCTION(BlueprintCallable, Category="Swim|Input")
	void Input_DivePressed();

	/** Release Dive (only meaningful if bDiveToggle is false = hold). */
	UFUNCTION(BlueprintCallable, Category="Swim|Input")
	void Input_DiveReleased();

	/** Enable/disable swim sprint (faster swim speed). */
	UFUNCTION(BlueprintCallable, Category="Swim|Input")
	void SetSwimSprinting(bool bSprint);
#pragma endregion

#pragma region BP_Control
	/** Force swim mode (useful for scripted sequences or debug). */
	UFUNCTION(BlueprintCallable, Category="Swim")
	void ForceSwimMode(EPlayerSwimMode NewMode);

	/** Stop swimming and return to walking movement mode if possible. */
	UFUNCTION(BlueprintCallable, Category="Swim")
	void StopSwimming();

	/**
	 * Apply cached movement inputs right now (Forward/Right), camera-based.
	 * Use if you disabled bAutoApplyCachedInput and want manual control.
	 */
	UFUNCTION(BlueprintCallable, Category="Swim")
	void ApplyCachedSwimMovementInput();
#pragma endregion

#pragma region Events
	UPROPERTY(BlueprintAssignable, Category="Swim|Events")
	FOnInWaterChanged OnInWaterChanged;

	UPROPERTY(BlueprintAssignable, Category="Swim|Events")
	FOnSwimModeChanged OnSwimModeChanged;
#pragma endregion

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#pragma region Settings
	/** Enable automatic swim activation when inside water volume and no foot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Auto", meta=(AllowPrivateAccess="true"))
	bool bAutoSwim = true;

	/** How often we check the floor while in water (performance-friendly). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Auto", meta=(ClampMin="0.01", AllowPrivateAccess="true"))
	float FloorCheckInterval = 0.10f;

	/** If floor is closer than this distance, we keep walking (shallow water). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Auto", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float MaxFloorDistanceToStayWalking = 45.f;

	/** If swimming on surface and floor comes closer than this, exit swim to walking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Auto", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float MaxFloorDistanceToExitSwim = 55.f;

	/** How deep we trace below the capsule bottom to find floor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Auto", meta=(ClampMin="10.0", AllowPrivateAccess="true"))
	float FloorTraceDepth = 220.f;

	/** Collision channel used for floor trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Auto", meta=(AllowPrivateAccess="true"))
	TEnumAsByte<ECollisionChannel> FloorTraceChannel = ECC_Visibility;

	/** If true, Dive toggles underwater; if false, it's hold-to-dive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Control", meta=(AllowPrivateAccess="true"))
	bool bDiveToggle = true;

	/** If true, underwater forward uses camera pitch (full 3D swim forward). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Control", meta=(AllowPrivateAccess="true"))
	bool bUseCameraPitchUnderwater = true;

	/** If true, cached axis inputs are auto-applied every Tick while swimming. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Control", meta=(AllowPrivateAccess="true"))
	bool bAutoApplyCachedInput = true;

	/** Movement tuning for surface swim. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Tuning", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float SurfaceSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Tuning", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float SurfaceSprintSpeed = 450.f;

	/** Movement tuning for underwater swim. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Tuning", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float UnderwaterSpeed = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Tuning", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float UnderwaterSprintSpeed = 380.f;

	/** Swim acceleration and braking (CharacterMovement). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Tuning", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float SwimAcceleration = 2048.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Tuning", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float BrakingDecelSwimming = 2048.f;

	/** Buoyancy for surface / underwater. Lower buoyancy makes you sink more. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Tuning", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float SurfaceBuoyancy = 1.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swim|Tuning", meta=(ClampMin="0.0", AllowPrivateAccess="true"))
	float UnderwaterBuoyancy = 0.75f;
#pragma endregion

private:
#pragma region CachedRefs
	TWeakObjectPtr<ACharacter> OwnerCharacter;
	UPROPERTY(Transient) UCharacterMovementComponent* MoveComp = nullptr;
#pragma endregion

#pragma region State
	UPROPERTY(Transient)
	bool bInWater = false;

	UPROPERTY(Transient)
	bool bWantsDive = false;

	UPROPERTY(Transient)
	bool bSwimSprinting = false;

	UPROPERTY(Transient)
	EPlayerSwimMode CurrentMode = EPlayerSwimMode::None;

	UPROPERTY(Transient)
	float CachedForwardAxis = 0.f;

	UPROPERTY(Transient)
	float CachedRightAxis = 0.f;

	float NextFloorCheckTime = 0.f;
#pragma endregion

#pragma region Internal
	bool IsOwnerInWaterVolume() const;
	bool FindWalkableFloor(float& OutFloorDist) const;

	void SetMode(EPlayerSwimMode NewMode);
	void ApplyMovementTuning() const;

	void ApplySwimMovementInput(float ForwardAxis, float RightAxis) const;
#pragma endregion
};
