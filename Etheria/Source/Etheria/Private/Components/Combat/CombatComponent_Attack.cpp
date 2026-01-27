/**
 * Etheria's End Project, 2025
 * Created by: 0nnen
 * Last Updated by: 0nnen
 * Class: "CombatComponent - Source (Attacks)"
 * Notes: Attack execution, traces, target assist, and damage application.
 */

#include "Components/Combat/CombatComponent.h"

#include "Characters/BaseCharacter.h"
#include "Components/Combat/LockTarget/LockTargetComponent.h"
#include "Components/Characters/CharacterStateComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

#pragma region ATTACK

const FAttackSpecConfig* UCombatComponent::FindAttack(FName AttackId) const
{
    return Attacks.FindByPredicate([&](const FAttackSpecConfig& S){ return S.AttackId == AttackId; });
}

bool UCombatComponent::TryAttackGroup(FName GroupId)
{
    if (IsInCooldown()) return false;

    const bool bInAir =
        (OwnerCharacter.IsValid() &&
         OwnerCharacter->GetCharacterMovement() &&
         OwnerCharacter->GetCharacterMovement()->IsFalling());

    const FAttackSpecConfig* Chosen = nullptr;
    for (const FAttackSpecConfig& S : Attacks)
    {
        if (S.Group != GroupId) continue;
        if (S.Stance == EStance::AirOnly    && !bInAir) continue;
        if (S.Stance == EStance::GroundOnly &&  bInAir) continue;
        Chosen = &S; break; // first match wins
    }

    if (!Chosen) return false;
    return TryAttackById(Chosen->AttackId);
}

bool UCombatComponent::TryAttackPrimary()
{
    if (Attacks.Num() == 0) return false;
    return TryAttackById(Attacks[0].AttackId);
}

bool UCombatComponent::TryAttackById(FName AttackId, float ChargeLevel)
{
    if (IsInCooldown()) return false;

    const FAttackSpecConfig* Spec = FindAttack(AttackId);
    if (!Spec) return false;
    if (!CanExecuteAttack(AttackId)) return false;

    // Combo start cooldown gating (per combo)
    {
        const FComboSpecConfig* GateCombo = nullptr;
        for (const FComboSpecConfig& C : Combos)
        {
            if (C.Steps.Num() > 0 && C.Steps[0].AttackId == AttackId) { GateCombo = &C; break; }
        }
        if (GateCombo)
        {
            const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
            const float Cd = (GateCombo->Cooldown > 0.f) ? GateCombo->Cooldown : DefaultComboStartCooldown;
            if (Cd > 0.f)
            {
                if (const float* Until = ComboCooldownUntil.Find(GateCombo->ComboId))
                {
                    if (Now < *Until)
                    {
                        return false; // still cooling down
                    }
                }
                ComboCooldownUntil.FindOrAdd(GateCombo->ComboId) = Now + Cd;
            }
        }
    }

    CurrentAttackId = AttackId;

    // Seed combo state to the index of this attack if it belongs to a combo.
    {
        const FComboSpecConfig* FoundCombo = nullptr;
        int32 FoundIndex = -1;

        for (const FComboSpecConfig& C : Combos)
        {
            for (int32 i = 0; i < C.Steps.Num(); ++i)
            {
                if (C.Steps[i].AttackId == AttackId)
                {
                    FoundCombo = &C;
                    FoundIndex = i;
                    break;
                }
            }
            if (FoundCombo) break;
        }

        if (FoundCombo)
        {
            ActiveComboId   = FoundCombo->ComboId;
            ActiveComboStep = FoundIndex; // exact index of the section we are about to play
        }
        else
        {
            ActiveComboId   = NAME_None;
            ActiveComboStep = -1;
        }
    }

    if (Spec->Charge.bChargeable)
    {
        // This is the "manual charge" style (BeginCharge/EndCharge driven by input).
        // Play montage immediately if desired; release will execute attack.
        if (OwnerCharacter.IsValid() && Spec->Montage)
        {
            PrePlayMontageSafety(*Spec);
            OwnerCharacter->PlayAnimMontage(Spec->Montage, 1.f, Spec->MontageSection);
        }

        if (StateComp.IsValid())
        {
            StateComp->SetCombatState(EtheriaTags::State_Combat_Attacking_Charging);
        }

        return true;
    }

    ExecuteAttack(*Spec, 1.f, 1.f);
    return true;
}

