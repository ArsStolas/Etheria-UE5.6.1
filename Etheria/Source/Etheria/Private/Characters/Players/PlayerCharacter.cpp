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
#include "Components/Characters/CharacterStateComponent.h"
#include "Components/Characters/HealthComponent.h"
#include "Components/Characters/Player/Glider/GliderComponent.h"
#include "Components/Interaction/InteractorComponent.h"
#include "Components/Inventory/InventoryComponent.h"
#include "Components/Combat/CombatComponent.h"
#include "Components/Combat/LockTargetComponent.h"
#include "Core/System/EtheriaGameplayTags.h"

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
    GliderComponent = CreateDefaultSubobject<UGliderComponent>(TEXT("GliderComponent"));
    GliderVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GliderVisual"));
    GliderVisual->SetupAttachment(RootComponent);

    // --- INVENTORY COMPONENT ---
    InventoryComponent  = CreateDefaultSubobject<UInventoryComponent>(TEXT("BPC_Inventory"));

    // --- INTERACTOR COMPONENT ---
    InteractorComponent = CreateDefaultSubobject<UInteractorComponent>(TEXT("BPC_Interactor"));
    
    // --- LOCK COMPONENT ---
    LockTargetComponent = CreateDefaultSubobject<ULockTargetComponent>(TEXT("BPC_LockTarget"));
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

    // --- Combat Delegates ---
    if (CombatComponent)
    {
        CombatComponent->OnAttackStarted.AddDynamic(this, &APlayerCharacter::HandleAttackStart);
        CombatComponent->OnAttackEnded  .AddDynamic(this, &APlayerCharacter::HandleAttackEnd);
    }

    // --- State Delegates ---
    if (StateComponent)
    {
        StateComponent->OnMovementStateChanged.AddDynamic(this, &APlayerCharacter::LogMovementStateChanged);
        StateComponent->OnCombatStateChanged.AddDynamic(this, &APlayerCharacter::LogCombatStateChanged);
        StateComponent->OnLifeStateChanged.AddDynamic(this, &APlayerCharacter::LogLifeStateChanged);
    }

    // --- Health Delegates ---
    if (HealthComponent)
    {
        HealthComponent->OnHealthChanged.AddDynamic(this, &APlayerCharacter::LogHealthChanged);
        HealthComponent->OnDeath.AddDynamic(this, &APlayerCharacter::LogDeath);
    }

    // --- Glider Delegates ---
    if (GliderComponent && StateComponent)
    {
        GliderComponent->OnGlideStart.AddDynamic(this, &APlayerCharacter::OnGlideStart);
        GliderComponent->OnGlideStop.AddDynamic(this, &APlayerCharacter::OnGlideStop);
        GliderComponent->OnDiveStart.AddDynamic(this, &APlayerCharacter::OnDiveStart);
        GliderComponent->OnDiveStop.AddDynamic(this, &APlayerCharacter::OnDiveStop);
    }
}

void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    HandleMovementInput();
    UpdateMovementState();
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
        EIC->BindAction(LeftAction,  ETriggerEvent::Started,   this, &APlayerCharacter::OnLeftStarted);
        EIC->BindAction(LeftAction,  ETriggerEvent::Completed, this, &APlayerCharacter::OnLeftCompleted);
        EIC->BindAction(RightAction, ETriggerEvent::Started,   this, &APlayerCharacter::OnRightStarted);
        EIC->BindAction(RightAction, ETriggerEvent::Completed, this, &APlayerCharacter::OnRightCompleted);
        
        EIC->BindAction(CrouchAction, ETriggerEvent::Started,   this, &APlayerCharacter::OnCrouchPressed);
        EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopCrouch);
        EIC->BindAction(SprintAction, ETriggerEvent::Started,   this, &APlayerCharacter::StartSprint);
        EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopSprint);
        EIC->BindAction(JumpAction,   ETriggerEvent::Started,   this, &APlayerCharacter::OnJumpPressed);
    #pragma endregion

    #pragma region "GLIDER BINDS"
        // GLIDER BINDS
        EIC->BindAction(GliderAction, ETriggerEvent::Started,   this, &APlayerCharacter::ToggleGlideMode);
        EIC->BindAction(DiveAction,   ETriggerEvent::Started,   this, &APlayerCharacter::ToggleDiveMode);
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
    }
}

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

    GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void APlayerCharacter::StopSprint()
{
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::OnJumpPressed()
{
    if (!CombatComponent) { Jump(); return; }
    if (CombatComponent->IsJumpBlocked()) return;

    if (CombatComponent->IsAttackActive())
    {
        bJumpBuffered = true;
        JumpBufferExpireAt = GetWorld() ? GetWorld()->GetTimeSeconds() + JumpBufferTime : 0.f;
        return;
    }

    Jump();

    if (UCharacterStateComponent* StateComp = GetStateComponent())
    {
        StateComp->SetMovementState(EtheriaTags::State_Movement_Airborne_Jumping);
    }
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    if (UCharacterStateComponent* StateComp = GetStateComponent())
    {
        StateComp->SetMovementState(EtheriaTags::State_Movement_Grounded_Idle);
    }
}

void APlayerCharacter::OnCrouchPressed()
{
    if (!CombatComponent) { Crouch(); return; }
    if (CombatComponent->IsCrouchBlocked()) return;
    if (CombatComponent->IsAttackActive()) return;

    Crouch();

    if (UCharacterStateComponent* StateComp = GetStateComponent())
    {
        StateComp->SetMovementState(EtheriaTags::State_Movement_Grounded_Crouching);
    }
}

void APlayerCharacter::StopCrouch()
{
    UnCrouch();

    if (UCharacterStateComponent* StateComp = GetStateComponent())
    {
        StateComp->SetMovementState(EtheriaTags::State_Movement_Grounded_Idle);
    }
}

#pragma endregion

#pragma region "MOVEMENT STATE HANDLERS"

void APlayerCharacter::HandleMovementInput()
{
    if (!Controller) return;

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

    if (MoveComp->IsFalling())
        HandleAirborneState();
    else
        HandleGroundedState();
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
    if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Gliding) ||
        StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Diving))
        return;

    const FVector Velocity = GetVelocity();
    if (Velocity.Z > 0.f)
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

#pragma region "GLIDER INPUTS"

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

#pragma endregion

#pragma region "GLIDER HANDLERS"
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
    {
        UE_LOG(LogTemp, Warning, TEXT("Can't attack while gliding or diving!"));
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
}

void APlayerCharacter::OnAttackHeavyPressed()
{
    if (!CombatComponent || !StateComponent) return;

    if (StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Gliding) ||
        StateComponent->IsInMovementState(EtheriaTags::State_Movement_Airborne_Diving))
    {
        UE_LOG(LogTemp, Warning, TEXT("Can't attack while gliding or diving!"));
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

    CombatComponent->StartDodgeIFrames(-1.f);
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
    UE_LOG(LogTemp, Warning, TEXT("Interact pressed"));
    if (!InteractorComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("InteractorComponent is null"));
        return;
    }
    InteractorComponent->TryInteract();
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

void APlayerCharacter::LogDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] Player DIED!"), *GetName());
    if (StateComponent)
    {
        StateComponent->SetLifeState(EtheriaTags::State_Life_Dead);
    }
}

#pragma endregion
