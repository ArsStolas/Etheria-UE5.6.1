/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UCombatComponent" - Source
 */
#include "Components/Combat/CombatComponent.h"
#include "Components/Combat/LockTargetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    ResolveOwnerRefs();
    BindDamageHooks();

    if (OwnerCharacter.IsValid())
    {
        MoveComp = OwnerCharacter->GetCharacterMovement();
        if (MoveComp.IsValid())
        {
            BaseWalkSpeed = MoveComp->MaxWalkSpeed;
        }
    }
    if (OwnerMesh.IsValid())
    {
        BaseGlobalAnimRate = OwnerMesh->GlobalAnimRateScale;
    }

    if (AActor* O = GetOwner())
    {
        LockComp = O->FindComponentByClass<ULockTargetComponent>();
    }
}

bool UCombatComponent::ResolveOwnerRefs()
{
    if (ACharacter* C = Cast<ACharacter>(GetOwner()))
    {
        OwnerCharacter = C;
        OwnerMesh = C->GetMesh();
        return true;
    }
    return false;
}

void UCombatComponent::BindDamageHooks()
{
    if (AActor* O = GetOwner())
    {
        O->OnTakeAnyDamage.AddDynamic(this, &UCombatComponent::HandleAnyDamage);
        O->OnTakePointDamage.AddDynamic(this, &UCombatComponent::HandlePointDamage);
    }
}

void UCombatComponent::SetAttacks(const TArray<FEEAttackSpec>& InAttacks)
{
    Attacks = InAttacks;
}

void UCombatComponent::SetCombos(const TArray<FEEComboSpec>& InCombos)
{
    Combos = InCombos;
}

bool UCombatComponent::IsInCooldown() const
{
    UWorld* W = GetWorld();
    return W ? (W->GetTimeSeconds() < CooldownEndTime) : false;
}

void UCombatComponent::SetExternalTarget(AActor* InTarget)
{
    ExternalTarget = InTarget;
}

AActor* UCombatComponent::GetCurrentTarget() const
{
    if (LockComp.IsValid() && LockComp->GetCurrentTarget())
    {
        return LockComp->GetCurrentTarget();
    }
    return ExternalTarget.Get();
}

const FEEAttackSpec* UCombatComponent::FindAttack(FName AttackId) const
{
    return Attacks.FindByPredicate([&](const FEEAttackSpec& S){ return S.AttackId == AttackId; });
}

bool UCombatComponent::TryAttackPrimary()
{
    if (Attacks.Num() == 0) return false;
    return TryAttackById(Attacks[0].AttackId);
}

