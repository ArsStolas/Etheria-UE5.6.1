/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: 0nnen
 * Class: PlayerCharacter - Header
 */

#pragma once
#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "Components/Inventory/InventoryComponent.h"
#include "Components/Interaction/InteractorComponent.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class ETHERIA_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const struct FInputActionValue& Value);
	void Look(const struct FInputActionValue& Value);

	void StartCrouch(const FInputActionValue& Value);
	void StopCrouch(const FInputActionValue& Value);

	void StartSprint(const FInputActionValue& Value);
	void StopSprint(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Components")
	UCameraComponent* FollowCamera;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 900.f;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Player", meta=(AllowPrivateAccess="true"))
	UInventoryComponent* InventoryComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Player", meta=(AllowPrivateAccess="true"))
	UInteractorComponent* InteractorComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
	UInputAction* NextItemAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
	UInputAction* PrevItemAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
	UInputAction* UseItemAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
	UInputAction* DropItemAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input")
	UInputAction* InteractAction; // (Pick up)

	UFUNCTION() void Input_SelectNext();
	UFUNCTION() void Input_SelectPrev();
	UFUNCTION() void Input_UseItem();
	UFUNCTION() void Input_DropItem();
	UFUNCTION() void Input_Interact();
private:
};
