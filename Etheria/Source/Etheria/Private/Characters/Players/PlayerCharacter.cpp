/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Mato
 * Class: PlayerCharacter - Source
 */

#include "Characters/Players/PlayerCharacter.h"

#include "CableComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "Components/Characters/HealthComponent.h"
#include "Components/Characters/Player/FlightModes/FlightComponent.h"
#include "Components/Characters/Player/Rope/RopeAttachComponent.h"
#include "Components/Characters/Player/Rope/RopeConstraintComponent.h"
#include "Components/Characters/Player/Rope/RopeDetectionComponent.h"
#include "Components/Characters/Player/Rope/RopeLengthControllerComponent.h"
#include "Components/Characters/Player/Rope/RopeLockComponent.h"
#include "Components/Characters/Player/Rope/Swinging/RopeSwingComponent.h"
#include "Components/Inventory/InventoryComponent.h"
#include "Components/Combat/CombatComponent.h"
#include "Components/Combat/LockTarget/LockTargetComponent.h"
#include "Components/Combat/LockTarget/LockVisualComponent.h"
#include "Core/System/EtheriaGameplayTags.h"
#include "Data/Weapons/WeaponData.h"
#include "World/Rope/RopeAttachPoint.h"
#include "Components/Characters/Player/Movements/Swim/SwimComponent.h"
#include "Components/Characters/Player/Rope/URopeCameraComponent.h"
#include "Components/Characters/Player/Rope/Pulling/RopePullComponent.h"

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

    // --- GLIDER COMPONENT ---
    FlightComponent = CreateDefaultSubobject<UFlightComponent>(TEXT("FlightComponent"));

    // --- INVENTORY COMPONENT ---
    InventoryComponent  = CreateDefaultSubobject<UInventoryComponent>(TEXT("BPC_Inventory"));

    // --- INTERACTION COMPONENT ---
    //InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("BPC_Interaction"));
    
    // --- LOCK COMPONENT ---
    LockTargetComponent = CreateDefaultSubobject<ULockTargetComponent>(TEXT("BPC_LockTarget"));
    LockVisualComponent = CreateDefaultSubobject<ULockVisualComponent>(TEXT("BPC_LockVisual"));

    // --- ROPE COMPONENTS ---
    RopeCableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("BPC_RopeCable"));

    RopeDetectionComponent = CreateDefaultSubobject<URopeDetectionComponent>(TEXT("BPC_RopeDetectionComponent"));
    RopeLockComponent = CreateDefaultSubobject<URopeLockComponent>(TEXT("BPC_RopeLockComponent"));
    RopeAttachComponent = CreateDefaultSubobject<URopeAttachComponent>(TEXT("BPC_RopeAttachComponent"));
    RopeConstraintComponent = CreateDefaultSubobject<URopeConstraintComponent>(TEXT("BPC_RopeConstraintComponent"));
    RopeSwingComponent = CreateDefaultSubobject<URopeSwingComponent>(TEXT("BPC_RopeSwingComponent"));
    RopeLengthControllerComponent = CreateDefaultSubobject<URopeLengthControllerComponent>(TEXT("BPC_RopeLengthController"));
    RopeCameraComponent = CreateDefaultSubobject<URopeCameraComponent>(TEXT("BPC_RopeCameraComponent"));
    RopePullComponent = CreateDefaultSubobject<URopePullComponent>(TEXT("BPC_RopePullComponent"));

    // --- SWIM COMPONENTS ---
    SwimComponent = CreateDefaultSubobject<USwimComponent>(TEXT("BPC_Swim"));
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    BaseArmLength = CameraBoom->TargetArmLength;
    BaseFOV = FollowCamera->FieldOfView;
    
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

    // --- Combat Delegates ---
    if (CombatComponent)
    {
        CombatComponent->OnAttackStarted.AddDynamic(this, &APlayerCharacter::HandleAttackStart);
        CombatComponent->OnAttackEnded  .AddDynamic(this, &APlayerCharacter::HandleAttackEnd);
    }

    // --- State Delegates ---
    if (StateComponent)
    {
        BIND_IF(bDebugMovementStateLogs, StateComponent->OnMovementStateChanged, LogMovementStateChanged);
        BIND_IF(bDebugCombatStateLogs, StateComponent->OnCombatStateChanged, LogCombatStateChanged);
        BIND_IF(bDebugLifeStateLogs, StateComponent->OnLifeStateChanged, LogLifeStateChanged);
    }

    // --- Health Delegates ---
    if (HealthComponent)
    {
        BIND_IF(bDebugLifeStateLogs, HealthComponent->OnHealthChanged, LogHealthChanged);
    }

    // --- Glider Delegates ---
    if (FlightComponent && StateComponent)
    {
        FlightComponent->OnGlideStart.AddDynamic(this, &APlayerCharacter::OnGlideStart);
        FlightComponent->OnGlideStop.AddDynamic(this, &APlayerCharacter::OnGlideStop);
        FlightComponent->OnDiveStart.AddDynamic(this, &APlayerCharacter::OnDiveStart);
        FlightComponent->OnDiveStop.AddDynamic(this, &APlayerCharacter::OnDiveStop);
    }
    
    if (RopeCableComponent)
    {
        RopeCableComponent->AttachToComponent(
            GetMesh(),
            FAttachmentTransformRules::SnapToTargetIncludingScale,
            RopeStartSocketName
        );
    }
}