bool UCombatComponent::TryAttackById(FName AttackId)
{
    if (IsInCooldown()) return false;
    const FEEAttackSpec* Spec = FindAttack(AttackId);
    if (!Spec) return false;
    if (!CanExecuteAttack(AttackId)) return false;

    CurrentAttackId = AttackId;

    if (Spec->Charge.bChargeable)
    {
        if (OwnerCharacter.IsValid() && Spec->Montage)
        {
            OwnerCharacter->PlayAnimMontage(Spec->Montage, 1.f, Spec->MontageSection);
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

void UCombatComponent::ExecuteAttack(const FEEAttackSpec& Spec, float DamageScale, float RangeScale)
{
    OnCue.Broadcast(FName("AttackStart"), EEECombatCuePhase::Start);
    OnAttackStarted.Broadcast(Spec.AttackId);

    if (OwnerCharacter.IsValid())
    {
        UAnimMontage* MontageToPlay = Spec.Montage;
        if (Spec.Charge.bChargeable && Spec.Charge.ReleaseMontages.Num() > 0 && ChargeLevelIndex >= 0 && Spec.Charge.ReleaseMontages.IsValidIndex(ChargeLevelIndex))
        {
            MontageToPlay = Spec.Charge.ReleaseMontages[ChargeLevelIndex] ? Spec.Charge.ReleaseMontages[ChargeLevelIndex] : MontageToPlay;
        }
        if (MontageToPlay)
        {
            OwnerCharacter->PlayAnimMontage(MontageToPlay, 1.f, Spec.MontageSection);
        }
    }

    if (Spec.HitWindow > 0.f)
    {
        OpenWindowWithTimers(Spec);
    }

    if (UWorld* W = GetWorld())
    {
        CooldownEndTime = W->GetTimeSeconds() + FMath::Max(0.f, Spec.Cooldown);
    }

    if (!bInAttackWindow)
    {
        switch (Spec.AttackType)
        {
            case EEEAttackType::Melee: PerformMeleeTrace(Spec, DamageScale, RangeScale); break;
            case EEEAttackType::AoE:
            default: PerformAoE(Spec, DamageScale, RangeScale); break;
        }
    }
}

void UCombatComponent::OpenWindowWithTimers(const FEEAttackSpec& Spec)
{
    if (UWorld* W = GetWorld())
    {
        FTimerHandle HStart;
        W->GetTimerManager().SetTimer(HStart, [this, Spec]()
        {
            BeginAttackWindow();

            FVector Fwd;
            const FVector Eye = GetEyeLocationForward(Fwd);
            Fwd = ApplyMagnetismBias(Fwd, Eye);
            NudgeOwnerRotationToward(Fwd, MaxAutoYawOnAttackDeg);

            if (Spec.AttackType == EEEAttackType::Melee) PerformMeleeTrace(Spec, 1.f, 1.f);
            else if (Spec.AttackType == EEEAttackType::AoE) PerformAoE(Spec, 1.f, 1.f);

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
    OnCue.Broadcast(FName("AttackEnd"), EEECombatCuePhase::End);
    CurrentAttackId = NAME_None;
    AdvanceComboIfRequested();
}

void UCombatComponent::BeginAttackWindow()
{
    bInAttackWindow = true;
    OnCue.Broadcast(FName("HitWindow"), EEECombatCuePhase::Start);

    const FEEAttackSpec* Spec = FindAttack(CurrentAttackId);
    if (Spec)
    {
        if (Spec->AttackType == EEEAttackType::Melee) PerformMeleeTrace(*Spec, 1.f, 1.f);
        else if (Spec->AttackType == EEEAttackType::AoE) PerformAoE(*Spec, 1.f, 1.f);
    }
}

void UCombatComponent::EndAttackWindow()
{
    bInAttackWindow = false;
    OnCue.Broadcast(FName("HitWindow"), EEECombatCuePhase::End);
    CloseCurrentAttack();
}

FVector UCombatComponent::GetEyeLocationForward(FVector& OutForward) const
{
    FVector Loc = FVector::ZeroVector;
    FRotator Rot = FRotator::ZeroRotator;

    if (OwnerCharacter.IsValid())
    {
        OwnerCharacter->GetActorEyesViewPoint(Loc, Rot);
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

    TArray<AActor*> Ignore; Ignore.Add(Owner);
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
    FRotator NewR = Current;
    const float Delta = FMath::FindDeltaAngleDegrees(Current.Yaw, Target.Yaw);
    const float Clamped = FMath::Clamp(Delta, -MaxAutoYawOnAttackDeg, MaxAutoYawOnAttackDeg);
    NewR.Yaw = Current.Yaw + Clamped;
    OwnerCharacter->SetActorRotation(NewR);
}

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
                const_cast<UCombatComponent*>(VictimCombat)->OnPerfect.Broadcast(EEEPerfectKind::Dodge);
                const_cast<UCombatComponent*>(VictimCombat)->ApplyPerfectBoost(EEEPerfectKind::Dodge);
                const_cast<UCombatComponent*>(VictimCombat)->OnCue.Broadcast(FName("PerfectDodge"), EEECombatCuePhase::Impact);
            }
        }
        else if (VictimCombat->bParryHeld)
        {
            if (VictimCombat->bPerfectParryWindow)
            {
                Damage = 0.f;
                const_cast<UCombatComponent*>(VictimCombat)->OnPerfect.Broadcast(EEEPerfectKind::Parry);
                const_cast<UCombatComponent*>(VictimCombat)->ApplyPerfectBoost(EEEPerfectKind::Parry);
                const_cast<UCombatComponent*>(VictimCombat)->OnCue.Broadcast(FName("PerfectParry"), EEECombatCuePhase::Impact);
            }
            else
            {
                Damage *= FMath::Clamp(VictimCombat->ParryDamageFactorWhileHeld, 0.f, 1.f);
                const_cast<UCombatComponent*>(VictimCombat)->OnCue.Broadcast(FName("ParryGuard"), EEECombatCuePhase::Impact);

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

void UCombatComponent::PerformMeleeTrace(const FEEAttackSpec& Spec, float DamageScale, float RangeScale)
{
    if (!GetWorld()) return;

    FVector Fwd;
    const FVector Eye = GetEyeLocationForward(Fwd);
    Fwd = ApplyMagnetismBias(Fwd, Eye);

    const float Range = Spec.Range * RangeScale;
    const FVector Start = Eye;
    const FVector End = Eye + Fwd * Range;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(CombatMelee), false, GetOwner());
    FCollisionObjectQueryParams Obj; Obj.AddObjectTypesToQuery(DamageTraceChannel);

    TArray<FHitResult> Hits;

    switch (Spec.TraceShape)
    {
        case EEETraceShape::Line:
        {
            FHitResult H;
            if (GetWorld()->LineTraceSingleByObjectType(H, Start, End, Obj, Params)) Hits.Add(H);
        } break;
        case EEETraceShape::Sphere:
        {
            const float R = Spec.Radius * RangeScale;
            GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Obj, FCollisionShape::MakeSphere(R), Params);
        } break;
        case EEETraceShape::Capsule:
        {
            const float R = Spec.Radius * RangeScale;
            const float HH = Spec.CapsuleHalfHeight * RangeScale;
            GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Obj, FCollisionShape::MakeCapsule(R, HH), Params);
        } break;
    }

#if !(UE_BUILD_SHIPPING)
    DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.f, 0, 1.f);
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

        if (bCrit) OnHitCrit.Broadcast(Other, FinalDamage); else OnHit.Broadcast(Other, FinalDamage);
        OnCue.Broadcast(FName("Impact"), EEECombatCuePhase::Impact);
    }
}

void UCombatComponent::PerformAoE(const FEEAttackSpec& Spec, float DamageScale, float RangeScale)
{
    if (!GetWorld()) return;
    AActor* Owner = GetOwner(); if (!Owner) return;

    const FVector Center = Owner->GetActorLocation();
    const float Radius = Spec.Radius * RangeScale;

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
    ObjTypes.Add(UEngineTypes::ConvertToObjectType(DamageTraceChannel));

    TArray<AActor*> Ignore; Ignore.Add(Owner);
    TArray<AActor*> OutActors;
    const bool bHit = UKismetSystemLibrary::SphereOverlapActors(
        Owner, Center, Radius, ObjTypes, AActor::StaticClass(), Ignore, OutActors
    );
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
        UGameplayStatics::ApplyPointDamage(Other, FinalDamage, FVector::UpVector, Dummy, Owner->GetInstigatorController(), Owner, nullptr);

        if (bCrit) OnHitCrit.Broadcast(Other, FinalDamage); else OnHit.Broadcast(Other, FinalDamage);
        OnCue.Broadcast(FName("Impact"), EEECombatCuePhase::Impact);
    }
}

void UCombatComponent::PerformFrontalRect(const FEEAttackSpec& Spec, float DamageScale, float RangeScale)
{
    if (!GetWorld()) return;
    AActor* Owner = GetOwner(); if (!Owner) return;

    FVector Fwd;
    const FVector Eye = GetEyeLocationForward(Fwd);
    Fwd = ApplyMagnetismBias(Fwd, Eye);

    const float Length = (Spec.Charge.MaxLength > 0.f ? Spec.Charge.MaxLength : Spec.Range) * RangeScale;
    const float Width  = (Spec.Charge.MaxWidth  > 0.f ? Spec.Charge.MaxWidth  : Spec.Radius*2.f) * RangeScale;
    const FVector Center = Owner->GetActorLocation() + Fwd * (Length * 0.5f);
    const FVector Extents(Width * 0.5f, Length * 0.5f, 100.f);

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
    ObjTypes.Add(UEngineTypes::ConvertToObjectType(DamageTraceChannel));
    TArray<AActor*> Ignore; Ignore.Add(Owner);
    TArray<AActor*> OutActors;

    const bool bHit = UKismetSystemLibrary::BoxOverlapActors(
        Owner, Center, Extents, ObjTypes, AActor::StaticClass(), Ignore, OutActors
    );
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

        if (bCrit) OnHitCrit.Broadcast(Other, FinalDamage); else OnHit.Broadcast(Other, FinalDamage);
        OnCue.Broadcast(FName("Impact"), EEECombatCuePhase::Impact);
    }

#if !(UE_BUILD_SHIPPING)
    DrawDebugBox(GetWorld(), Center, Extents, FQuat(Fwd.Rotation()), FColor::Orange, false, 1.f, 0, 1.f);
#endif
}