bool UCombatComponent::CanExecuteAttack(const FName AttackId) const
{
    return !IsInCooldown();
}

#pragma endregion

#pragma region EXECUTION / WINDOWS

void UCombatComponent::ExecuteAttack(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale)
{
    OnCue.Broadcast(FName("AttackStart"), ECombatCuePhase::Start);
    OnAttackStarted.Broadcast(Spec.AttackId);

    CurrentAttackMontage = Spec.Montage;
    
    WeaponDissolve_PingActivity();

    if (StateComp.IsValid())
    {
        StateComp->SetCombatState(EtheriaTags::State_Combat_Attacking);
    }

    if (OwnerCharacter.IsValid() && Spec.Montage)
    {
        PlayOrJumpMontageSection(Spec);
    }

    // Only notifies or timers drive the hit window. No eager hit now.
    if (Spec.HitWindow > 0.f)
    {
        OpenWindowWithTimers(Spec);
    }

    if (UWorld* W = GetWorld())
    {
        CooldownEndTime = W->GetTimeSeconds() + FMath::Max(0.f, Spec.Cooldown);
    }
}

void UCombatComponent::OpenWindowWithTimers(const FAttackSpecConfig& Spec)
{
    if (UWorld* W = GetWorld())
    {
        FTimerHandle HStart;
        W->GetTimerManager().SetTimer(HStart, [this, Spec]()
        {
            BeginAttackWindow();
            if (UWorld* W2 = GetWorld())
            {
                FTimerHandle HEnd;
                W2->GetTimerManager().SetTimer(HEnd, [this](){ EndAttackWindow(); }, Spec.HitWindow, false);
            }
        }, Spec.HitStartDelay, false);
    }
}

void UCombatComponent::CloseCurrentAttack()
{
    OnAttackEnded.Broadcast(CurrentAttackId);
    OnCue.Broadcast(FName("AttackEnd"), ECombatCuePhase::End);

    CurrentAttackMontage = nullptr;

    LastAttackId = CurrentAttackId;
    CurrentAttackId = NAME_None;

    AdvanceComboIfRequested();
    
    if (StateComp.IsValid())
    {
        StateComp->ClearCombatState();
    }
}

void UCombatComponent::BeginAttackWindow()
{
    bInAttackWindow = true;
    OnCue.Broadcast(FName("HitWindow"), ECombatCuePhase::Start);

    const FAttackSpecConfig* Spec = FindAttack(CurrentAttackId);
    if (!Spec) return;

    FVector Fwd;
    const FVector Eye = GetEyeLocationForward(Fwd);
    Fwd = ApplyMagnetismBias(Fwd, Eye);
    NudgeOwnerRotationToward(Fwd, MaxAutoYawOnAttackDeg);

    ClearRecentHitActors();

    switch (Spec->AttackType)
    {
        case EAttackType::Melee:  PerformMeleeTrace(*Spec, 1.f, 1.f);  break;
        case EAttackType::AoE:    PerformAoE(*Spec, 1.f, 1.f);         break;
        case EAttackType::Ranged: PerformRangedLine(*Spec, 1.f, 1.f);  break;
        default: break;
    }
}

void UCombatComponent::EndAttackWindow()
{
    bInAttackWindow = false;
    OnCue.Broadcast(FName("HitWindow"), ECombatCuePhase::End);
    CloseCurrentAttack();
}

void UCombatComponent::EndHitWindow()
{
    bInAttackWindow = false;
    OnCue.Broadcast(FName("HitWindow"), ECombatCuePhase::End);
}