void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    HandleMovementInput();
    UpdateMovementState();

    // Keep camera updated if player stays aiming (smooth interpolation)
    // if (bIsAiming && CombatComponent && CombatComponent->GetCurrentWeaponData())
    // {
    //     const FWeaponRangedConfig& Ranged = CombatComponent->GetCurrentWeaponData()->Ranged;
    //     CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, Ranged.AimArmLength, DeltaTime, 8.f);
    //     FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, Ranged.AimFOV, DeltaTime, 8.f));
    // }
    // else if (bIsAiming && CombatComponent && CombatComponent->GetCurrentWeaponData())
    // {
    //     // Restore default FOV gradually when not aiming
    //     CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, BaseArmLength, DeltaTime, 6.f);
    //     FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, BaseFOV, DeltaTime, 6.f));
    // }
}


void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
    #pragma region "CAMERA BINDS"
        // CAMERA BINDS
        EIC->BindAction(LookAction,  ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
    #pragma endregion
        
    #pragma region "MOVEMENTS BINDS"
        // MOVEMENTS BINDS
        EIC->BindAction(ForwardAction, ETriggerEvent::Started,   this, &APlayerCharacter::OnForwardStarted);
        EIC->BindAction(ForwardAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnForwardCompleted);
        EIC->BindAction(BackAction,    ETriggerEvent::Started,   this, &APlayerCharacter::OnBackStarted);
        EIC->BindAction(BackAction,    ETriggerEvent::Completed, this, &APlayerCharacter::OnBackCompleted);
        EIC->BindAction(LeftAction,    ETriggerEvent::Started,   this, &APlayerCharacter::OnLeftStarted);
        EIC->BindAction(LeftAction,    ETriggerEvent::Completed, this, &APlayerCharacter::OnLeftCompleted);
        EIC->BindAction(RightAction,   ETriggerEvent::Started,   this, &APlayerCharacter::OnRightStarted);
        EIC->BindAction(RightAction,   ETriggerEvent::Completed, this, &APlayerCharacter::OnRightCompleted);
        
        EIC->BindAction(CrouchAction, ETriggerEvent::Started,   this, &APlayerCharacter::OnCrouchPressed);
        EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopCrouch);
        EIC->BindAction(SprintAction, ETriggerEvent::Started,   this, &APlayerCharacter::StartSprint);
        EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopSprint);
        EIC->BindAction(JumpAction,   ETriggerEvent::Started,   this, &APlayerCharacter::OnJumpPressed);
    #pragma endregion

    #pragma region "FLIGHT MODE BINDS"
        // GLIDER x SWIM BINDS
        EIC->BindAction(GliderAction, ETriggerEvent::Started,   this, &APlayerCharacter::ToggleGlideMode);
        
        // --- DIVE (routes to Swim when in water, otherwise flight dive) ---
        UInputAction* EffectiveDiveAction = SwimDiveAction ? SwimDiveAction : DiveAction;
        if (EffectiveDiveAction)
        {
            EIC->BindAction(EffectiveDiveAction, ETriggerEvent::Started,   this, &APlayerCharacter::OnDiveInputPressed);
            EIC->BindAction(EffectiveDiveAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnDiveInputReleased);
        }
    #pragma endregion

    #pragma region "INVENTORY BINDS"
        // INVENTORY BINDS
        EIC->BindAction(NextItemAction, ETriggerEvent::Triggered, this, &ThisClass::Input_SelectNext);
        EIC->BindAction(PrevItemAction, ETriggerEvent::Triggered, this, &ThisClass::Input_SelectPrev);
        EIC->BindAction(UseItemAction,  ETriggerEvent::Started, this, &ThisClass::Input_UseItem);
        EIC->BindAction(DropItemAction, ETriggerEvent::Started, this, &ThisClass::Input_DropItem);
    #pragma endregion

    #pragma region "INTERACTION BINDS"
         // INTERACTION BIND 
        EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::Input_Interact);
    #pragma endregion

    #pragma region "COMBAT BINDS"
        // COMBAT BINDS
        if (AttackLightAction)
        {
            // Press: start or chain combo
            EIC->BindAction(AttackLightAction, ETriggerEvent::Started,   this, &APlayerCharacter::OnAttackLightPressed);
            // Release: can be used to trigger buffered advance as you prefer
            EIC->BindAction(AttackLightAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnAttackLightReleased);
        }

        if (AttackHeavyAction)
        {
            // Press: start a charge-capable attack (heavy id)
            EIC->BindAction(AttackHeavyAction, ETriggerEvent::Started,   this, &APlayerCharacter::OnAttackHeavyPressed);
            // Release: release the charge at current level
            EIC->BindAction(AttackHeavyAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnAttackHeavyReleased);
            // Canceled path if your input can be canceled
            EIC->BindAction(AttackHeavyAction, ETriggerEvent::Canceled,  this, &APlayerCharacter::OnAttackHeavyCanceled);
        }

        if (ParryAction)
        {
            EIC->BindAction(ParryAction, ETriggerEvent::Started,   this, &APlayerCharacter::OnParryPressed);
            EIC->BindAction(ParryAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnParryReleased);
        }

        if (DodgeAction)
        {
            EIC->BindAction(DodgeAction, ETriggerEvent::Started, this, &APlayerCharacter::OnDodgePressed);
        }
    #pragma endregion
        
    #pragma region "LOCK TARGET BINDS"
        if (LockToggleAction)
        {
            EIC->BindAction(LockToggleAction, ETriggerEvent::Started, this, &APlayerCharacter::OnLockToggle);
        }
        if (LockSwitchLeftAction)
        {
            EIC->BindAction(LockSwitchLeftAction, ETriggerEvent::Started, this, &APlayerCharacter::OnLockSwitchLeft);
        }
        if (LockSwitchRightAction)
        {
            EIC->BindAction(LockSwitchRightAction, ETriggerEvent::Started, this, &APlayerCharacter::OnLockSwitchRight);
        }
    #pragma endregion

    #pragma region "ROPE BINDS"
        if (AttachRopeAction)
        {
            EIC->BindAction(AttachRopeAction, ETriggerEvent::Started, this, &APlayerCharacter::OnRopeAttachPressed);
        }
        
        if (RopeLengthAction)
        {
            EIC->BindAction(RopeLengthAction, ETriggerEvent::Triggered, this, &APlayerCharacter::RopeLengthInput);
            EIC->BindAction(RopeLengthAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopRopeLengthInput);
        }
    #pragma endregion
        
    #pragma region "SWIM BINDS"
            // SWIM BIND 

    #pragma endregion
    }
}

