/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: PlayerCharacter - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UGliderComponent;

UCLASS()
class ETHERIA_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	UStaticMeshComponent* GetGliderVisual() const;
	FORCEINLINE FVector2D GetMoveInput() const { return MoveInput; }

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);

	void StartCrouch();
	void StopCrouch();

	void StartSprint();
	void StopSprint();

	void ToggleGlideMode();
	void ToggleDiveMode();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider")
	UGliderComponent* GliderComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glider|Visual")
	UStaticMeshComponent* GliderVisual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
	UInputMappingContext* PlayerContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Input")
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInput")
	UInputAction* GliderAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerInput")
	UInputAction* DiveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 900.f;

private:
	FVector2D MoveInput = FVector2D::ZeroVector;
};