#pragma endregion

#pragma region FACING AND TARGET ASSIST

FVector UCombatComponent::GetEyeLocationForward(FVector& OutForward) const
{
    FVector Loc = FVector::ZeroVector;
    FRotator Rot = FRotator::ZeroRotator;

    if (OwnerCharacter.IsValid())
    {
        Loc = OwnerCharacter->GetActorLocation();
        Rot = OwnerCharacter->GetActorRotation();
        if (USkeletalMeshComponent* M = OwnerCharacter->GetMesh())
        {
            Loc = M->GetComponentLocation();
        }
    }
    else if (GetOwner())
    {
        Loc = GetOwner()->GetActorLocation();
        Rot = GetOwner()->GetActorRotation();
    }

    OutForward = Rot.Vector();
    return Loc;
}

AActor* UCombatComponent::ResolveBestTarget(const FVector& EyeLoc, const FVector& Forward) const
{
    if (AActor* L = GetCurrentTarget()) return L;

    AActor* Owner = GetOwner();
    if (!Owner) return nullptr;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
    ObjTypes.Add(UEngineTypes::ConvertToObjectType(DamageTraceChannel));

    TArray<AActor*> Ignore;
    Ignore.Add(Owner);

    TArray<AActor*> OutActors;
    const bool bHit = UKismetSystemLibrary::SphereOverlapActors(
        Owner, EyeLoc, 1200.f, ObjTypes, AActor::StaticClass(), Ignore, OutActors
    );
    if (!bHit) return nullptr;

    float BestScore = -FLT_MAX;
    AActor* Best = nullptr;

    for (AActor* A : OutActors)
    {
        if (!A || A == Owner) continue;
        const FVector Dir = (A->GetActorLocation() - EyeLoc).GetSafeNormal();
        const float CosAng = FVector::DotProduct(Dir, Forward);
        const float AngDeg = FMath::RadiansToDegrees(acosf(FMath::Clamp(CosAng, -1.f, 1.f)));
        if (AngDeg > MagnetismAngleDeg) continue;

        const float Dist = FVector::Dist(EyeLoc, A->GetActorLocation());
        const float Score = (1.0f - FMath::Clamp(Dist / 1200.f, 0.f, 1.f)) + CosAng * 1.5f;
        if (Score > BestScore) { BestScore = Score; Best = A; }
    }
    return Best;
}

FVector UCombatComponent::ApplyMagnetismBias(const FVector& RawForward, const FVector& EyeLoc) const
{
    AActor* T = ResolveBestTarget(EyeLoc, RawForward);
    if (!T) return RawForward;

    const FVector ToTarget = (T->GetActorLocation() - EyeLoc).GetSafeNormal();
    const float CosAng = FVector::DotProduct(ToTarget, RawForward);
    const float AngDeg = FMath::RadiansToDegrees(acosf(FMath::Clamp(CosAng, -1.f, 1.f)));
    if (AngDeg > MagnetismAngleDeg) return RawForward;

    return (RawForward * (1.f - MagnetismStrength) + ToTarget * MagnetismStrength).GetSafeNormal();
}

void UCombatComponent::NudgeOwnerRotationToward(const FVector& Direction, float MaxYawDeltaDeg) const
{
    if (!OwnerCharacter.IsValid()) return;

    const FRotator Current = OwnerCharacter->GetActorRotation();
    const FRotator Target = Direction.Rotation();

    const float Delta = FMath::FindDeltaAngleDegrees(Current.Yaw, Target.Yaw);
    const float Clamped = FMath::Clamp(Delta, -MaxAutoYawOnAttackDeg, MaxAutoYawOnAttackDeg);

    FRotator NewR = Current;
    NewR.Yaw = Current.Yaw + Clamped;

    OwnerCharacter->SetActorRotation(NewR);
}

#pragma endregion

#pragma region DAMAGE AND DEFENSE