#pragma region "SIMPLE ACCESSORS"

bool APlayerCharacter::IsGrounded() const
{
    FHitResult Hit;
    FVector Start = this->GetActorLocation();
    FVector End = Start - FVector(0.f, 0.f, 100.f);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    bool bHit = this->GetWorld()->LineTraceSingleByChannel(
        Hit, Start, End, ECC_Visibility, QueryParams
    );

    return bHit;
}

#pragma endregion

#pragma region AIMING

void APlayerCharacter::ToggleAiming(bool bEnable)
{
    // Allow disabling aim even if weapon data is missing (e.g., during weapon swap).
    if (!bEnable)
    {
        bIsAiming = false;

        // Restore camera targets (Tick already interpolates smoothly toward these).
        CameraBoom->TargetArmLength = BaseArmLength;
        FollowCamera->SetFieldOfView(BaseFOV);

        // Restore locomotion defaults.
        bUseControllerRotationYaw = false;
        if (UCharacterMovementComponent* Move = GetCharacterMovement())
        {
            Move->bOrientRotationToMovement = true;
            Move->MaxWalkSpeed = WalkSpeed;
        }

        UE_LOG(LogTemp, Verbose, TEXT("[Aiming] OFF"));
        return;
    }

    if (!CombatComponent) return;

    const UWeaponData* WeaponData = CombatComponent->GetCurrentWeaponData();
    if (!WeaponData) return;

    const FWeaponRangedConfig& Ranged = WeaponData->Ranged;

    // Only ranged weapons can aim.
    if (!Ranged.bIsRangedWeapon) return;

    bIsAiming = true;

    // Camera targets.
    CameraBoom->TargetArmLength = Ranged.AimArmLength;
    FollowCamera->SetFieldOfView(Ranged.AimFOV);

    // While aiming, rotate character with control rotation so ranged traces match the camera direction.
    bUseControllerRotationYaw = true;
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->bOrientRotationToMovement = false;
        Move->MaxWalkSpeed = WalkSpeed * Ranged.AimMoveSpeedMultiplier;
    }

    // Snap once so yaw immediately matches camera when entering aim.
    AlignToCamera();

    UE_LOG(LogTemp, Log, TEXT("[Aiming] ON | Arm=%.0f FOV=%.0f"), Ranged.AimArmLength, Ranged.AimFOV);
}


