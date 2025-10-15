/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: 0nnen
 * Class: PlayerCharacter - Source
*/

#include "Characters/Players/PlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

APlayerCharacter::APlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 300.f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    InventoryComponent  = CreateDefaultSubobject<UInventoryComponent>(TEXT("BPC_Inventory"));
    InteractorComponent = CreateDefaultSubobject<UInteractorComponent>(TEXT("BPC_Interactor"));
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (const APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                if (PlayerContext)
                {
                    Subsystem->AddMappingContext(PlayerContext, 0);
                }
            }
        }
    }
}

void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction)
        {
            EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
        }
        if (LookAction)
        {
            EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
        }
        if (JumpAction)
        {
            EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }
        if (CrouchAction)
        {
            EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::StartCrouch);
            EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopCrouch);
        }

        if (SprintAction)
        {
            EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::StartSprint);
            EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopSprint);
        }
        
        if (NextItemAction) { EnhancedInput->BindAction(NextItemAction, ETriggerEvent::Triggered, this, &ThisClass::Input_SelectNext); }
        if (PrevItemAction) { EnhancedInput->BindAction(PrevItemAction, ETriggerEvent::Triggered, this, &ThisClass::Input_SelectPrev); }

        if (UseItemAction)  { EnhancedInput->BindAction(UseItemAction,  ETriggerEvent::Started, this, &ThisClass::Input_UseItem); }
        if (DropItemAction) { EnhancedInput->BindAction(DropItemAction, ETriggerEvent::Started, this, &ThisClass::Input_DropItem); }

        if (InteractAction)
        {
            EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::Input_Interact);
        }
    }
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    
    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        AddMovementInput(Forward, MovementVector.Y);
        AddMovementInput(Right, MovementVector.X);
    }
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxis = Value.Get<FVector2D>();
    AddControllerYawInput(LookAxis.X);
    AddControllerPitchInput(LookAxis.Y);
}

void APlayerCharacter::StartCrouch(const FInputActionValue& Value)
{
    Crouch();
}

void APlayerCharacter::StopCrouch(const FInputActionValue& Value)
{
    UnCrouch();
}

void APlayerCharacter::StartSprint(const FInputActionValue& Value)
{
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void APlayerCharacter::StopSprint(const FInputActionValue& Value)
{
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::Input_SelectNext()
{
    if (InventoryComponent) { InventoryComponent->SelectNext(); }
}
void APlayerCharacter::Input_SelectPrev()
{
    if (InventoryComponent) { InventoryComponent->SelectPrevious(); }
}
void APlayerCharacter::Input_UseItem()
{
    if (InventoryComponent) { InventoryComponent->UseSelected(); }
}
void APlayerCharacter::Input_DropItem()
{
    if (InventoryComponent) { InventoryComponent->DropSelected(true, 1); }
}

void APlayerCharacter::Input_Interact()
{
    UE_LOG(LogTemp, Warning, TEXT("Interact pressed"));
    if (!InteractorComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("InteractorComponent is null"));
        return;
    }
    InteractorComponent->TryInteract();
}