float UCombatComponent::ComputeFinalDamageForTarget(AActor* Victim, float RawDamage, bool& bOutCrit, float CritChance, float CritMultiplier) const
{
    bOutCrit = false;
    if (!Victim) return 0.f;

    const bool bCrit = (FMath::FRand() < FMath::Clamp(CritChance, 0.f, 1.f));
    const float CritMul = bCrit ? FMath::Max(1.f, CritMultiplier) : 1.f;
    bOutCrit = bCrit;

    float Damage = RawDamage * CritMul;

    if (const UCombatComponent* VictimCombat = Victim->FindComponentByClass<UCombatComponent>())
    {
        if (VictimCombat->bInDodgeIFrames)
        {
            Damage = 0.f;
            if (VictimCombat->bPerfectDodgeWindow)
            {
                const_cast<UCombatComponent*>(VictimCombat)->OnPerfect.Broadcast(EPerfectKind::Dodge);
                const_cast<UCombatComponent*>(VictimCombat)->ApplyPerfectBoost(EPerfectKind::Dodge);
                const_cast<UCombatComponent*>(VictimCombat)->OnCue.Broadcast(FName("PerfectDodge"), ECombatCuePhase::Impact);
            }
        }
        else if (VictimCombat->bParryHeld)
        {
            if (VictimCombat->bPerfectParryWindow)
            {
                Damage = 0.f;
                const_cast<UCombatComponent*>(VictimCombat)->OnPerfect.Broadcast(EPerfectKind::Parry);
                const_cast<UCombatComponent*>(VictimCombat)->ApplyPerfectBoost(EPerfectKind::Parry);
                const_cast<UCombatComponent*>(VictimCombat)->OnCue.Broadcast(FName("PerfectParry"), ECombatCuePhase::Impact);
            }
            else
            {
                Damage *= FMath::Clamp(VictimCombat->ParryDamageFactorWhileHeld, 0.f, 1.f);
                const_cast<UCombatComponent*>(VictimCombat)->OnCue.Broadcast(FName("ParryGuard"), ECombatCuePhase::Impact);

                if (VictimCombat->ParryHitReactionMontages.Num() > 0 && VictimCombat->OwnerCharacter.IsValid())
                {
                    const int32 Idx = FMath::RandRange(0, VictimCombat->ParryHitReactionMontages.Num() - 1);
                    if (VictimCombat->ParryHitReactionMontages[Idx])
                    {
                        VictimCombat->OwnerCharacter->PlayAnimMontage(VictimCombat->ParryHitReactionMontages[Idx], 1.f);
                    }
                }
            }
        }
    }

    return Damage;
}

#pragma endregion

#pragma region TRACE HELPERS

TArray<AActor*> UCombatComponent::UniqueActorsFromHits(const TArray<FHitResult>& Hits) const
{
    TArray<AActor*> Result;
    for (const FHitResult& H : Hits)
    {
        AActor* Other = H.GetActor();
        if (!Other) continue;
        if (bIgnoreOwner && Other == GetOwner()) continue;
        Result.AddUnique(Other);
    }
    return Result;
}

#pragma endregion

#pragma region PERFORM TRACE

void UCombatComponent::PerformMeleeTrace(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale)
{
    if (!GetWorld()) return;

    FVector Fwd;
    const FVector Eye = GetEyeLocationForward(Fwd);
    Fwd = ApplyMagnetismBias(Fwd, Eye);

    const float Range = Spec.Range * RangeScale;
    const FVector Start = Eye;
    const FVector End = Eye + Fwd * Range;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(CombatMelee), false, GetOwner());
    FCollisionObjectQueryParams Obj;
    Obj.AddObjectTypesToQuery(DamageTraceChannel);

    TArray<FHitResult> Hits;

    switch (Spec.TraceShape)
    {
        case ETraceShape::Line:
        {
            FHitResult H;
            if (GetWorld()->LineTraceSingleByObjectType(H, Start, End, Obj, Params)) Hits.Add(H);
        } break;

        case ETraceShape::Sphere:
        {
            const float R = Spec.Radius * RangeScale;
            GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Obj, FCollisionShape::MakeSphere(R), Params);
        } break;

        case ETraceShape::Capsule:
        {
            const float R = Spec.Radius * RangeScale;
            const float HH = Spec.CapsuleHalfHeight * RangeScale;
            GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Obj, FCollisionShape::MakeCapsule(R, HH), Params);
        } break;
    }