void APlayerCharacter::OnAimPressed()
{
    ToggleAiming(true);
}

void APlayerCharacter::OnAimReleased()
{
    ToggleAiming(false);
}

#pragma endregion

#pragma region "MOVEMENT INPUTS"

void APlayerCharacter::OnForwardStarted(const FInputActionValue&)   { Vertical.OnPosStarted(GetWorld()->GetTimeSeconds()); }
void APlayerCharacter::OnForwardCompleted(const FInputActionValue&){ Vertical.OnPosCompleted(); }
void APlayerCharacter::OnBackStarted(const FInputActionValue&)      { Vertical.OnNegStarted(GetWorld()->GetTimeSeconds()); }
void APlayerCharacter::OnBackCompleted(const FInputActionValue&)    { Vertical.OnNegCompleted(); }
void APlayerCharacter::OnLeftStarted(const FInputActionValue&)      { Horizontal.OnNegStarted(GetWorld()->GetTimeSeconds()); }
void APlayerCharacter::OnLeftCompleted(const FInputActionValue&)    { Horizontal.OnNegCompleted(); }
void APlayerCharacter::OnRightStarted(const FInputActionValue&)     { Horizontal.OnPosStarted(GetWorld()->GetTimeSeconds()); }
void APlayerCharacter::OnRightCompleted(const FInputActionValue&)   { Horizontal.OnPosCompleted(); }
    
void APlayerCharacter::StartSprint()
{
    if (!StateComponent) return;
    
    if (SwimComponent && SwimComponent->IsSwimmingActive())
    {
        SwimComponent->SetSwimSprinting(true);
        return;
    }
    
    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

    // --- Double-tap = Dodge ---
    if (!CombatComponent) return;

    const int Hor = Horizontal.GetAxisValue();
    const int Ver = Vertical.GetAxisValue();

    // Check direction
    if (Hor == 0 && Ver == 0)
    {
        return;
    }

    if (!Controller)
    {
        return;
    }

    const FRotator Rotation   = Controller->GetControlRotation();
    const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
    const FVector Forward     = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right       = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    const FVector WorldDir = (Forward * static_cast<float>(Ver) + Right * static_cast<float>(Hor)).GetSafeNormal();
    if (!WorldDir.IsNearlyZero())
    {
        // Check first or second tap
        CombatComponent->HandleDodgeInputTap(WorldDir);
    }
}

void APlayerCharacter::StopSprint()
{
    if (SwimComponent && SwimComponent->IsSwimmingActive())
    {
        SwimComponent->SetSwimSprinting(false);
        return;
    }
    
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::OnJumpPressed()
{
    if (!CombatComponent)
    {
        Jump();
        return;
    }

    if (CombatComponent->IsJumpBlocked()) return;
    if (SwimComponent && SwimComponent->IsSwimmingActive()) return;

    // ===============================
    // ROPE JUMP HANDLING
    // ===============================

    if (RopeSwingComponent && RopeSwingComponent->IsSwinging())
    {
        FVector LaunchVelocity;

        const bool bDidDetach =
            RopeSwingComponent->TryJumpOffRope(LaunchVelocity);

        if (bDidDetach)
        {
            // Fully detach rope
            if (RopeAttachComponent)
            {
                RopeAttachComponent->DetachRope();
            }

            LaunchCharacter(LaunchVelocity, true, true);

            if (StateComponent)
            {
                StateComponent->SetMovementState(
                    EtheriaTags::State_Movement_Airborne_Jumping
                );
            }

            return; // Do NOT call default Jump()
        }

        // Not enough speed → stay attached
        return;
    }

    // ===============================
    // NORMAL JUMP
    // ===============================

    if (CombatComponent->IsAttackActive())
    {
        bJumpBuffered = true;
        JumpBufferExpireAt = GetWorld()
            ? GetWorld()->GetTimeSeconds() + JumpBufferTime
            : 0.f;
        return;
    }

    Jump();

    if (StateComponent)
    {
        StateComponent->SetMovementState(
            EtheriaTags::State_Movement_Airborne_Jumping
        );
    }
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    if (UCharacterStateComponent* StateComp = GetStateComponent())
    {
        if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Rope) &&
            !StateComponent->IsInMovementState(EtheriaTags::State_Movement_Rope_Detached))
        {
            return;
        }
        StateComp->SetMovementState(EtheriaTags::State_Movement_Grounded_Idle);
    }
}