/* ================= Parry / Dodge ================= */

void UCombatComponent::SetParryHeld(bool bHeld)
{
    if (bParryHeld == bHeld) return;
    bParryHeld = bHeld;

    if (MoveComp.IsValid() && BaseWalkSpeed > 0.f)
    {
        const float Mult = bParryHeld ? ParryMoveSpeedMultiplier : 1.f;
        MoveComp->MaxWalkSpeed = BaseWalkSpeed * Mult;
    }
}

void UCombatComponent::BeginPerfectParryWindow()
{
    bPerfectParryWindow = true;
    OnCue.Broadcast(FName("PerfectParryWindow"), EEECombatCuePhase::Start);
}

void UCombatComponent::EndPerfectParryWindow()
{
    bPerfectParryWindow = false;
    OnCue.Broadcast(FName("PerfectParryWindow"), EEECombatCuePhase::End);
}

void UCombatComponent::StartDodgeIFrames(float DurationOverride)
{
    if (bInDodgeIFrames) return;
    bInDodgeIFrames = true;

    const float Duration = (DurationOverride > 0.f) ? DurationOverride : DodgeIFrameDuration;
    OnCue.Broadcast(FName("DodgeIFrames"), EEECombatCuePhase::Start);

    if (UWorld* W = GetWorld())
    {
        FTimerHandle H;
        W->GetTimerManager().SetTimer(H, [this]()
        {
            bInDodgeIFrames = false;
            OnCue.Broadcast(FName("DodgeIFrames"), EEECombatCuePhase::End);
        }, Duration, false);
    }
}