#if !(UE_BUILD_SHIPPING)
    if (bDebugDraw)
    {
        DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.f, 0, 1.f);
    }
#endif

    const TArray<AActor*> Unique = UniqueActorsFromHits(Hits);
    for (AActor* Other : Unique)
    {
        bool bCrit = false;
        const float FinalDamage = ComputeFinalDamageForTarget(Other, Spec.BaseDamage * DamageScale, bCrit, Spec.CritChance, Spec.CritMultiplier);
        if (FinalDamage <= 0.f) continue;

        FHitResult Dummy;
        Dummy.TraceStart = Start;
        Dummy.ImpactPoint = Other->GetActorLocation();

        UGameplayStatics::ApplyPointDamage(Other, FinalDamage, Fwd, Dummy, GetOwner()->GetInstigatorController(), GetOwner(), nullptr);

        if (bCrit) OnHitCrit.Broadcast(Other, FinalDamage);
        else       OnHit.Broadcast(Other, FinalDamage);

        OnCue.Broadcast(FName("Impact"), ECombatCuePhase::Impact);
        PushRecentHitActor(Other);
    }
}

void UCombatComponent::PerformRangedLine(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale)
{
    if (!GetWorld()) return;

    // Use mesh or actor facing, not the camera
    FVector Fwd;
    const FVector Origin = GetEyeLocationForward(Fwd);
    const float Range = Spec.Range * RangeScale;

    // Try muzzle socket if available
    FVector Start = Origin;
    if (OwnerCharacter.IsValid() && OwnerCharacter->GetMesh())
    {
        if (OwnerCharacter->GetMesh()->DoesSocketExist(MuzzleSocketName))
        {
            Start = OwnerCharacter->GetMesh()->GetSocketLocation(MuzzleSocketName);
        }
    }

    const FVector End = Start + Fwd * Range;

    FCollisionObjectQueryParams Obj;
    Obj.AddObjectTypesToQuery(DamageTraceChannel);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(CombatRanged), false, GetOwner());

    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, Obj, Params);

#if !(UE_BUILD_SHIPPING)
    if (bDebugDraw)
    {
        DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, 1.f, 0, 1.5f);
        if (bHit) DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.f, FColor::Yellow, false, 1.f);
    }
#endif

    if (!bHit) return;

    AActor* Other = Hit.GetActor();
    if (!Other) return;
    if (bIgnoreOwner && Other == GetOwner()) return;

    bool bCrit = false;
    const float FinalDamage = ComputeFinalDamageForTarget(
        Other,
        Spec.BaseDamage * DamageScale,
        bCrit,
        Spec.CritChance,
        Spec.CritMultiplier
    );
    if (FinalDamage <= 0.f) return;

    UGameplayStatics::ApplyPointDamage(
        Other,
        FinalDamage,
        Fwd,
        Hit,
        GetOwner()->GetInstigatorController(),
        GetOwner(),
        nullptr
    );

    if (bCrit) OnHitCrit.Broadcast(Other, FinalDamage);
    else       OnHit.Broadcast(Other, FinalDamage);

    OnCue.Broadcast(FName("Impact"), ECombatCuePhase::Impact);
    PushRecentHitActor(Other);
}