void APlayerCharacter::OnCrouchPressed()
{
    if (!CombatComponent) { Crouch(); return; }
    if (CombatComponent->IsCrouchBlocked()) return;
    if (CombatComponent->IsAttackActive()) return;

    Crouch();

    if (StateComponent)
    {
        if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Rope) &&
            !StateComponent->IsInMovementState(EtheriaTags::State_Movement_Rope_Detached))
        {
            return;
        }
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Grounded_Crouching);
    }
}

void APlayerCharacter::StopCrouch()
{
    UnCrouch();

    if (StateComponent)
    {
        if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Rope) &&
            !StateComponent->IsInMovementState(EtheriaTags::State_Movement_Rope_Detached))
        {
            return;
        }
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Grounded_Idle);
    }
}

#pragma endregion

#pragma region "MOVEMENT STATE HANDLERS"

void APlayerCharacter::HandleMovementInput()
{
    if (!Controller) return;
    
    if (RopeSwingComponent && RopeSwingComponent->IsSwinging())
    {
        UE_LOG(LogTemp, VeryVerbose, TEXT("Swinging - skipping normal movement input"));
        return;
    }

    const int Hor = Horizontal.GetAxisValue();
    const int Ver = Vertical.GetAxisValue();

    if (Hor == 0 && Ver == 0) return;

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);
    const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    if (Ver != 0) AddMovementInput(Forward, static_cast<float>(Ver));
    if (Hor != 0) AddMovementInput(Right, static_cast<float>(Hor));
}

void APlayerCharacter::UpdateMovementState()
{
    if (!StateComponent || !GetCharacterMovement()) return;

    UCharacterMovementComponent* MoveComp = GetCharacterMovement();

    if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Rope) &&
        !StateComponent->IsInMovementState(EtheriaTags::State_Movement_Rope_Detached))
    {
        return;
    }
    
    if (MoveComp->IsFalling())
        HandleAirborneState();
    else
        HandleGroundedState();
    
    // --- Swim overrides grounded/airborne state ---
    if (SwimComponent && SwimComponent->IsSwimmingActive())
    {
        if (SwimComponent->IsUnderwater())
        {
            StateComponent->SetMovementState(EtheriaTags::State_Movement_Swim_Underwater);
        }
        else
        {
            StateComponent->SetMovementState(EtheriaTags::State_Movement_Swim_Surface);
        }
        return;
    }
}

void APlayerCharacter::HandleGroundedState()
{

    if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Gliding) ||
    StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Diving))
    {
        return;
    }
    
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    const int Hor = Horizontal.GetAxisValue();
    const int Ver = Vertical.GetAxisValue();

    if (bIsCrouched)
    {
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Grounded_Crouching);
    }
    else if (MoveComp->MaxWalkSpeed == SprintSpeed && (Hor != 0 || Ver != 0))
    {
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Grounded_Sprinting);
    }
    else if (Hor != 0 || Ver != 0)
    {
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Grounded_Walking);
    }
    else
    {
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Grounded_Idle);
    }
}

void APlayerCharacter::HandleAirborneState()
{
    if (!StateComponent || !GetWorld()) return;

    if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Gliding) ||
        StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Diving))
        return;

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now < AirborneIgnoreUntil)
        return;

    const FVector Velocity = GetVelocity();
    const float JumpThresholdZ = 250.f;

    if (StateComponent->WasInMovementState(EtheriaTags::State_Movement_Airborne_Diving) ||
        StateComponent->WasInMovementState(EtheriaTags::State_Movement_Airborne_Gliding))
    {
        if (Velocity.Z > 0.f)
            return;
    }

    if (Velocity.Z > JumpThresholdZ)
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Airborne_Jumping);
    else
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Airborne_Falling);
}

