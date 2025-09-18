#include "PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

// === Constructeur ===
APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// === Camera Boom ===
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true;

	// === Camera ===
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// === Mouvement ===
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// === Sprint / Crouch ===
	CurrentSprintStamina = MaxSprintStamina;
	bIsCrouchingHold = false;
	bIsSprinting = false;

	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->CrouchedHalfHeight = 44.f;

	TargetCameraZ = StandingCameraZ;
}

// === Début du jeu ===
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// === Enhanced Input ===
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (PlayerMappingContext)
			{
				Subsystem->AddMappingContext(PlayerMappingContext, 0);
			}
		}
	}
}

// === Tick ===
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CurrentLoc = FollowCamera->GetRelativeLocation();
	CurrentLoc.Z = FMath::FInterpTo(CurrentLoc.Z, TargetCameraZ, DeltaTime, 6.f);
	FollowCamera->SetRelativeLocation(CurrentLoc);

	if (bIsSprinting)
	{
		CurrentSprintStamina -= SprintStaminaDrainRate * DeltaTime;
		if (CurrentSprintStamina <= 0.f)
		{
			CurrentSprintStamina = 0.f;
			bIsSprinting = false;
			GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		}
	}
	else
	{
		if (CurrentSprintStamina < MaxSprintStamina)
		{
			CurrentSprintStamina += SprintStaminaRecoveryRate * DeltaTime;
			if (CurrentSprintStamina > MaxSprintStamina)
				CurrentSprintStamina = MaxSprintStamina;
		}
	}
}

// === Mouvement ===
void APlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (Controller != nullptr)
	{
		FRotator Rotation = Controller->GetControlRotation();
		FRotator YawRotation(0, Rotation.Yaw, 0);

		FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, MovementVector.X);
		AddMovementInput(Right, MovementVector.Y);
	}
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

// === Jump ===
void APlayerCharacter::StartJump() { Jump(); }
void APlayerCharacter::StopJump() { StopJumping(); }

// === Crouch ===
void APlayerCharacter::CrouchPressed()
{
	bIsCrouchingHold = true;
	Crouch();

	GetCharacterMovement()->MaxWalkSpeed = CrouchSpeed;

	TargetCameraZ = CrouchingCameraZ;
}

void APlayerCharacter::CrouchReleased()
{
	bIsCrouchingHold = false;
	UnCrouch();
	
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	TargetCameraZ = StandingCameraZ;
}

// === Sprint ===
void APlayerCharacter::SprintPressed()
{
	if (CurrentSprintStamina > 0.f)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * SprintSpeedMultiplier;
	}
}

void APlayerCharacter::SprintReleased()
{
	if (bIsSprinting)
	{
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

// === Input ===
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction) EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		if (LookAction) EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::StartJump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopJump);
		}
		
		if (CrouchAction)
		{
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::CrouchPressed);
			EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::CrouchReleased);
		}
		
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::SprintPressed);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::SprintReleased);
		}
	}
}
