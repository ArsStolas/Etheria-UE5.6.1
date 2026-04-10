// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Interaction/OrionInteractionFunctionLibrary.h"
#include "Interaction/OrionInteractionComponent.h"
#include "OrionRPG.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Pawn.h"


UOrionInteractionComponent* UOrionInteractionFunctionLibrary::GetInteractionComponent(const UObject* WorldContextObject)
{
    return GetInteractionComponentFromTarget(UGameplayStatics::GetPlayerController(WorldContextObject, 0));
}

UOrionInteractionComponent* UOrionInteractionFunctionLibrary::GetInteractionComponentFromTarget(AActor* Target)
{
    if (!Target)
    {
        return nullptr;
    }

    if (UOrionInteractionComponent* InteractionComp = Target->FindComponentByClass<UOrionInteractionComponent>())
    {
        return InteractionComp;
    }

    // Try to get from PlayerState if available
    if (APlayerController* OwningController = Cast<APlayerController>(Target))
    {
        if (OwningController->GetPawn())
        {
            if (UOrionInteractionComponent* PawnComp = OwningController->GetPawn()->FindComponentByClass<UOrionInteractionComponent>())
            {
                return PawnComp;
            }
        }
        if (OwningController->PlayerState)
        {
            if (UOrionInteractionComponent* StateComp = OwningController->PlayerState->FindComponentByClass<UOrionInteractionComponent>())
            {
                return StateComp;
            }
        }
    }

    if (APawn* OwningPawn = Cast<APawn>(Target))
    {
        if (OwningPawn->GetController())
        {
            if (UOrionInteractionComponent* ControllerComp = OwningPawn->GetController()->FindComponentByClass<UOrionInteractionComponent>())
            {
                return ControllerComp;
            }
        }
        if (OwningPawn->GetPlayerState())
        {
            if (UOrionInteractionComponent* StateComp = OwningPawn->GetPlayerState()->FindComponentByClass<UOrionInteractionComponent>())
            {
                return StateComp;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("OrionInteractionFunctionLibrary : Current Component Not Found!"));
    return nullptr;
}
