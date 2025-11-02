/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
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

    void StartCrouch();
    void StopCrouch();

    void StartSprint();
    void StopSprint();

    void ToggleGlideMode();
    void ToggleDiveMode();

    void AlignToCamera();

    // === CAMERA COMPONENTS ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
    UCameraComponent* FollowCamera;

    // === GLIDER ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider")
    UGliderComponent* GliderComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider|Visual")
    UStaticMeshComponent* GliderVisual;

    // === INPUT ===
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputMappingContext* PlayerContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* CrouchAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* SprintAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Input")
    UInputAction* GliderAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Input")
    UInputAction* DiveAction;

    // 4 actions séparées
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* ForwardAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* BackAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* LeftAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
    UInputAction* RightAction;

    // === MOVEMENT SPEEDS ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float WalkSpeed = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeed = 900.f;

private:
    // États d’axes
    FAxisPressState Horizontal;
    FAxisPressState Vertical;

    // === CAMERA STATE ===
    bool bIsLookingAround = false;
    float TimeSinceLastLook = 0.f;

    // Handlers
    void OnForwardStarted(const FInputActionValue& Value);
    void OnForwardCompleted(const FInputActionValue& Value);
    void OnBackStarted(const FInputActionValue& Value);
    void OnBackCompleted(const FInputActionValue& Value);
    void OnLeftStarted(const FInputActionValue& Value);
    void OnLeftCompleted(const FInputActionValue& Value);
    void OnRightStarted(const FInputActionValue& Value);
    void OnRightCompleted(const FInputActionValue& Value);
};