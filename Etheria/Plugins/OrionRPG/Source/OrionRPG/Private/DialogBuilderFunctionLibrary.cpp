// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "DialogBuilderFunctionLibrary.h"
#include "OrionRPG.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "DialogBuilderGraph.h"
#include "DialogBuilderNode.h"
#include "DialogBuilderNode_DialogLine.h"
#include "DialogBuilderNode_RerouteNode.h"
#include "DialogBuilderNode_Root.h"
#include "DialogData.h"
#include "DialogComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AssetRegistry/AssetRegistryModule.h"


UDialogComponent* UDialogBuilderFunctionLibrary::GetDialogComponent(const UObject* WorldContextObject)
{
    return GetDialogComponentFromTarget(UGameplayStatics::GetPlayerController(WorldContextObject, 0));
}

UDialogComponent* UDialogBuilderFunctionLibrary::GetDialogComponentFromTarget(AActor* Target)
{
    if (!Target)
    {
        return nullptr;
    }

    if (UDialogComponent* DialogComp = Target->FindComponentByClass<UDialogComponent>())
    {
        return DialogComp;
    }

    //Dialog comp may be on the controllers pawn or pawns controller
    if (APlayerController* OwningController = Cast<APlayerController>(Target))
    {
        if (OwningController->GetPawn())
        {
            return OwningController->GetPawn()->FindComponentByClass<UDialogComponent>();
        }
    }

    if (APawn* OwningPawn = Cast<APawn>(Target))
    {
        if (OwningPawn->GetController())
        {
            return OwningPawn->GetController()->FindComponentByClass<UDialogComponent>();
        }
    }

    return nullptr;
}

UDialogBuilderGraph* UDialogBuilderFunctionLibrary::GetCurrentActiveDialog(const UObject* WorldContextObject)
{
    if (UDialogComponent* DialogComp = GetDialogComponentFromTarget(UGameplayStatics::GetPlayerController(WorldContextObject, 0)))
    {
        return DialogComp->CurrentActiveDialog;
    }

    return nullptr;
}

UDialogBuilderNode* UDialogBuilderFunctionLibrary::GetCurrentDialogNode(const UObject* WorldContextObject)
{
    UDialogBuilderGraph* CurrentDialogGraph = GetCurrentActiveDialog(WorldContextObject);
    if (CurrentDialogGraph && CurrentDialogGraph->CurrentNode)
    {
        return CurrentDialogGraph->CurrentNode;
    }

    return nullptr;
}

UDialogBuilderNode_DialogLine* UDialogBuilderFunctionLibrary::GetCurrentLine(const UObject* WorldContextObject)
{
    UDialogBuilderGraph* CurrentDialogGraph = GetCurrentActiveDialog(WorldContextObject);
    if (CurrentDialogGraph && CurrentDialogGraph->CurrentNode)
    {
        return CurrentDialogGraph->CurrentLine;
    }

    return nullptr;
}

void UDialogBuilderFunctionLibrary::AdvanceDialogLine(const UObject* WorldContextObject)
{
    UDialogBuilderGraph* CurrentDialog = GetCurrentActiveDialog(WorldContextObject);
    if (CurrentDialog)
    {
        CurrentDialog->AdvanceDialogLine();
    }
}

void UDialogBuilderFunctionLibrary::SelectDialogChoice(const UObject* WorldContextObject, UDialogBuilderNode_PlayerChoice* InOption)
{
    if (UDialogComponent* DialogComp = GetDialogComponentFromTarget(UGameplayStatics::GetPlayerController(WorldContextObject, 0)))
    {
        DialogComp->SelectDialogChoice(InOption);
    }

}

void UDialogBuilderFunctionLibrary::BeginDialog(const UObject* WorldContextObject, UDialogBuilderGraph* DialogAsset)
{
    if (UDialogComponent* DialogComp = GetDialogComponentFromTarget(UGameplayStatics::GetPlayerController(WorldContextObject, 0)))
    {
        DialogComp->BeginDialog(DialogAsset);
    }
}