void UCombatComponent::HandleRangedProjectileImpact(AActor* HitActor, const FHitResult& Hit, FName AttackId, float ChargeAlpha, float DamageScale)
{
    if (!GetWorld() || !GetOwner() || !HitActor) return;
    if (bIgnoreOwner && HitActor == GetOwner()) return;

    const FAttackSpecConfig* Spec = FindAttack(AttackId);
    if (!Spec)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Combat] HandleRangedProjectileImpact: AttackId '%s' not found."), *AttackId.ToString());
        return;
    }

    // Resolve charge multipliers from AttackSpec charge levels (optional).
    float ChargeDamageMul = 1.f;
    float ChargeRangeMul  = 1.f;

    const float Alpha = FMath::Clamp(ChargeAlpha, 0.f, 1.f);
    if (Spec->Charge.bChargeable && Spec->Charge.Levels.Num() > 0)
    {
        float Total = 0.f;
        for (const FChargeLevelConfig& L : Spec->Charge.Levels)
        {
            Total += FMath::Max(0.001f, L.Time);
        }

        // Map alpha to a time along the charge curve.
        float TargetT = Alpha * Total;
        float Acc = 0.f;

        // Start values (no charge)
        float PrevDM = 1.f;
        float PrevRM = 1.f;

        for (int32 i = 0; i < Spec->Charge.Levels.Num(); ++i)
        {
            const FChargeLevelConfig& L = Spec->Charge.Levels[i];
            const float Segment = FMath::Max(0.001f, L.Time);
            const float NextAcc = Acc + Segment;

            const float NextDM = L.DamageMultiplier;
            const float NextRM = L.RangeMultiplier;

            if (TargetT <= NextAcc)
            {
                const float LocalA = (TargetT - Acc) / Segment;
                ChargeDamageMul = FMath::Lerp(PrevDM, NextDM, LocalA);
                ChargeRangeMul  = FMath::Lerp(PrevRM, NextRM, LocalA);
                break;
            }

            PrevDM = NextDM;
            PrevRM = NextRM;
            Acc = NextAcc;

            // If we exceeded all levels, clamp to last
            if (i == Spec->Charge.Levels.Num() - 1)
            {
                ChargeDamageMul = NextDM;
                ChargeRangeMul  = NextRM;
            }
        }
    }

    const float FinalDamageScale = DamageScale * ChargeDamageMul;

    bool bCrit = false;
    const float FinalDamage = ComputeFinalDamageForTarget(
        HitActor,
        Spec->BaseDamage * FinalDamageScale,
        bCrit,
        Spec->CritChance,
        Spec->CritMultiplier
    );
    if (FinalDamage <= 0.f) return;

    FVector Dir = FVector::ZeroVector;
    if (!Hit.TraceStart.IsNearlyZero())
    {
        Dir = (Hit.ImpactPoint - Hit.TraceStart).GetSafeNormal();
    }
    if (Dir.IsNearlyZero())
    {
        Dir = (HitActor->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal();
    }

    UGameplayStatics::ApplyPointDamage(
        HitActor,
        FinalDamage,
        Dir,
        Hit,
        GetOwner()->GetInstigatorController(),
        GetOwner(),
        nullptr
    );

    if (bCrit) OnHitCrit.Broadcast(HitActor, FinalDamage);
    else       OnHit.Broadcast(HitActor, FinalDamage);

    OnCue.Broadcast(FName("Impact"), ECombatCuePhase::Impact);
    PushRecentHitActor(HitActor);

#if !(UE_BUILD_SHIPPING)
    if (bDebugDraw)
    {
        DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 14.f, bCrit ? FColor::Orange : FColor::Green, false, 1.f);
        DrawDebugString(GetWorld(), Hit.ImpactPoint + FVector(0,0,12.f), FString::Printf(TEXT("%.0f%%"), Alpha * 100.f), nullptr, FColor::White, 1.f);
    }
#endif
}

