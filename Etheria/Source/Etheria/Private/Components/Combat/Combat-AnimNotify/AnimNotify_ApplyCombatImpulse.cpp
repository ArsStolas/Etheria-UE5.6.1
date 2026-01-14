/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "AnimNotify_ApplyCombatImpulse" - Source
 */
#include "Components/Combat/Combat-AnimNotify/AnimNotify_ApplyCombatImpulse.h"

#include "Components/Combat/CombatComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Components/PrimitiveComponent.h"

static FVector MakeDirFromEnum(AActor* Owner, EEEImpulseDir Dir, const FVector& Custom)
{
    if (!Owner) return FVector::UpVector;

    const FVector Fwd = Owner->GetActorForwardVector();
    const FVector Right = Owner->GetActorRightVector();

    switch (Dir)
    {
        case EEEImpulseDir::Up:            return FVector::UpVector;
        case EEEImpulseDir::Down:          return -FVector::UpVector;

        case EEEImpulseDir::Forward:       return Fwd;
        case EEEImpulseDir::Backward:      return -Fwd;
        case EEEImpulseDir::Right:         return Right;
        case EEEImpulseDir::Left:          return -Right;

        case EEEImpulseDir::ForwardRight:  return (Fwd + Right).GetSafeNormal();
        case EEEImpulseDir::ForwardLeft:   return (Fwd - Right).GetSafeNormal();
        case EEEImpulseDir::BackwardRight: return (-Fwd + Right).GetSafeNormal();
        case EEEImpulseDir::BackwardLeft:  return (-Fwd - Right).GetSafeNormal();

        case EEEImpulseDir::Custom:        return Custom.GetSafeNormal();
        default:                           break;
    }

    return FVector::UpVector;
}

static bool GetPawnInputWorldDir(AActor* Owner, FVector& OutWorldDir)
{
    OutWorldDir = FVector::ZeroVector;

    APawn* Pawn = Cast<APawn>(Owner);
    if (!Pawn) return false;

    FVector V = Pawn->GetLastMovementInputVector();
    if (V.IsNearlyZero())
    {
        V = Pawn->GetPendingMovementInputVector();
    }

    if (V.IsNearlyZero()) return false;

    OutWorldDir = V.GetSafeNormal();
    return true;
}

static FVector QuantizeDir8(const FVector& WorldDir, const FVector& BasisForward, const FVector& BasisRight)
{
    const float X = FVector::DotProduct(WorldDir, BasisForward);
    const float Y = FVector::DotProduct(WorldDir, BasisRight);

    FVector2D Local(X, Y);
    if (Local.IsNearlyZero()) return FVector::ZeroVector;

    const float Angle = FMath::Atan2(Local.Y, Local.X); // -PI..PI
    const float Step = PI / 4.f;                        // 45 deg
    const float Snapped = FMath::RoundToFloat(Angle / Step) * Step;

    const float CosA = FMath::Cos(Snapped);
    const float SinA = FMath::Sin(Snapped);

    FVector SnappedWorld = (BasisForward * CosA + BasisRight * SinA);
    return SnappedWorld.GetSafeNormal();
}

static FVector ResolveFinalDir(
    AActor* Owner,
    EEEImpulseDir Dir,
    const FVector& Custom,
    EEEImpulseInputBasis InputBasis,
    bool bQuantize8,
    float Deadzone)
{
    if (!Owner) return FVector::UpVector;

    // Non-input modes
    if (Dir != EEEImpulseDir::Input)
    {
        return MakeDirFromEnum(Owner, Dir, Custom);
    }

    // Input mode
    FVector InputWorldDir;
    if (!GetPawnInputWorldDir(Owner, InputWorldDir))
    {
        return Owner->GetActorForwardVector(); // fallback
    }

    if (Deadzone > 0.f)
    {
        // Deadzone works only if we had non-normalized, but we can approximate:
        // If the raw vectors were tiny, GetPawnInputWorldDir returns false anyway.
    }

    if (!bQuantize8)
    {
        return InputWorldDir;
    }

    // Quantize relative to chosen basis
    FVector BasisFwd = Owner->GetActorForwardVector();
    FVector BasisRight = Owner->GetActorRightVector();

    if (InputBasis == EEEImpulseInputBasis::ControllerYaw)
    {
        if (APawn* Pawn = Cast<APawn>(Owner))
        {
            if (AController* C = Pawn->GetController())
            {
                const float Yaw = C->GetControlRotation().Yaw;
                const FRotator YawRot(0.f, Yaw, 0.f);
                BasisFwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
                BasisRight = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
            }
        }
    }

    return QuantizeDir8(InputWorldDir, BasisFwd, BasisRight);
}

static void ApplyImpulseToActor(AActor* TargetActor, const FVector& Impulse, bool bOverrideXY, bool bOverrideZ, bool bUseLaunchCharacter)
{
    if (!TargetActor) return;

    // In animation editor preview: skip (optional, but avoids spam)
    if (UWorld* W = TargetActor->GetWorld())
    {
        if (W->WorldType == EWorldType::EditorPreview)
        {
            return;
        }
    }

    // Preferred for dodges / characters
    if (ACharacter* C = Cast<ACharacter>(TargetActor))
    {
        if (bUseLaunchCharacter)
        {
            C->LaunchCharacter(Impulse, bOverrideXY, bOverrideZ);
            return;
        }
    }

    // Physics fallback ONLY if simulating
    if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
    {
        if (Prim->IsSimulatingPhysics())
        {
            Prim->AddImpulse(Impulse, NAME_None, true);
        }
    }
}

void UAnimNotify_ApplyCombatImpulse::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;

    AActor* Owner = MeshComp->GetOwner();
    if (!Owner) return;

    FVector Dir = ResolveFinalDir(
        Owner,
        Direction,
        CustomDirection,
        InputBasis,
        bQuantizeInputTo8Directions,
        InputDeadzone);

    // Optional ground projection for dodges
    if (bProjectToXYPlane && Direction != EEEImpulseDir::Up && Direction != EEEImpulseDir::Down)
    {
        Dir.Z = 0.f;
        Dir = Dir.GetSafeNormal();
        if (Dir.IsNearlyZero())
        {
            Dir = Owner->GetActorForwardVector();
        }
    }

    const FVector Impulse = Dir * Magnitude;

    // Self does NOT require CombatComponent anymore
    if (Target == EEEImpulseTarget::Self || Target == EEEImpulseTarget::Both)
    {
        ApplyImpulseToActor(Owner, Impulse, bOverrideXY, bOverrideZ, true);
    }

    // Victims require CombatComponent
    if (Target == EEEImpulseTarget::Victims || Target == EEEImpulseTarget::Both)
    {
        UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>();
        if (!Combat) return;

        TArray<AActor*> Victims;
        Combat->GetRecentHitActors(Victims);

        for (AActor* V : Victims)
        {
            ApplyImpulseToActor(V, Impulse, bOverrideXY, bOverrideZ, bVictimsUseLaunchCharacter);
        }
    }
}
