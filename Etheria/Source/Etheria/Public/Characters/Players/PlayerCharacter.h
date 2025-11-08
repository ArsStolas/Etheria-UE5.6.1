/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: 0nnen
 * Class: PlayerCharacter - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "InputActionValue.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UGliderComponent;
class UInventoryComponent;
class UInteractorComponent;
class ULockTargetComponent;

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
        {
            return (PosLastTime > NegLastTime) ? +1 : -1;
        }
        if (bNegPressed) return -1;
        if (bPosPressed) return +1;
        return 0;
    }

    void OnNegStarted(double Time) { bNegPressed = true;  NegLastTime = Time; }
    void OnPosStarted(double Time) { bPosPressed = true;  PosLastTime = Time; }
    void OnNegCompleted()          { bNegPressed = false; }
    void OnPosCompleted()          { bPosPressed = false; }
};

UCLASS()
class ETHERIA_API APlayerCharacter : public ABaseCharacter
{
    GENERATED_BODY()

public:
    APlayerCharacter();

    UStaticMeshComponent* GetGliderVisual() const { return GliderVisual; }
    bool IsInSpecialMode() const;

    // Exposés pour le GliderComponent
    int32 GetHorizontalAxis() const { return Horizontal.GetAxisValue(); }
    int32 GetVerticalAxis() const { return Vertical.GetAxisValue(); }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void Look(const struct FInputActionValue& Value);

    void OnJumpPressed();
    
    void OnCrouchPressed();
    void StartCrouch();
    void StopCrouch();

    void StartSprint();
    void StopSprint();

    void ToggleGlideMode();
    void ToggleDiveMode();

    void AlignToCamera();

    UFUNCTION() void Input_SelectNext();
    UFUNCTION() void Input_SelectPrev();
    UFUNCTION() void Input_UseItem();
    UFUNCTION() void Input_DropItem();
    UFUNCTION() void Input_Interact();
    
    UFUNCTION() void HandleAttackStart(FName AttackId);
    UFUNCTION() void HandleAttackEnd(FName AttackId);
    
#pragma region "COMPONENTS"
    // === CAMERA COMPONENTS ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
    UCameraComponent* FollowCamera;

    // === GLIDER COMPONENT ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider")
    UGliderComponent* GliderComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider|Visual")
    UStaticMeshComponent* GliderVisual;

    // === INVENTORY COMPONENTS ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory Component", meta=(AllowPrivateAccess="true"))
    UInventoryComponent* InventoryComponent;

    // === Interactor COMPONENTS ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interactor Component", meta=(AllowPrivateAccess="true"))
    UInteractorComponent* InteractorComponent;

    // === LOCK TARGET COMPONENTS ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lock Target Components", meta=(AllowPrivateAccess="true"))
    ULockTargetComponent* LockTargetComponent;
#pragma endregion
    
#pragma region "INPUTS"
    // --===-- INPUTS --===--
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputMappingContext* PlayerContext;

    // === CAMERA ACTION ===
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* LookAction;
    
    // === MOVEMENTS ACTIONS ===
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* ForwardAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* BackAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* LeftAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* RightAction;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* CrouchAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* SprintAction;

    // === GLIDER ACTIONS ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Input")
    UInputAction* GliderAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Input")
    UInputAction* DiveAction;

    // === INVENTORY ACTIONS ===
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* NextItemAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* PrevItemAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* UseItemAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* DropItemAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* InteractAction;
    
    // === COMBAT ACTIONS ===
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* AttackLightAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* AttackHeavyAction; // used for charge attacks

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* ParryAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* DodgeAction;

    // === LOCK TARGET ACTIONS ===
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* LockToggleAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* LockSwitchLeftAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
    UInputAction* LockSwitchRightAction;

#pragma endregion
    
    // === MOVEMENT SPEEDS ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float WalkSpeed = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeed = 900.f;

    UPROPERTY(EditAnywhere, Category="Input") float JumpBufferTime = 0.25f;
    bool bJumpBuffered = false;
    float JumpBufferExpireAt = 0.f;
    bool bLockJumpCrouchFromCombat = false;

private:
    // STATES AXIS
    FAxisPressState Horizontal;
    FAxisPressState Vertical;

    // === CAMERA STATE ===
    bool bIsLookingAround = false;
    float TimeSinceLastLook = 0.f;

#pragma region "HANDLERS"
    // === MOVEMENTS HANDLERS ===
    void OnForwardStarted(const FInputActionValue& Value);
    void OnForwardCompleted(const FInputActionValue& Value);
    void OnBackStarted(const FInputActionValue& Value);
    void OnBackCompleted(const FInputActionValue& Value);
    void OnLeftStarted(const FInputActionValue& Value);
    void OnLeftCompleted(const FInputActionValue& Value);
    void OnRightStarted(const FInputActionValue& Value);
    void OnRightCompleted(const FInputActionValue& Value);
    
    // === COMBAT HANDLERS ===
    void OnAttackLightPressed();
    void OnAttackLightReleased();
    void OnAttackHeavyPressed();
    void OnAttackHeavyReleased();   // release the charge at current level
    void OnAttackHeavyCanceled();   // cancel the charge if needed
    void OnParryPressed();
    void OnParryReleased();
    void OnDodgePressed();
    void OnLockToggle();
    
    // === LOCK TARGET HANDLERS ===
    void OnLockSwitchLeft();
    void OnLockSwitchRight();
#pragma endregion
};