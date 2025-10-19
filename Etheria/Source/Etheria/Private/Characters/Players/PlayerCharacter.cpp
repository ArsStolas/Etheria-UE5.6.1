/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: PlayerCharacter - Source
*/

#include "Characters/Players/PlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Camera/CameraComponent.h"
#include "Components/Characters/Player/Glider/GliderComponent.h"
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

    GliderVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GliderVisual"));
    GliderVisual->SetupAttachment(RootComponent);
    GliderVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    GliderComponent = CreateDefaultSubobject<UGliderComponent>(TEXT("GliderComponent"));
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    GliderVisual->SetVisibility(false);

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

    if (GEngine && GetCharacterMovement())
    {
        float VerticalSpeed = GetCharacterMovement()->Velocity.Z;
        FString VerticalState = (VerticalSpeed > 50.f) ? TEXT("↑ MONTÉE") 
                               : (VerticalSpeed < -50.f) ? TEXT("↓ DESCENTE") 
                               : TEXT("→ STABLE");

        FString Msg = FString::Printf(
            TEXT("[PLAYER] Alt: %.0f | VZ: %.0f %s | Speed: %.0f"), 
            GetActorLocation().Z,
            VerticalSpeed,
            *VerticalState,
            GetCharacterMovement()->Velocity.Size()
        );

        FColor DebugColor = FColor::White;
        if (VerticalSpeed > 50.f) DebugColor = FColor::Green;
        else if (VerticalSpeed < -50.f) DebugColor = FColor::Red;

        GEngine->AddOnScreenDebugMessage(0, 0.f, DebugColor, Msg);
    }
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

        if (GliderAction)
        {
            EnhancedInput->BindAction(GliderAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleGlideMode);
        }

        if (DiveAction)
        {
            EnhancedInput->BindAction(DiveAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleDiveMode);
        }

    }
}

UStaticMeshComponent* APlayerCharacter::GetGliderVisual() const
{
    return GliderVisual;
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
    MoveInput = Value.Get<FVector2D>();
    
    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(Forward, MoveInput.X);
        AddMovementInput(Right, MoveInput.Y);
    }
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxis = Value.Get<FVector2D>();
    AddControllerYawInput(LookAxis.X);
    AddControllerPitchInput(LookAxis.Y);
}

void APlayerCharacter::StartCrouch()
{
    Crouch();
}

void APlayerCharacter::StopCrouch()
{
    UnCrouch();
}

void APlayerCharacter::StartSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void APlayerCharacter::StopSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::ToggleGlideMode()
{
    if (GliderComponent)
    {
        GliderComponent->ToggleGliding();
    }
}

void APlayerCharacter::ToggleDiveMode()
{
    if (GliderComponent)
    {
        GliderComponent->ToggleDiving();
    }
}

