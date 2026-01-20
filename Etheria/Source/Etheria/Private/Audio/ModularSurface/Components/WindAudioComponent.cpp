/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UWindAudioComponent" - Source
 */

#include "Audio/ModularSurface/Components/WindAudioComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

UWindAudioComponent::UWindAudioComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWindAudioComponent::BeginPlay()
{
    Super::BeginPlay();

    // No auto start timer; we only run when enabled.
    if (bForceEnabled)
    {
        SetEnabled(true);
    }
}

void UWindAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopTimer();

    if (WindAudioComponent)
    {
        WindAudioComponent->Stop();
        WindAudioComponent = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void UWindAudioComponent::SetEnabled(bool bNewEnabled)
{
    bEnabled = bNewEnabled;

    if (bEnabled)
    {
        EnsureAudioComponent();
        StartTimer();
    }
    else
    {
        StopTimer();
        if (WindAudioComponent && WindAudioComponent->IsPlaying())
        {
            WindAudioComponent->FadeOut(FadeOutSeconds, 0.0f);
        }
    }
}

void UWindAudioComponent::EnsureAudioComponent()
{
    if (WindAudioComponent)
    {
        return;
    }

    if (!Profile || !Profile->WindLoop)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    WindAudioComponent = NewObject<UAudioComponent>(Owner);
    WindAudioComponent->bAutoActivate = false;
    WindAudioComponent->bIsUISound = false;
    WindAudioComponent->SetSound(Profile->WindLoop);

    if (Profile->Attenuation)
    {
        WindAudioComponent->AttenuationSettings = Profile->Attenuation;
    }

    WindAudioComponent->RegisterComponent();
    WindAudioComponent->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
}

void UWindAudioComponent::StartTimer()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float Interval = (Profile && Profile->UpdateInterval > 0.0f) ? Profile->UpdateInterval : 0.05f;
    World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &UWindAudioComponent::UpdateWind, Interval, true);
}

void UWindAudioComponent::StopTimer()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(UpdateTimerHandle);
}

float UWindAudioComponent::EvalCurve(const UCurveFloat* Curve, float X, float DefaultValue) const
{
    if (!Curve)
    {
        return DefaultValue;
    }

    return Curve->GetFloatValue(X);
}

void UWindAudioComponent::UpdateWind()
{
    if (!Profile || !Profile->WindLoop)
    {
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    // Auto enable while falling (no tick; driven by timer)
    if (bAutoEnableWhileFalling && !bForceEnabled)
    {
        if (const ACharacter* Char = Cast<ACharacter>(Owner))
        {
            const UCharacterMovementComponent* Move = Char->GetCharacterMovement();
            if (Move)
            {
                const bool bShould = Move->IsFalling();
                if (!bShould && bEnabled)
                {
                    // If we were enabled purely by falling, keep enabled but fade based on speed.
                }
            }
        }
    }

    EnsureAudioComponent();
    if (!WindAudioComponent)
    {
        return;
    }

    const FVector Vel = Owner->GetVelocity();
    const float Speed = Vel.Size();

    const float MinSpeed = Profile->MinSpeed;
    const bool bAbove = Speed >= MinSpeed;

    if (!bEnabled)
    {
        // If not enabled, do nothing.
        return;
    }

    if (!bAbove)
    {
        if (WindAudioComponent->IsPlaying())
        {
            WindAudioComponent->FadeOut(FadeOutSeconds, 0.0f);
        }
        return;
    }

    if (!WindAudioComponent->IsPlaying())
    {
        WindAudioComponent->FadeIn(FadeInSeconds, 1.0f);
    }

    const FVector VelN = Speed > KINDA_SMALL_NUMBER ? (Vel / Speed) : FVector::ZeroVector;
    const FVector Fwd = Owner->GetActorForwardVector();
    const FVector Right = Owner->GetActorRightVector();

    const float ForwardDot = FVector::DotProduct(VelN, Fwd);
    const float RightDot = FVector::DotProduct(VelN, Right);

    const float Vol = FMath::Max(0.0f, EvalCurve(Profile->VolumeCurve, Speed, 1.0f));
    const float Pit = FMath::Max(0.0f, EvalCurve(Profile->PitchCurve, Speed, 1.0f));

    WindAudioComponent->SetVolumeMultiplier(Vol);
    WindAudioComponent->SetPitchMultiplier(Pit);

    // MetaSound parameters (if the sound supports them, harmless otherwise)
    WindAudioComponent->SetFloatParameter(Profile->SpeedParam, Speed);
    WindAudioComponent->SetFloatParameter(Profile->ForwardDotParam, ForwardDot);
    WindAudioComponent->SetFloatParameter(Profile->RightDotParam, RightDot);
}
