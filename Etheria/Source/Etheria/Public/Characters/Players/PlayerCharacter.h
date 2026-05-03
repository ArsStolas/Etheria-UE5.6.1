/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Mato
 * Class: PlayerCharacter - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Characters/BaseCharacter.h"
#include "PlayerCharacter.generated.h"

class UCableComponent;
class URopeLengthControllerComponent;
class URopeConstraintComponent;
class URopeSwingComponent;
class URopeAttachComponent;
class URopeLockComponent;
class URopeDetectionComponent;
class URopeCameraComponent;
class URopePullComponent;
struct FInputActionValue;

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UFlightComponent;
class UInventoryComponent;
//class UInteractionComponent;
class ULockTargetComponent;
class ULockVisualComponent;
class USwimComponent;

// ============================================================
// AXIS STATE STRUCT
// ============================================================
USTRUCT()
struct FAxisPressState
{
    GENERATED_BODY();

    bool bNegPressed = false;
    bool bPosPressed = false;
    double NegLastTime = -DBL_MAX;
    double PosLastTime = -DBL_MAX;

    float GetAxisValue() const
    {
        if (bNegPressed && bPosPressed)
            return (PosLastTime > NegLastTime) ? 1.f : -1.f;

        if (bNegPressed)  return -1.f;
        if (bPosPressed)  return  1.f;
        return 0.f;
    }

    void OnNegStarted(double Time) { bNegPressed = true;  NegLastTime = Time; }
    void OnPosStarted(double Time) { bPosPressed = true;  PosLastTime = Time; }
    void OnNegCompleted()          { bNegPressed = false; }
    void OnPosCompleted()          { bPosPressed = false; }
};

// ============================================================
// PLAYER CHARACTER
// ============================================================
UCLASS()
class ETHERIA_API APlayerCharacter : public ABaseCharacter
{
    GENERATED_BODY()

public:
    APlayerCharacter();

    FORCEINLINE float GetHorizontalInput() const { return Horizontal.GetAxisValue(); }
    FORCEINLINE float GetVerticalInput() const   { return Vertical.GetAxisValue(); }
    FORCEINLINE UFlightComponent* GetFlightComponent() const { return FlightComponent; }
    FORCEINLINE UCableComponent* GetRopeCableComponent() const { return RopeCableComponent; }
    FORCEINLINE URopeDetectionComponent* GetRopeDetectionComponent() const { return RopeDetectionComponent; }
    FORCEINLINE URopeAttachComponent* GetRopeAttachComponent() const { return RopeAttachComponent; }
    FORCEINLINE URopeLockComponent* GetRopeLockComponent() const { return RopeLockComponent; }
    FORCEINLINE URopeConstraintComponent* GetRopeConstraintComponent() const { return RopeConstraintComponent; }
    FORCEINLINE URopeSwingComponent* GetRopeSwingComponent() const { return RopeSwingComponent; }
    FORCEINLINE URopeLengthControllerComponent* GetRopeLengthControllerComponent() const { return RopeLengthControllerComponent; }
    FORCEINLINE URopePullComponent* GetRopePullComponent() const { return RopePullComponent; }
    FORCEINLINE USwimComponent* GetSwimComponent() const { return SwimComponent; }
    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    
    // Simple accessor
    bool IsGrounded() const;
    
protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    
// ============================================================
// COMPONENTS
// ============================================================
#pragma region COMPONENTS

    // --- CAMERA ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* FollowCamera;

    // --- FLIGHT MODE ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FlightMode")
    UFlightComponent* FlightComponent;

    // --- INVENTORY ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
    UInventoryComponent* InventoryComponent;

    // --- INTERACTOR ---
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction", meta=(AllowPrivateAccess="true"))
    //UInteractionComponent* InteractionComponent;

    // --- LOCK TARGET ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Combat|LockTarget", meta=(AllowPrivateAccess="true"))
    ULockTargetComponent* LockTargetComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Combat|LockTarget", meta=(AllowPrivateAccess="true"))
    ULockVisualComponent* LockVisualComponent;