void UCombatComponent::BeginPerfectDodgeWindow()
{
    bPerfectDodgeWindow = true;
    OnCue.Broadcast(FName("PerfectDodgeWindow"), EEECombatCuePhase::Start);
}

void UCombatComponent::EndPerfectDodgeWindow()
{
    bPerfectDodgeWindow = false;
    OnCue.Broadcast(FName("PerfectDodgeWindow"), EEECombatCuePhase::End);
}

void UCombatComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser) {}
void UCombatComponent::HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser) {}

void UCombatComponent::ApplyPerfectBoost(EEEPerfectKind Kind)
{
    if (MoveComp.IsValid() && BaseWalkSpeed > 0.f)
    {
        MoveComp->MaxWalkSpeed = BaseWalkSpeed * PerfectMoveSpeedMultiplier;
    }
    if (OwnerMesh.IsValid())
    {
        OwnerMesh->GlobalAnimRateScale = BaseGlobalAnimRate * PerfectAnimRateMultiplier;
    }

    if (UWorld* W = GetWorld())
    {
        FTimerHandle H;
        W->GetTimerManager().SetTimer(H, [this](){ RestoreBoosts(); }, PerfectBoostDuration, false);
    }
}

void UCombatComponent::RestoreBoosts()
{
    if (MoveComp.IsValid() && BaseWalkSpeed > 0.f)
    {
        const float ParryMult = bParryHeld ? ParryMoveSpeedMultiplier : 1.f;
        MoveComp->MaxWalkSpeed = BaseWalkSpeed * ParryMult;
    }
    if (OwnerMesh.IsValid())
    {
        OwnerMesh->GlobalAnimRateScale = BaseGlobalAnimRate;
    }
}

