/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GliderComponent - Source
*/

#include "Components/Characters/Player/GliderComponent.h"

#include "Characters/Players/PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"

UGliderComponent::UGliderComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UGliderComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
}

void UGliderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!OwnerCharacter || !bIsGliderActive) return;

    HandleDescent(DeltaTime);

    if (OwnerCharacter->GetCharacterMovement()->IsWalking())
    {
        StopGliding();
    }
}

void UGliderComponent::ToggleGliding()
{
    if (bIsGliderActive)
    {
        StopGliding();
    }
    else
    {
        StartGliding();
    }
}

void UGliderComponent::StartGliding()
{
    if (!OwnerCharacter || !CanStartGliding()) return;

    bIsGliderActive = true;
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Glider Activated"));

    RecordOriginalSettings();

    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    MoveComp->RotationRate = FRotator(0.f, 250.f, 0.f);
    MoveComp->GravityScale = 0.0f;
    MoveComp->AirControl = 0.9f;
    MoveComp->BrakingDecelerationFalling = 350.f;
    MoveComp->MaxAcceleration = 1024.f;
    MoveComp->MaxWalkSpeed = 640.f;
    MoveComp->bUseControllerDesiredRotation = true;
}

void UGliderComponent::StopGliding()
{
    if (!OwnerCharacter) return;

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Glider Deactivated"));
    ApplyOriginalSettings();
    bIsGliderActive = false;
}

bool UGliderComponent::CanStartGliding() const
{
    if (!OwnerCharacter) return false;

    FHitResult Hit;
    FVector TraceStart = OwnerCharacter->GetActorLocation();
    FVector TraceEnd = TraceStart - OwnerCharacter->GetActorUpVector() * MinimumHeight;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);

    bool bHit = OwnerCharacter->GetWorld()->LineTraceSingleByChannel(
        Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams
    );

    return (!bHit && OwnerCharacter->GetCharacterMovement()->IsFalling());
}

void UGliderComponent::RecordOriginalSettings()
{
    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    OriginalOrientRotation = MoveComp->bOrientRotationToMovement;
    OriginalGravityScale = MoveComp->GravityScale;
    OriginalAirControl = MoveComp->AirControl;
    OriginalWalkingSpeed = MoveComp->MaxWalkSpeed;
    OriginalDeceleration = MoveComp->BrakingDecelerationFalling;
    OriginalAcceleration = MoveComp->MaxAcceleration;
    OriginalDesiredRotation = MoveComp->bUseControllerDesiredRotation;
}

void UGliderComponent::ApplyOriginalSettings()
{
    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    MoveComp->bOrientRotationToMovement = OriginalOrientRotation;
    MoveComp->GravityScale = OriginalGravityScale;
    MoveComp->AirControl = OriginalAirControl;
    MoveComp->MaxWalkSpeed = OriginalWalkingSpeed;
    MoveComp->BrakingDecelerationFalling = OriginalDeceleration;
    MoveComp->MaxAcceleration = OriginalAcceleration;
    MoveComp->bUseControllerDesiredRotation = OriginalDesiredRotation;
    MoveComp->RotationRate = FRotator(0.f, 500.f, 0.f);
}

void UGliderComponent::HandleDescent(float DeltaTime)
{
    UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
    FVector Velocity = MoveComp->Velocity;

    if (Velocity.Z != -DescendingRate)
    {
        Velocity.Z = UKismetMathLibrary::FInterpTo(Velocity.Z, -DescendingRate, DeltaTime, 3.f);
        MoveComp->Velocity = Velocity;
    }
}