    // --- ROPE ---
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
    UCableComponent* RopeCableComponent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rope")
    URopeDetectionComponent* RopeDetectionComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rope")
    URopeLockComponent* RopeLockComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rope")
    URopeAttachComponent* RopeAttachComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rope")
    URopeConstraintComponent* RopeConstraintComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rope")
    URopeSwingComponent* RopeSwingComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rope")
    URopeLengthControllerComponent* RopeLengthControllerComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rope")
    URopeCameraComponent* RopeCameraComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rope")
    URopePullComponent* RopePullComponent;
    
    /** Socket name where the rope attaches on the character mesh*/
    UPROPERTY(EditAnywhere, Category="Rope|Attachment")
    FName RopeStartSocketName = TEXT("hand_r");
    
    // --- SWIM ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Movement", meta=(AllowPrivateAccess="true"))
    USwimComponent* SwimComponent;

#pragma endregion

// ============================================================
// INPUT SYSTEM
// ============================================================
#pragma region INPUT ACTIONS

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Context")
    UInputMappingContext* PlayerContext;

    // --- CAMERA ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Camera")
    UInputAction* LookAction;
    void OnAimPressed();
    void OnAimReleased();

    // --- MOVEMENT ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    UInputAction* ForwardAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    UInputAction* BackAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    UInputAction* LeftAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    UInputAction* RightAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    UInputAction* JumpAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    UInputAction* CrouchAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Movement")
    UInputAction* SprintAction;

    // --- GLIDER ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Glider")
    UInputAction* GliderAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Glider")
    UInputAction* DiveAction;

    // --- INVENTORY ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Inventory")
    UInputAction* NextItemAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Inventory")
    UInputAction* PrevItemAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Inventory")
    UInputAction* UseItemAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Inventory")
    UInputAction* DropItemAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Inventory")
    UInputAction* InteractAction;

    // --- COMBAT ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat")
    UInputAction* AttackLightAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat")
    UInputAction* AttackHeavyAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat")
    UInputAction* ParryAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat")
    UInputAction* DodgeAction;

    // --- LOCK TARGET ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat|LockTarget")
    UInputAction* LockToggleAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat|LockTarget")
    UInputAction* LockSwitchLeftAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat|LockTarget")
    UInputAction* LockSwitchRightAction;

    // --- ROPE ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Rope")
    UInputAction* AttachRopeAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Rope")
    UInputAction* RopeLengthAction;

    // --- SWIM ---
    /** Optional swim dive input. If unset, DiveAction is used. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input|Swim", meta=(ToolTip="Optional swim dive input. If unset, DiveAction is used for swim too."))
    UInputAction* SwimDiveAction = nullptr;

    /** Optional swim sprint input. If unset, SprintAction is used. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input|Swim", meta=(ToolTip="Optional swim sprint input. If unset, SprintAction is used for swim sprint too."))
    UInputAction* SwimSprintAction = nullptr;
#pragma endregion

// ============================================================
// MOVEMENT CONFIG
// ============================================================
#pragma region MOVEMENT CONFIG

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float WalkSpeed = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeed = 900.f;

    UPROPERTY(EditAnywhere, Category = "Movement|Jump Buffer")
    float JumpBufferTime = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Movement|State")
    float AirborneStateGraceDuration = 0.18f;

    bool  bJumpBuffered = false;
    float JumpBufferExpireAt = 0.f;
    bool  bLockJumpCrouchFromCombat = false;

#pragma endregion

// ============================================================
// AIMING 
// ============================================================
#pragma region AIMING

    /** True when the player is aiming (right click held). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aiming")
    bool bIsAiming = false;

    /** Base FOV for normal gameplay (cached at BeginPlay). */
    float BaseFOV = 90.f;

    /** Base arm length for normal camera (cached at BeginPlay). */
    float BaseArmLength = 350.f;

    /** Called to enable or disable aiming mode. */
    void ToggleAiming(bool bEnable);