/* ================= Charge ================= */

void UCombatComponent::BeginCharge(FName AttackId, float ExpectedDuration)
{
    if (AttackId == NAME_None)
    {
        if (CurrentAttackId == NAME_None && Attacks.Num() > 0) AttackId = Attacks[0].AttackId;
        else AttackId = CurrentAttackId;
    }

    const FEEAttackSpec* Spec = FindAttack(AttackId);
    if (!Spec || !Spec->Charge.bChargeable) return;

    CurrentAttackId = AttackId;
    bCharging = true;
    ChargeStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    ChargeExpectedDuration = FMath::Max(0.1f, ExpectedDuration);
    ChargeAccumulated = 0.f;
    ChargeLevelIndex = -1;

    if (Spec->Charge.bShowTelegraph)
    {
        SpawnTelegraph();
        UpdateTelegraph(0.f);
    }

    OnCue.Broadcast(FName("ChargeStart"), EEECombatCuePhase::Start);
}

void UCombatComponent::UpdateChargeProgress(float DeltaTime)
{
    if (!bCharging) return;
    ChargeAccumulated += FMath::Max(0.f, DeltaTime);
    const float Alpha = FMath::Clamp(ChargeAccumulated / FMath::Max(0.001f, ChargeExpectedDuration), 0.f, 1.f);
    UpdateTelegraph(Alpha);
}

void UCombatComponent::EndCharge(bool bCanceled)
{
    if (!bCharging) return;
    bCharging = false;

    const FEEAttackSpec* Spec = FindAttack(CurrentAttackId);
    if (!Spec) { DestroyTelegraph(); return; }

    float Elapsed = ChargeAccumulated;
    int32 Level = -1;
    float DamageScale = 1.f;
    float RangeScale = 1.f;

    if (Spec->Charge.Levels.Num() > 0)
    {
        for (int32 i=0;i<Spec->Charge.Levels.Num();++i)
        {
            if (Elapsed >= Spec->Charge.Levels[i].Time) { Level = i; }
        }
        if (Level >= 0)
        {
            DamageScale = Spec->Charge.Levels[Level].DamageMultiplier;
            RangeScale  = Spec->Charge.Levels[Level].RangeMultiplier;
        }
    }
    ChargeLevelIndex = Level;

    DestroyTelegraph();

    if (bCanceled)
    {
        OnCue.Broadcast(FName("ChargeCancel"), EEECombatCuePhase::End);
        return;
    }

    OnCue.Broadcast(FName("ChargeRelease"), EEECombatCuePhase::Impact);
    ExecuteAttack(*Spec, DamageScale, RangeScale);

    if (Spec->Charge.Shape == EEEChargeShape::Radial)
    {
        FEEAttackSpec Copy = *Spec;
        Copy.AttackType = EEEAttackType::AoE;
        Copy.Radius = Spec->Charge.MaxRadius;
        PerformAoE(Copy, DamageScale, RangeScale);
    }
    else if (Spec->Charge.Shape == EEEChargeShape::FrontalRect)
    {
        PerformFrontalRect(*Spec, DamageScale, RangeScale);
    }
}

/* Telegraph visuals */

void UCombatComponent::SpawnTelegraph()
{
    DestroyTelegraph();
    if (!GetOwner()) return;

    const FEEAttackSpec* Spec = FindAttack(CurrentAttackId);
    if (!Spec || !Spec->Charge.bShowTelegraph) return;

    UMaterialInterface* Mat = nullptr;
    if (Spec->Charge.Shape == EEEChargeShape::Radial) Mat = RadialDecalMaterial;
    else if (Spec->Charge.Shape == EEEChargeShape::FrontalRect) Mat = RectDecalMaterial;

    if (!Mat) return;

    ActiveDecal = NewObject<UDecalComponent>(GetOwner(), UDecalComponent::StaticClass(), NAME_None);
    if (!ActiveDecal) return;

    ActiveDecal->RegisterComponent();
    ActiveDecal->SetDecalMaterial(Mat);
    ActiveDecal->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

    FVector Loc = GetOwner()->GetActorLocation();
    FRotator Rot = GetOwner()->GetActorRotation();
    Rot.Pitch = -90.f;
    ActiveDecal->SetWorldLocationAndRotation(Loc, Rot);
    ActiveDecal->SetFadeScreenSize(0.0001f);
}