#pragma endregion

#pragma region "CAMERA INPUTS"
void APlayerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxis = Value.Get<FVector2D>();
    if (Controller)
    {
        AddControllerYawInput(LookAxis.X);
        AddControllerPitchInput(LookAxis.Y);
    }
}
#pragma endregion

#pragma region "FLIGHT MODE INPUTS"

void APlayerCharacter::ToggleGlideMode()
{
    if (!FlightComponent || RopeAttachComponent->IsAttached()) return;

    if (FlightComponent->IsInMode(EFlightMode::Glide))
    {
        FlightComponent->StopMode();
    }
    else if (FlightComponent->IsInMode(EFlightMode::Dive))
    {
        FlightComponent->StopMode();
        FlightComponent->StartGlide();
    }
    else
    {
        FlightComponent->StartGlide();
    }
}

void APlayerCharacter::ToggleDiveMode()
{
    if (!FlightComponent || RopeAttachComponent->IsAttached()) return;

    if (FlightComponent->IsInMode(EFlightMode::Dive))
    {
        FlightComponent->StopMode();
    }
    else if (FlightComponent->IsInMode(EFlightMode::Glide))
    {
        FlightComponent->StartDive();
    } else
    {
        FlightComponent->StartDive();
    }
}

void APlayerCharacter::AlignToCamera()
{
    if (Controller)
    {
        FRotator CamRot = Controller->GetControlRotation();
        SetActorRotation(FRotator(0.f, CamRot.Yaw, 0.f));
    }
}

#pragma endregion

#pragma region "FLIGHT MODE HANDLERS"
void APlayerCharacter::OnGlideStart()
{
    if (StateComponent)
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Airborne_Gliding);
}

void APlayerCharacter::OnGlideStop()
{
    if (StateComponent)
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Airborne_Falling);
}

void APlayerCharacter::OnDiveStart()
{
    if (StateComponent)
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Airborne_Diving);
}

void APlayerCharacter::OnDiveStop()
{
    if (StateComponent)
        StateComponent->SetMovementState(EtheriaTags::State_Movement_Airborne_Falling);
}
#pragma endregion

#pragma region "COMBAT INPUTS"

void APlayerCharacter::OnAttackLightPressed()
{
    if (!CombatComponent || !StateComponent) return;

    if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Gliding) ||
        StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Diving))
        return;

    if (bIsAiming && CombatComponent->GetCurrentWeaponData() && CombatComponent->GetCurrentWeaponData()->Ranged.bIsRangedWeapon)
    {
        CombatComponent->StartRangedFire();
        return;
    }
    
    // Si une attaque est déjà active, on tente une avance de combo
    if (CombatComponent->IsAttackActive() || CombatComponent->IsInAttackWindow())
    {
        CombatComponent->RequestComboAdvance();
        return;
    }

    // Attaque légère selon le contexte (au sol ou en l’air)
    CombatComponent->TryAttackGroup(FName("Light"));
}

void APlayerCharacter::OnAttackLightReleased()
{
    if (bIsAiming && CombatComponent->GetCurrentWeaponData() && CombatComponent->GetCurrentWeaponData()->Ranged.bIsRangedWeapon)
    {
        CombatComponent->StopRangedFire();
        return;
    }
}

void APlayerCharacter::OnAttackHeavyPressed()
{
    if (!CombatComponent || !StateComponent) return;

    if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Gliding) ||
        StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Diving))
        return;

    if (CombatComponent->GetCurrentWeaponData() && CombatComponent->GetCurrentWeaponData()->Ranged.bIsRangedWeapon)
    {
        ToggleAiming(true);
        return;
    }
    if (CombatComponent && bIsAiming)
    {
        CombatComponent->StartRangedFire();
        return;
    }
    
    CombatComponent->RequestComboAdvance();
    if (!CombatComponent->IsAttackActive())
    {
        CombatComponent->TryAttackGroup(FName("Heavy"));
    }
}

void APlayerCharacter::OnAttackHeavyReleased()
{
    if (!CombatComponent) return;
    
    if (CombatComponent->GetCurrentWeaponData() && CombatComponent->GetCurrentWeaponData()->Ranged.bIsRangedWeapon)
    {
        ToggleAiming(false);
        return;
    }
    if (CombatComponent && bIsAiming)
    {
        CombatComponent->StopRangedFire();
        return;
    }
    
    CombatComponent->EndCharge(false);
}

