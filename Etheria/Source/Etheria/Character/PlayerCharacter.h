// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class ETHERIA_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// === Constructeur ===
	APlayerCharacter();

	// === Tick ===
	virtual void Tick(float DeltaTime) override;

	// === Input ===
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// === Début du jeu ===
	virtual void BeginPlay() override;

	// === Composants ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* FollowCamera;

	// === Mouvement ===
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float WalkSpeed = 600.f;

	// === Saut / Crouch ===
	void StartJump();
	void StopJump();

	void CrouchPressed();
	void CrouchReleased();
	bool bIsCrouchingHold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Crouch")
	float CrouchSpeed = 300.f;

	// === Camera | Crouch Offset ===
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float StandingCameraZ = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CrouchingCameraZ = 30.f;

	// Caméra interpolée
	float TargetCameraZ;

	// === Sprint ===
	void SprintPressed();
	void SprintReleased();
	bool bIsSprinting;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Sprint")
	float MaxSprintStamina = 5.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Sprint")
	float CurrentSprintStamina;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Sprint")
	float SprintSpeedMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Sprint")
	float SprintStaminaDrainRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Sprint")
	float SprintStaminaRecoveryRate = 0.5f;

	// === Input Actions (Enhanced Input) ===
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* PlayerMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SprintAction;
};