void UCombatComponent::UpdateTelegraph(float Alpha)
{
    if (!ActiveDecal) return;
    const FEEAttackSpec* Spec = FindAttack(CurrentAttackId);
    if (!Spec) return;

    Alpha = FMath::Clamp(Alpha, 0.f, 1.f);

    if (Spec->Charge.Shape == EEEChargeShape::Radial)
    {
        const float R = FMath::Lerp(0.f, Spec->Charge.MaxRadius, Alpha);
        ActiveDecal->DecalSize = FVector(1.f, R, R);
    }
    else if (Spec->Charge.Shape == EEEChargeShape::FrontalRect)
    {
        const float L = FMath::Lerp(0.f, Spec->Charge.MaxLength, Alpha);
        const float W = FMath::Lerp(0.f, Spec->Charge.MaxWidth, Alpha);
        ActiveDecal->DecalSize = FVector(1.f, L*0.5f, W*0.5f);
        FVector Fwd = GetOwner()->GetActorForwardVector();
        FVector Loc = GetOwner()->GetActorLocation() + Fwd * (L * 0.5f);
        FRotator Rot = GetOwner()->GetActorRotation(); Rot.Pitch = -90.f;
        ActiveDecal->SetWorldLocationAndRotation(Loc, Rot);
    }
}

void UCombatComponent::DestroyTelegraph()
{
    if (ActiveDecal)
    {
        ActiveDecal->DestroyComponent();
        ActiveDecal = nullptr;
    }
}

/* ================= Combo ================= */

void UCombatComponent::BeginComboWindow(FName ComboId)
{
    bComboWindowOpen = true;
    if (ComboId != NAME_None) ActiveComboId = ComboId;
}

void UCombatComponent::EndComboWindow(FName ComboId)
{
    bComboWindowOpen = false;
    AdvanceComboIfRequested();
}

void UCombatComponent::RequestComboAdvance()
{
    bComboAdvanceRequested = true;
    AdvanceComboIfRequested();
}

void UCombatComponent::AdvanceComboIfRequested()
{
    if (!bComboAdvanceRequested) return;
    bComboAdvanceRequested = false;

    const FEEComboSpec* Combo = nullptr;

    if (ActiveComboId != NAME_None)
    {
        Combo = Combos.FindByPredicate([&](const FEEComboSpec& C){ return C.ComboId == ActiveComboId; });
    }

    if (!Combo)
    {
        Combo = Combos.FindByPredicate([&](const FEEComboSpec& C)
        {
            return C.Steps.Num() > 0 && C.Steps[0].AttackId == CurrentAttackId;
        });
        if (Combo) ActiveComboId = Combo->ComboId;
    }
    if (!Combo || Combo->Steps.Num() == 0) return;

    int32 NextIndex = (ActiveComboStep < 0) ? 0 : ActiveComboStep + 1;
    if (!Combo->Steps.IsValidIndex(NextIndex)) { ActiveComboId = NAME_None; ActiveComboStep = -1; return; }
    ActiveComboStep = NextIndex;

    const FEEComboStep& Step = Combo->Steps[ActiveComboStep];
    const FEEAttackSpec* Spec = FindAttack(Step.AttackId);
    if (!Spec) return;

    FEEAttackSpec Local = *Spec;
    if (Step.DamageOverride > 0.f) Local.BaseDamage = Step.DamageOverride;
    if (Step.CritChanceOverride >= 0.f) Local.CritChance = Step.CritChanceOverride;
    if (Step.CritMultiplierOverride >= 0.f) Local.CritMultiplier = Step.CritMultiplierOverride;

    TryAttackById(Local.AttackId);

    if (UWorld* W = GetWorld())
    {
        ComboResetTime = W->GetTimeSeconds() + Combo->ResetDelay;
    }
}