void APlayerCharacter::OnAttackHeavyCanceled()
{
    if (!CombatComponent) return;
    CombatComponent->EndCharge(true);
}

void APlayerCharacter::OnParryPressed()
{
    if (!CombatComponent || !StateComponent) return;

    if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Gliding) ||
        StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Diving))
    {
        UE_LOG(LogTemp, Warning, TEXT("Can't parry while gliding or diving!"));
        return;
    }

    CombatComponent->SetParryHeld(true);
}

void APlayerCharacter::OnParryReleased()
{
    if (!CombatComponent) return;
    CombatComponent->SetParryHeld(false);
}

void APlayerCharacter::OnDodgePressed()
{
    if (!CombatComponent || !StateComponent) return;

    if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Gliding) ||
        StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Diving))
    {
        UE_LOG(LogTemp, Warning, TEXT("Can't dodge while gliding or diving!"));
        return;
    }

    const int Hor = Horizontal.GetAxisValue();
    const int Ver = Vertical.GetAxisValue();

    FVector WorldDir = FVector::ZeroVector;

    if (Controller && (Hor != 0 || Ver != 0))
    {
        const FRotator Rotation   = Controller->GetControlRotation();
        const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
        const FVector Forward     = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector Right       = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        WorldDir = (Forward * static_cast<float>(Ver) + Right * static_cast<float>(Hor)).GetSafeNormal();
    }

    if (!WorldDir.IsNearlyZero())
    {
        // Dodge with the movement direction
        CombatComponent->TryDodgeWorldDirection(WorldDir);
    }
    else
    {
        // No input = Backward dash
        if (Controller)
        {
            const FRotator Rotation   = Controller->GetControlRotation();
            const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
            const FVector Forward     = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

            CombatComponent->TryDodgeWorldDirection(-Forward);
        }
        else
        {
            CombatComponent->TryDodgeDirection(EDodgeDirection::Backward);
        }
    }
}

#pragma endregion

#pragma region "COMBAT HANDLERS"
void APlayerCharacter::HandleAttackStart(FName)
{
    bLockJumpCrouchFromCombat = true;
}

void APlayerCharacter::HandleAttackEnd(FName)
{
    bLockJumpCrouchFromCombat = false;

    if (bJumpBuffered && GetWorld())
    {
        const float Now = GetWorld()->GetTimeSeconds();
        if (Now <= JumpBufferExpireAt)
        {
            Jump();
        }
    }
    bJumpBuffered = false;
}
#pragma endregion

#pragma region "LOCK TARGET INPUTS"
void APlayerCharacter::OnLockToggle()
{
    if (LockTargetComponent)
    {
        LockTargetComponent->ToggleLock(nullptr);
    }
}

void APlayerCharacter::OnLockSwitchLeft()
{
    if (LockTargetComponent)
    {
        LockTargetComponent->SwitchTarget(false);
    }
}

void APlayerCharacter::OnLockSwitchRight()
{
    if (LockTargetComponent)
    {
        LockTargetComponent->SwitchTarget(true);
    }
}
#pragma endregion

#pragma region "INVENTORY INPUTS"

// INVENTORY INPUTS
void APlayerCharacter::Input_SelectNext() { if (InventoryComponent) { InventoryComponent->SelectNext(); } }
void APlayerCharacter::Input_SelectPrev() { if (InventoryComponent) { InventoryComponent->SelectPrevious(); } }
void APlayerCharacter::Input_UseItem() { if (InventoryComponent) { InventoryComponent->UseSelected(); } }
void APlayerCharacter::Input_DropItem() { if (InventoryComponent) { InventoryComponent->DropSelected(true, 1); } }

#pragma endregion

#pragma region "INTERACTION INPUT"

// INTERACTION INPUT
void APlayerCharacter::Input_Interact()
{
    //UE_LOG(LogTemp, Warning, TEXT("Interact pressed"));
    //if (!InteractionComponent)
    //{
    //    UE_LOG(LogTemp, Error, TEXT("InteractionComponent is null"));
    //    return;
    //}
    //InteractionComponent->Interact();
}

#pragma endregion

#pragma region "ROPE INPUTS"

void APlayerCharacter::OnRopeAttachPressed()
{
    if (!RopeLockComponent || !RopeSwingComponent || !RopeAttachComponent) return;

    if (RopeAttachComponent->IsAttached())
    {
        CheckRopeAttachMode();
        RopeLockComponent->Unlock();
        if (StateComponent)
        {
            StateComponent->SetMovementState(EtheriaTags::State_Movement_Rope_Detached);
        }
        return;
    }

    if (RopeLockComponent->TryLock())
    {
        if (StateComponent)
        {
            StateComponent->SetMovementState(EtheriaTags::State_Movement_Rope_Attached);
        }
    }
}

