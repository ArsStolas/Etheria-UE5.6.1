/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UPlayerSessionControlComponent" - Source
 */
#include "Core/Common/PlayerSessionControlComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UPlayerSessionControlComponent::UPlayerSessionControlComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerSessionControlComponent::BeginPlay()
{
    Super::BeginPlay();
}

APlayerController* UPlayerSessionControlComponent::ResolvePlayerController() const
{
    if (APlayerController* AsPC = Cast<APlayerController>(GetOwner()))
    {
        return AsPC;
    }

    if (APawn* AsPawn = Cast<APawn>(GetOwner()))
    {
        return Cast<APlayerController>(AsPawn->GetController());
    }

    // Fallback: local player 0
    if (UWorld* World = GetWorld())
    {
        return UGameplayStatics::GetPlayerController(World, 0);
    }

    return nullptr;
}

void UPlayerSessionControlComponent::ApplyInputMode(APlayerController* PC, ESessionInputMode Mode, bool bShowCursor)
{
    if (!PC)
    {
        return;
    }

    PC->bShowMouseCursor = bShowCursor;

    switch (Mode)
    {
        case ESessionInputMode::UIOnly:
        {
            FInputModeUIOnly InputMode;
            PC->SetInputMode(InputMode);
            break;
        }
        case ESessionInputMode::GameAndUI:
        {
            FInputModeGameAndUI InputMode;
            PC->SetInputMode(InputMode);
            break;
        }
        case ESessionInputMode::GameOnly:
        default:
        {
            FInputModeGameOnly InputMode;
            PC->SetInputMode(InputMode);
            break;
        }
    }
}

void UPlayerSessionControlComponent::RestoreInputMode(APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    // Best-effort restore.
    PC->bShowMouseCursor = bPrevShowCursor;

    FInputModeGameOnly InputMode;
    PC->SetInputMode(InputMode);
}

void UPlayerSessionControlComponent::ApplyPolicy(const FSessionControlPolicy& Policy)
{
    APlayerController* PC = ResolvePlayerController();
    if (!PC)
    {
        return;
    }

    if (!bPolicyApplied)
    {
        bPrevIgnoreMove = PC->IsMoveInputIgnored();
        bPrevIgnoreLook = PC->IsLookInputIgnored();
        bPrevShowCursor = PC->bShowMouseCursor;

        if (UWorld* World = GetWorld())
        {
            PrevTimeDilation = UGameplayStatics::GetGlobalTimeDilation(World);
        }
    }

    PC->SetIgnoreMoveInput(Policy.bLockMovement);
    PC->SetIgnoreLookInput(Policy.bLockCamera);

    bActionsLocked = Policy.bLockActions;

    if (Policy.bHideHUD)
    {
        // CinematicMode is a convenient way to hide HUD. We keep movement/turning controlled by IgnoreMove/IgnoreLook.
        PC->SetCinematicMode(true, false, true, false, false);
    }

    ApplyInputMode(PC, Policy.InputMode, Policy.bShowCursor);

    if (UWorld* World = GetWorld())
    {
        UGameplayStatics::SetGlobalTimeDilation(World, Policy.TimeDilation);
    }

    bPolicyApplied = true;
    BP_OnPolicyApplied(Policy);
}

void UPlayerSessionControlComponent::RestorePolicy()
{
    if (!bPolicyApplied)
    {
        return;
    }

    APlayerController* PC = ResolvePlayerController();
    if (!PC)
    {
        bPolicyApplied = false;
        bActionsLocked = false;
        return;
    }

    PC->SetIgnoreMoveInput(bPrevIgnoreMove);
    PC->SetIgnoreLookInput(bPrevIgnoreLook);
    bActionsLocked = false;

    // Restore HUD
    PC->SetCinematicMode(false, false, true, false, false);

    RestoreInputMode(PC);

    if (UWorld* World = GetWorld())
    {
        UGameplayStatics::SetGlobalTimeDilation(World, PrevTimeDilation);
    }

    bPolicyApplied = false;
    BP_OnPolicyRestored();
}
