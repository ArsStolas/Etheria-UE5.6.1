/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: PlayerCharacter - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Characters/BaseCharacter.h"
#include "PlayerCharacter.generated.h"

struct FInputActionValue;

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UGliderComponent;
class UInventoryComponent;
class UInteractorComponent;
class ULockTargetComponent;

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

    int32 GetAxisValue() const
    {
        if (bNegPressed && bPosPressed)
            return (PosLastTime > NegLastTime) ? +1 : -1;

        if (bNegPressed)  return -1;
        if (bPosPressed)  return +1;
        return 0;
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

    FORCEINLINE UStaticMeshComponent* GetGliderVisual() const { return GliderVisual; }
    FORCEINLINE int32 GetHorizontalAxis() const { return Horizontal.GetAxisValue(); }
    FORCEINLINE int32 GetVerticalAxis() const { return Vertical.GetAxisValue(); }
    FORCEINLINE UGliderComponent* GetGliderComponent() const { return GliderComponent; }

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

    // --- GLIDER ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider")
    UGliderComponent* GliderComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider|Visual")
    UStaticMeshComponent* GliderVisual;

    // --- INVENTORY ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
    UInventoryComponent* InventoryComponent;

    // --- INTERACTOR ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interactor", meta=(AllowPrivateAccess="true"))
    UInteractorComponent* InteractorComponent;

    // --- LOCK TARGET ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LockTarget", meta=(AllowPrivateAccess="true"))
    ULockTargetComponent* LockTargetComponent;

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
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|LockTarget")
    UInputAction* LockToggleAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|LockTarget")
    UInputAction* LockSwitchLeftAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|LockTarget")
    UInputAction* LockSwitchRightAction;

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

    bool  bJumpBuffered = false;
    float JumpBufferExpireAt = 0.f;
    bool  bLockJumpCrouchFromCombat = false;

#pragma endregion

// ============================================================
// STATE & AXIS TRACKING
// ============================================================
private:
    FAxisPressState Horizontal;
    FAxisPressState Vertical;

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

#pragma endregion

// ============================================================
// STATE LOGGING
// ============================================================
#pragma region STATE LOGS

    UFUNCTION() void LogMovementStateChanged(FGameplayTag Previous, FGameplayTag New);
    UFUNCTION() void LogCombatStateChanged(FGameplayTag Previous, FGameplayTag New);
    UFUNCTION() void LogLifeStateChanged(FGameplayTag Previous, FGameplayTag New);
    UFUNCTION() void LogHealthChanged(float NewHealth, float MaxHealth);
    UFUNCTION() void LogDeath();

#pragma endregion
};