void APlayerCharacter::CheckRopeAttachMode()
{
    if (!RopeLockComponent || !RopeSwingComponent || !RopeAttachComponent) return;

    ARopeAttachPoint* LockedPoint = RopeLockComponent->GetLockedPoint();
    if (!LockedPoint) return;

    switch (LockedPoint->AttachType)
    {
        case ERopeAttachType::Swing:
            {
                RopeSwingComponent->StopSwing();
                break;
            }
        case ERopeAttachType::Pull:
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Point Pull, pas de swing"));
            break;
        default:
            GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("AttachType non géré"));
            break;
    }
}

void APlayerCharacter::RopeLengthInput(const FInputActionValue& Value)
{
    const float AxisVal = Value.Get<float>();

    if (!RopeAttachComponent || !RopeAttachComponent->IsAttached())
        return;

    ARopeAttachPoint* AttachPoint = RopeAttachComponent->GetAttachedPoint();
    if (!AttachPoint)
        return;

    if (RopeLengthControllerComponent)
        RopeLengthControllerComponent->SetRopeLengthInput(AxisVal);

    switch (AttachPoint->GetAttachType())
    {
        case ERopeAttachType::Swing:
            if (StateComponent)
                StateComponent->SetMovementState(EtheriaTags::State_Movement_Rope_Climbing);
            break;

        case ERopeAttachType::Pull:
            if (StateComponent)
                StateComponent->SetMovementState(EtheriaTags::State_Movement_Rope_Pulling);
            break;

        default: break;
    }
}

void APlayerCharacter::StopRopeLengthInput(const FInputActionValue& Value)
{
    if (RopeLengthControllerComponent)
        RopeLengthControllerComponent->SetRopeLengthInput(0.f);
}

#pragma endregion

#pragma region "SWIM INPUTS"
void APlayerCharacter::OnDiveInputPressed()
{
    // If we're in water (or currently swimming), this input is for Swim
    if (SwimComponent && (SwimComponent->IsInWater() || SwimComponent->IsSwimmingActive()))
    {
        SwimComponent->Input_DivePressed();
        return;
    }

    // Otherwise keep existing flight behavior
    ToggleDiveMode();
}

void APlayerCharacter::OnDiveInputReleased()
{
    if (SwimComponent && (SwimComponent->IsInWater() || SwimComponent->IsSwimmingActive()))
    {
        SwimComponent->Input_DiveReleased();
    }
}
#pragma endregion

#pragma region "STATE LOGS"

void APlayerCharacter::LogMovementStateChanged(FGameplayTag Previous, FGameplayTag New)
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] MovementState changed: %s -> %s"),
           *GetName(), *Previous.ToString(), *New.ToString());
}

void APlayerCharacter::LogCombatStateChanged(FGameplayTag Previous, FGameplayTag New)
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] CombatState changed: %s -> %s"),
           *GetName(), *Previous.ToString(), *New.ToString());
}

void APlayerCharacter::LogLifeStateChanged(FGameplayTag Previous, FGameplayTag New)
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] LifeState changed: %s -> %s"),
           *GetName(), *Previous.ToString(), *New.ToString());
}

void APlayerCharacter::LogHealthChanged(float NewHealth, float MaxHealth)
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] Health changed: %f / %f"), *GetName(), NewHealth, MaxHealth);
}

#pragma endregion

#pragma region "COMMANDS EXEC"

void APlayerCharacter::DamageSelf(float Amount)
{
    if (HealthComponent)
    {
        HealthComponent->TakeDamage(FMath::Max(0.f, Amount));
    }
}

void APlayerCharacter::HealSelf(float Amount)
{
    if (HealthComponent)
    {
        HealthComponent->Heal(FMath::Max(0.f, Amount));
    }
}

void APlayerCharacter::SetHPPercent(float Percent)
{
    if (HealthComponent)
    {
        const float P = FMath::Clamp(Percent, 0.f, 1.f);
        const float Target = HealthComponent->GetMaxHealth() * P;
        const float Current = HealthComponent->GetHealth();
        const float Delta = Target - Current;

        if (Delta > 0.f) HealthComponent->Heal(Delta);
        else HealthComponent->TakeDamage(-Delta);
    }
}

#pragma endregion