    /** Simple accessor. */
    bool IsAiming() const { return bIsAiming; }

#pragma endregion
    
// ============================================================
// STATE & AXIS TRACKING
// ============================================================
private:
    FAxisPressState Horizontal;
    FAxisPressState Vertical;
    
    float AirborneIgnoreUntil = 0.f;
    
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDebugMovementStateLogs = false;
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDebugCombatStateLogs = false;
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDebugLifeStateLogs   = false;

// ============================================================
// INTERNAL HANDLERS
// ============================================================
#pragma region HANDLERS

    // --- Movement ---
    void OnForwardStarted(const FInputActionValue&);   void OnForwardCompleted(const FInputActionValue&);
    void OnBackStarted(const FInputActionValue&);      void OnBackCompleted(const FInputActionValue&);
    void OnLeftStarted(const FInputActionValue&);      void OnLeftCompleted(const FInputActionValue&);
    void OnRightStarted(const FInputActionValue&);     void OnRightCompleted(const FInputActionValue&);
    void StartSprint();                                void StopSprint();
    void OnCrouchPressed();                            void StopCrouch();
    void OnJumpPressed();                              virtual void Landed(const FHitResult&) override;

    // --- Movement State ---
    void HandleMovementInput();
    void UpdateMovementState();
    void HandleAirborneState();
    void HandleGroundedState();

    // --- Camera ---
    void Look(const FInputActionValue& Value);

    // --- Glider ---
    void ToggleGlideMode();                            void ToggleDiveMode();
    void AlignToCamera();
    UFUNCTION() void OnGlideStart();                   UFUNCTION() void OnGlideStop();
    UFUNCTION() void OnDiveStart();                    UFUNCTION() void OnDiveStop();

    // --- Combat ---
    void OnAttackLightPressed();   void OnAttackLightReleased();
    void OnAttackHeavyPressed();   void OnAttackHeavyReleased();   
    void OnAttackHeavyCanceled();
    void OnParryPressed();         void OnParryReleased();
    void OnDodgePressed();
    UFUNCTION() void HandleAttackStart(FName AttackId);
    UFUNCTION() void HandleAttackEnd(FName AttackId);

    // --- Lock Target ---
    void OnLockToggle();
    void OnLockSwitchLeft();
    void OnLockSwitchRight();

    // --- Inventory ---
    UFUNCTION() void Input_SelectNext();   UFUNCTION() void Input_SelectPrev();
    UFUNCTION() void Input_UseItem();      UFUNCTION() void Input_DropItem();

    // --- Interactor ---
    UFUNCTION() void Input_Interact();

    // --- Rope ---
    void OnRopeAttachPressed();
    void CheckRopeAttachMode();
    void RopeLengthInput(const FInputActionValue& Value);
    void StopRopeLengthInput(const FInputActionValue& Value);
    
    // --- Swim / Dive Input Routing ---
    void OnDiveInputPressed();
    void OnDiveInputReleased();

#pragma endregion

// ============================================================
// STATE LOGGING
// ============================================================
#pragma region STATE LOGS

    UFUNCTION() void LogMovementStateChanged(FGameplayTag Previous, FGameplayTag New);
    UFUNCTION() void LogCombatStateChanged(FGameplayTag Previous, FGameplayTag New);
    UFUNCTION() void LogLifeStateChanged(FGameplayTag Previous, FGameplayTag New);
    UFUNCTION() void LogHealthChanged(float NewHealth, float MaxHealth);

#pragma endregion

#pragma region UTILITIES
    
#define BIND_IF(Condition, Delegate, Function) \
if (Condition) { Delegate.AddDynamic(this, &APlayerCharacter::Function); }

#pragma endregion

// ============================================================
// STATE LOGGING
// ============================================================
#pragma region "COMMANDS EXEC"

    UFUNCTION(Exec)
    void DamageSelf(float Amount = 10.f);

    UFUNCTION(Exec)
    void HealSelf(float Amount = 10.f);

    UFUNCTION(Exec)
    void SetHPPercent(float Percent = 0.2f);
    
#pragma endregion
};

