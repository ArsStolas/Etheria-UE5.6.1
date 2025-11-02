/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: PlayerCharacter - Source
 */

#include "Characters/Players/PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/Characters/Player/Glider/GliderComponent.h"

APlayerCharacter::APlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // --- CAMERA SETUP ---
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 300.f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // --- CHARACTER ROTATION ---
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;

    // --- GLIDER ---
    GliderComponent = CreateDefaultSubobject<UGliderComponent>(TEXT("GliderComponent"));
    GliderVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GliderVisual"));
    GliderVisual->SetupAttachment(RootComponent);
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (PlayerContext)
            {
                Subsystem->AddMappingContext(PlayerContext, 0);
            }
        }
    }

    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    int Hor = Horizontal.GetAxisValue();
    int Ver = Vertical.GetAxisValue();

    if (Controller)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector Right   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        if (Ver != 0) AddMovementInput(Forward, static_cast<float>(Ver));
        if (Hor != 0) AddMovementInput(Right, static_cast<float>(Hor));
    }
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Mouvement 4 touches
        EIC->BindAction(ForwardAction, ETriggerEvent::Started,   this, &APlayerCharacter::OnForwardStarted);
        EIC->BindAction(ForwardAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnForwardCompleted);
        EIC->BindAction(BackAction,    ETriggerEvent::Started,   this, &APlayerCharacter::OnBackStarted);
        EIC->BindAction(BackAction,    ETriggerEvent::Completed, this, &APlayerCharacter::OnBackCompleted);
        EIC->BindAction(LeftAction,    ETriggerEvent::Started,   this, &APlayerCharacter::OnLeftStarted);
        EIC->BindAction(LeftAction,  ETriggerEvent::Started,   this, &APlayerCharacter::OnLeftStarted);
        EIC->BindAction(LeftAction,  ETriggerEvent::Completed, this, &APlayerCharacter::OnLeftCompleted);
        EIC->BindAction(RightAction, ETriggerEvent::Started,   this, &APlayerCharacter::OnRightStarted);
        EIC->BindAction(RightAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnRightCompleted);

        // Autres actions
        EIC->BindAction(LookAction,  ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
        EIC->BindAction(JumpAction,  ETriggerEvent::Triggered, this, &ACharacter::Jump);
        EIC->BindAction(CrouchAction, ETriggerEvent::Started,   this, &APlayerCharacter::StartCrouch);
        EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopCrouch);
        EIC->BindAction(SprintAction, ETriggerEvent::Started,   this, &APlayerCharacter::StartSprint);
        EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopSprint);
        EIC->BindAction(GliderAction, ETriggerEvent::Started,   this, &APlayerCharacter::ToggleGlideMode);
        EIC->BindAction(DiveAction,   ETriggerEvent::Started,   this, &APlayerCharacter::ToggleDiveMode);
    }
}

void APlayerCharacter::OnForwardStarted(const FInputActionValue&)   { Vertical.OnPosStarted(GetWorld()->GetTimeSeconds()); }
void APlayerCharacter::OnForwardCompleted(const FInputActionValue&){ Vertical.OnPosCompleted(); }
void APlayerCharacter::OnBackStarted(const FInputActionValue&)      { Vertical.OnNegStarted(GetWorld()->GetTimeSeconds()); }
void APlayerCharacter::OnBackCompleted(const FInputActionValue&)    { Vertical.OnNegCompleted(); }
void APlayerCharacter::OnLeftStarted(const FInputActionValue&)      { Horizontal.OnNegStarted(GetWorld()->GetTimeSeconds()); }
void APlayerCharacter::OnLeftCompleted(const FInputActionValue&)    { Horizontal.OnNegCompleted(); }
void APlayerCharacter::OnRightStarted(const FInputActionValue&)     { Horizontal.OnPosStarted(GetWorld()->GetTimeSeconds()); }
void APlayerCharacter::OnRightCompleted(const FInputActionValue&)   { Horizontal.OnPosCompleted(); }

void APlayerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxis = Value.Get<FVector2D>();
    if (Controller)
    {
        AddControllerYawInput(LookAxis.X);
        AddControllerPitchInput(LookAxis.Y);
    }
}

void APlayerCharacter::StartSprint() { GetCharacterMovement()->MaxWalkSpeed = SprintSpeed; }
void APlayerCharacter::StopSprint()  { GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; }
void APlayerCharacter::StartCrouch() { Crouch(); }
void APlayerCharacter::StopCrouch()  { UnCrouch(); }

void APlayerCharacter::ToggleGlideMode()
{
    if (GliderComponent) GliderComponent->ToggleGliding();
}
void APlayerCharacter::ToggleDiveMode()
{
    if (GliderComponent) GliderComponent->ToggleDiving();
}

void APlayerCharacter::AlignToCamera()
{
    if (Controller)
    {
        FRotator CamRot = Controller->GetControlRotation();
        SetActorRotation(FRotator(0.f, CamRot.Yaw, 0.f));
    }
}

bool APlayerCharacter::IsInSpecialMode() const
{
    return (GliderComponent && GliderComponent->IsGliding());
}