void UCombatComponent::PerformAoE(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale)
{
    if (!GetWorld()) return;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    const FVector Center = Owner->GetActorLocation();
    const float Radius = Spec.Radius * RangeScale;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
    ObjTypes.Add(UEngineTypes::ConvertToObjectType(DamageTraceChannel));

    TArray<AActor*> Ignore;
    Ignore.Add(Owner);

    TArray<AActor*> OutActors;
    const bool bHit = UKismetSystemLibrary::SphereOverlapActors(Owner, Center, Radius, ObjTypes, AActor::StaticClass(), Ignore, OutActors);
    if (!bHit) return;

#if !(UE_BUILD_SHIPPING)
    if (bDebugDraw)
    {
        DrawDebugSphere(GetWorld(), Center, Radius, 16, FColor::Red, false, 1.f, 0, 1.f);
    }
#endif

    for (AActor* Other : OutActors)
    {
        if (!Other) continue;
        if (bIgnoreOwner && Other == Owner) continue;

        bool bCrit = false;
        const float FinalDamage = ComputeFinalDamageForTarget(Other, Spec.BaseDamage * DamageScale, bCrit, Spec.CritChance, Spec.CritMultiplier);
        if (FinalDamage <= 0.f) continue;

        FHitResult Dummy;
        Dummy.ImpactPoint = Other->GetActorLocation();

        UGameplayStatics::ApplyPointDamage(Other, FinalDamage, FVector::UpVector, Dummy, Owner->GetInstigatorController(), Owner, nullptr);

        if (bCrit) OnHitCrit.Broadcast(Other, FinalDamage);
        else       OnHit.Broadcast(Other, FinalDamage);

        OnCue.Broadcast(FName("Impact"), ECombatCuePhase::Impact);
        PushRecentHitActor(Other);
    }
}

void UCombatComponent::PerformFrontalRect(const FAttackSpecConfig& Spec, float DamageScale, float RangeScale)
{
    if (!GetWorld()) return;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    FVector Fwd;
    const FVector Eye = GetEyeLocationForward(Fwd);
    Fwd = ApplyMagnetismBias(Fwd, Eye);

    const float Length = (Spec.Charge.MaxLength > 0.f ? Spec.Charge.MaxLength : Spec.Range) * RangeScale;
    const float Width  = (Spec.Charge.MaxWidth  > 0.f ? Spec.Charge.MaxWidth  : Spec.Radius * 2.f) * RangeScale;

    const FVector Center = Owner->GetActorLocation() + Fwd * (Length * 0.5f);
    const FVector Extents(Width * 0.5f, Length * 0.5f, 100.f);

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
    ObjTypes.Add(UEngineTypes::ConvertToObjectType(DamageTraceChannel));

    TArray<AActor*> Ignore;
    Ignore.Add(Owner);

    TArray<AActor*> OutActors;
    const bool bHit = UKismetSystemLibrary::BoxOverlapActors(Owner, Center, Extents, ObjTypes, AActor::StaticClass(), Ignore, OutActors);
    if (!bHit) return;

    for (AActor* Other : OutActors)
    {
        if (!Other) continue;
        if (bIgnoreOwner && Other == Owner) continue;

        bool bCrit = false;
        const float FinalDamage = ComputeFinalDamageForTarget(Other, Spec.BaseDamage * DamageScale, bCrit, Spec.CritChance, Spec.CritMultiplier);
        if (FinalDamage <= 0.f) continue;

        FHitResult Dummy;
        Dummy.ImpactPoint = Other->GetActorLocation();

        UGameplayStatics::ApplyPointDamage(Other, FinalDamage, Fwd, Dummy, Owner->GetInstigatorController(), Owner, nullptr);

        if (bCrit) OnHitCrit.Broadcast(Other, FinalDamage);
        else       OnHit.Broadcast(Other, FinalDamage);

        OnCue.Broadcast(FName("Impact"), ECombatCuePhase::Impact);
        PushRecentHitActor(Other);
    }

#if !(UE_BUILD_SHIPPING)
    if (bDebugDraw)
    {
        DrawDebugBox(GetWorld(), Center, Extents, FQuat(Fwd.Rotation()), FColor::Orange, false, 1.f, 0, 1.f);
    }
#endif
}

#pragma endregion
