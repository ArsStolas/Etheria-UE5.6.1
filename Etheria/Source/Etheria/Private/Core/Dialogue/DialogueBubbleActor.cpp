/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ADialogueBubbleActor" - Source
 */
#include "Core/Dialogue/DialogueBubbleActor.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Core/Dialogue/UI/DialogueBubbleWidgetInterface.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

ADialogueBubbleActor::ADialogueBubbleActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
    WidgetComponent->SetupAttachment(Root);
    WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    WidgetComponent->SetDrawAtDesiredSize(true);
    WidgetComponent->SetTwoSided(true);
    WidgetComponent->SetTickWhenOffscreen(false);

    SetActorEnableCollision(false);
}

void ADialogueBubbleActor::BeginPlay()
{
    Super::BeginPlay();
    Deactivate();
}

void ADialogueBubbleActor::ActivateForSpeaker(AActor* InSpeaker, TSubclassOf<UUserWidget> InWidgetClass, const FVector& InRelativeOffset)
{
    Speaker = InSpeaker;
    bIsActive = (InSpeaker != nullptr);

    if (!bIsActive)
    {
        Deactivate();
        return;
    }

    if (InWidgetClass)
    {
        WidgetComponent->SetWidgetClass(InWidgetClass);
        WidgetComponent->InitWidget();
    }

    AttachToActor(InSpeaker, FAttachmentTransformRules::KeepRelativeTransform);
    SetActorRelativeLocation(InRelativeOffset);
    SetActorHiddenInGame(false);
    SetActorTickEnabled(bBillboardToCamera);
}

void ADialogueBubbleActor::Deactivate()
{
    bIsActive = false;
    Speaker.Reset();

    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);

    // Keep widget allocated to support pooling.
}

void ADialogueBubbleActor::SetText(const FText& InText)
{
    if (!WidgetComponent)
    {
        return;
    }

    UUserWidget* Widget = WidgetComponent->GetUserWidgetObject();
    if (!Widget)
    {
        return;
    }

    if (Widget->GetClass()->ImplementsInterface(UDialogueBubbleWidgetInterface::StaticClass()))
    {
        IDialogueBubbleWidgetInterface::Execute_SetBubbleText(Widget, InText);
        IDialogueBubbleWidgetInterface::Execute_SetSpeaker(Widget, Speaker.Get());
    }
}

void ADialogueBubbleActor::SetBillboardEnabled(bool bEnabled)
{
    bBillboardToCamera = bEnabled;
    SetActorTickEnabled(bIsActive && bBillboardToCamera);
}

void ADialogueBubbleActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bIsActive || !bBillboardToCamera)
    {
        return;
    }

    UpdateBillboard(DeltaSeconds);
}

void ADialogueBubbleActor::UpdateBillboard(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC || !PC->PlayerCameraManager)
    {
        return;
    }

    const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
    const FVector ToCam = (CamLoc - GetActorLocation()).GetSafeNormal();
    if (ToCam.IsNearlyZero())
    {
        return;
    }

    const FRotator TargetRot = ToCam.Rotation();
    const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, BillboardInterpSpeed);
    SetActorRotation(NewRot);
}
