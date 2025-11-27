/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "AnimNotify_ApplyCombatImpulse" - Source
 */
#include "Components/Combat/AnimNotify_ApplyCombatImpulse.h"
#include "Components/Combat/CombatComponent.h"
#include "GameFramework/Character.h"
#include "Components/PrimitiveComponent.h"

static FVector ResolveDir(AActor* Owner, EEEImpulseDir Dir, const FVector& Custom)
{
    if (!Owner) return FVector::UpVector;
    switch(Dir)
    {
        case EEEImpulseDir::Up:      return FVector::UpVector;
        case EEEImpulseDir::Down:    return -FVector::UpVector;
        case EEEImpulseDir::Forward: return Owner->GetActorForwardVector();
        case EEEImpulseDir::Custom:  return Custom.GetSafeNormal();
    }
    return FVector::UpVector;
}

void UAnimNotify_ApplyCombatImpulse::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner) return;

    UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>();
    if (!Combat) return;

    const FVector Dir = ResolveDir(Owner, Direction, CustomDirection);
    const FVector Impulse = Dir * Magnitude;

    if (Target == EEEImpulseTarget::Self || Target == EEEImpulseTarget::Both)
    {
        if (ACharacter* C = Cast<ACharacter>(Owner))
        {
            C->LaunchCharacter(Impulse, true, true);
        }
        else if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent()))
        {
            Prim->AddImpulse(Impulse, NAME_None, true);
        }
    }

    if (Target == EEEImpulseTarget::Victims || Target == EEEImpulseTarget::Both)
    {
        TArray<AActor*> Victims;
        Combat->GetRecentHitActors(Victims);
        for (AActor* V : Victims)
        {
            if (!V) continue;
            if (ACharacter* CV = Cast<ACharacter>(V))
            {
                if (bVictimsUseLaunchCharacter) { CV->LaunchCharacter(Impulse, true, true); }
                else if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(CV->GetRootComponent()))
                {
                    Prim->AddImpulse(Impulse, NAME_None, true);
                }
            }
            else if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(V->GetRootComponent()))
            {
                Prim->AddImpulse(Impulse, NAME_None, true);
            }
        }
    }
}