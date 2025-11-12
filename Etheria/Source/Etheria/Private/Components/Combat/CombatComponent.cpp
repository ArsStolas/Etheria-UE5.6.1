/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: "CombatComponent" - Source
 */
#include "Components/Combat/CombatComponent.h"
#include "Components/Combat/LockTargetComponent.h"
#include "Data/Weapons/WeaponData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"
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

    // Auto-apply weapon data if provided
    if (WeaponData)
    {
        ApplyWeaponData();
    }
}

bool UCombatComponent::ResolveOwnerRefs()
{
    AActor* O = GetOwner();
    OwnerCharacter = Cast<ACharacter>(O);

    // Prefer the character mesh if available
    if (OwnerCharacter.IsValid())
    {
        OwnerMesh = OwnerCharacter->GetMesh();
        MoveComp  = OwnerCharacter->GetCharacterMovement();
    }
    else if (O)
    {
        // Fallback: any skeletal mesh on the owner
        if (USkeletalMeshComponent* Skel = O->FindComponentByClass<USkeletalMeshComponent>())
        {
            OwnerMesh = Skel;
        }
    }

    return OwnerCharacter.IsValid() || OwnerMesh.IsValid();
}

#pragma region "Set data"
    void UCombatComponent::SetAttacks(const TArray<FEEAttackSpec>& InAttacks) { Attacks = InAttacks; }
    void UCombatComponent::SetCombos(const TArray<FEEComboSpec>& InCombos) { Combos = InCombos; }

    void UCombatComponent::SetWeaponData(UWeaponData* InData)
    {
        WeaponData = InData;
        ApplyWeaponData();
    }

    void UCombatComponent::ApplyWeaponData()
    {
        if (!WeaponData) return;
        Attacks = WeaponData->Attacks;
        Combos  = WeaponData->Combos;

        if (WeaponData->DamageTraceChannelOverride != ECC_MAX)
        {
            DamageTraceChannel = WeaponData->DamageTraceChannelOverride;
        }
        if (WeaponData->bOverrideMagnetism)
        {
            MagnetismAngleDeg   = WeaponData->MagnetismAngleDeg;
            MagnetismStrength   = WeaponData->MagnetismStrength;
        }
    }
#pragma endregion

#pragma region "State"
    bool UCombatComponent::IsInCooldown() const
    {
        UWorld* W = GetWorld();
        return W ? (W->GetTimeSeconds() < CooldownEndTime) : false;
    }
#pragma endregion

bool UCombatComponent::IsJumpBlocked() const { return JumpLocks.Num() > 0; }
bool UCombatComponent::IsCrouchBlocked() const { return CrouchLocks.Num() > 0; }

void UCombatComponent::PushInputLock(FName LockId, bool bBlockJump, bool bBlockCrouch)
{
    if (LockId == NAME_None) return;
    if (bBlockJump)   JumpLocks.Add(LockId);
    if (bBlockCrouch) CrouchLocks.Add(LockId);
}

void UCombatComponent::PopInputLock(FName LockId)
{
    if (LockId == NAME_None) return;
    JumpLocks.Remove(LockId);
    CrouchLocks.Remove(LockId);
}

void UCombatComponent::SetExternalTarget(AActor* InTarget) { ExternalTarget = InTarget; }

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

bool UCombatComponent::TryAttackGroup(FName GroupId)
{
    if (IsInCooldown()) return false;

    const bool bInAir =
        (OwnerCharacter.IsValid() &&
         OwnerCharacter->GetCharacterMovement() &&
         OwnerCharacter->GetCharacterMovement()->IsFalling());

    const FEEAttackSpec* Chosen = nullptr;
    for (const FEEAttackSpec& S : Attacks)
    {
        if (S.Group != GroupId) continue;
        if (S.Stance == EEEStance::AirOnly   && !bInAir) continue;
        if (S.Stance == EEEStance::GroundOnly &&  bInAir) continue;
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

bool UCombatComponent::TryAttackById(FName AttackId)
{
    if (IsInCooldown()) return false;
    const FEEAttackSpec* Spec = FindAttack(AttackId);
    if (!Spec) return false;
    if (!CanExecuteAttack(AttackId)) return false;

    // Combo start cooldown gating (per combo)
    {
        const FEEComboSpec* GateCombo = nullptr;
        for (const FEEComboSpec& C : Combos)
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
        const FEEComboSpec* FoundCombo = nullptr;
        int32 FoundIndex = -1;
        for (const FEEComboSpec& C : Combos)
        {
            for (int32 i=0;i<C.Steps.Num();++i)
            {
                if (C.Steps[i].AttackId == AttackId)
                {
                    FoundCombo = &C; FoundIndex = i; break;
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
        if (OwnerCharacter.IsValid() && Spec->Montage)
        {
            PrePlayMontageSafety(*Spec);

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

void UCombatComponent::PrePlayMontageSafety(const FEEAttackSpec& Spec)
{
    if (!bForceFallbackAnimBPForMontages) return;
    if (!OwnerCharacter.IsValid() || !Spec.Montage) return;

    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (!Mesh) return;

    UAnimInstance* Anim = Mesh->GetAnimInstance();

    // Swap to fallback AnimBP that contains the Slot, if needed
    if (FallbackMontageAnimClass && (!Anim || !Anim->IsA(FallbackMontageAnimClass)))
    {
        SavedAnimClass = Mesh->GetAnimClass();
        Mesh->SetAnimInstanceClass(FallbackMontageAnimClass);
        bUsingFallbackAnimClass = true;

        Anim = Mesh->GetAnimInstance();
    }

    if (Anim)
    {
        Anim->OnMontageEnded.RemoveDynamic(this, &UCombatComponent::HandleMontageEnded_RestoreAnimClass);
        Anim->OnMontageEnded.AddDynamic(this, &UCombatComponent::HandleMontageEnded_RestoreAnimClass);
    }
}

void UCombatComponent::HandleMontageEnded_RestoreAnimClass(UAnimMontage* Montage, bool bInterrupted)
{
    if (!OwnerCharacter.IsValid()) return;
    USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
    if (!Mesh) return;

    if (bUsingFallbackAnimClass && SavedAnimClass)
    {
        Mesh->SetAnimInstanceClass(SavedAnimClass);
    }

    bUsingFallbackAnimClass = false;
    SavedAnimClass = nullptr;
}

void UCombatComponent::PlayOrJumpMontageSection(const FEEAttackSpec& Spec)
{
    if (!OwnerCharacter.IsValid() || !Spec.Montage) return;

    // NEW: ensure a valid Slot by swapping to fallback if needed
    PrePlayMontageSafety(Spec);

    UAnimInstance* AnimInst = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInst) return;

    if (AnimInst->Montage_IsPlaying(Spec.Montage))
    {
        if (Spec.MontageSection != NAME_None)
        {
            AnimInst->Montage_JumpToSection(Spec.MontageSection, Spec.Montage);
            return;
        }
    }
    OwnerCharacter->PlayAnimMontage(Spec.Montage, 1.f, Spec.MontageSection);
}

void UCombatComponent::ExecuteAttack(const FEEAttackSpec& Spec, float DamageScale, float RangeScale)
{
    OnCue.Broadcast(FName("AttackStart"), EEECombatCuePhase::Start);
    OnAttackStarted.Broadcast(Spec.AttackId);

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

void UCombatComponent::OpenWindowWithTimers(const FEEAttackSpec& Spec)
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
    OnCue.Broadcast(FName("AttackEnd"), EEECombatCuePhase::End);

    LastAttackId = CurrentAttackId;
    CurrentAttackId = NAME_None;

    AdvanceComboIfRequested();
}

void UCombatComponent::BeginAttackWindow()
{
    bInAttackWindow = true;
    OnCue.Broadcast(FName("HitWindow"), EEECombatCuePhase::Start);

    const FEEAttackSpec* Spec = FindAttack(CurrentAttackId);
    if (!Spec) return;

    FVector Fwd;
    const FVector Eye = GetEyeLocationForward(Fwd);
    Fwd = ApplyMagnetismBias(Fwd, Eye);
    NudgeOwnerRotationToward(Fwd, MaxAutoYawOnAttackDeg);
    
    ClearRecentHitActors();

    switch (Spec->AttackType)
    {
        case EEEAttackType::Melee:  PerformMeleeTrace(*Spec, 1.f, 1.f);  break;
        case EEEAttackType::AoE:    PerformAoE(*Spec, 1.f, 1.f);        break;
        case EEEAttackType::Ranged: PerformRangedLine(*Spec, 1.f, 1.f); break;
        default: break;
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

void UCombatComponent::PushRecentHitActor(AActor* A)
{
    if (!A) return;
    RecentHitActors.AddUnique(A);
}

void UCombatComponent::ClearRecentHitActors()
{
    RecentHitActors.Reset();
}

void UCombatComponent::GetRecentHitActors(TArray<AActor*>& Out) const
{
    for (const TWeakObjectPtr<AActor>& W : RecentHitActors)
    {
        if (W.IsValid()) Out.Add(W.Get());
    }
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

        if (bCrit) OnHitCrit.Broadcast(Other, FinalDamage); else OnHit.Broadcast(Other, FinalDamage);
        OnCue.Broadcast(FName("Impact"), EEECombatCuePhase::Impact);

        PushRecentHitActor(Other);
    }
}

void UCombatComponent::PerformRangedLine(const FEEAttackSpec& Spec, float DamageScale, float RangeScale)
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

    // Trace against DamageTraceChannel
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

    OnCue.Broadcast(FName("Impact"), EEECombatCuePhase::Impact);
    
    PushRecentHitActor(Other);
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

        if (bCrit) OnHitCrit.Broadcast(Other, FinalDamage); else OnHit.Broadcast(Other, FinalDamage);
        OnCue.Broadcast(FName("Impact"), EEECombatCuePhase::Impact);
        
        PushRecentHitActor(Other);
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

        if (bCrit) OnHitCrit.Broadcast(Other, FinalDamage); else OnHit.Broadcast(Other, FinalDamage);
        OnCue.Broadcast(FName("Impact"), EEECombatCuePhase::Impact);
        
        PushRecentHitActor(Other);
    }

#if !(UE_BUILD_SHIPPING)
    if (bDebugDraw)
    {
        DrawDebugBox(GetWorld(), Center, Extents, FQuat(Fwd.Rotation()), FColor::Orange, false, 1.f, 0, 1.f);
    }
#endif
}

/* ================= Parry / Dodge ================= */
#pragma region "PARRY / DODGE"
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
#pragma endregion

/* ================= Charge ================= */
#pragma region "CHARGE"
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
    ObservedChargeLevel = -1;

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

    const FEEAttackSpec* Spec = FindAttack(CurrentAttackId);
    if (!Spec) return;
    int32 NewObserved = -1;
    for (int32 i=0; i<Spec->Charge.Levels.Num(); ++i)
    {
        if (ChargeAccumulated >= Spec->Charge.Levels[i].Time) NewObserved = i;
    }
    if (NewObserved != ObservedChargeLevel)
    {
        ObservedChargeLevel = NewObserved;
        if (ObservedChargeLevel >= 0)
        {
            const FName Cue = FName(*FString::Printf(TEXT("ChargeLevel_%d"), ObservedChargeLevel + 1));
            OnCue.Broadcast(Cue, EEECombatCuePhase::Start);
        }
    }
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
    float RangeScale  = 1.f;

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

    if (OwnerCharacter.IsValid() && Spec->Montage && ChargeLevelIndex >= 0 && Spec->Charge.ReleaseMontages.Num() == 0)
    {
        if (UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance())
        {
            const FName ReleaseSection = FName(*FString::Printf(TEXT("Release_L%d"), ChargeLevelIndex + 1));
            if (Anim->Montage_IsPlaying(Spec->Montage))
            {
                Anim->Montage_JumpToSection(ReleaseSection, Spec->Montage);
            }
            else
            {
                OwnerCharacter->PlayAnimMontage(Spec->Montage, 1.f, ReleaseSection);
            }
        }
    }
    else
    {
        ExecuteAttack(*Spec, DamageScale, RangeScale);
    }

    if (Spec->Charge.Shape == EEEChargeShape::Radial)
    {
        FEEAttackSpec Copy = *Spec; Copy.AttackType = EEEAttackType::AoE; Copy.Radius = Spec->Charge.MaxRadius;
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
#pragma endregion

/* ================= Combo ================= */
#pragma region "COMBO"
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
    if (UWorld* W = GetWorld())
    {
        ComboBufferExpireAt = W->GetTimeSeconds() + FMath::Max(0.0f, MaxComboBufferTime);
    }
}

void UCombatComponent::AdvanceComboIfRequested()
{
    if (!bComboAdvanceRequested) return;

    if (UWorld* W = GetWorld())
    {
        if (ComboBufferExpireAt > 0.0f && W->GetTimeSeconds() > ComboBufferExpireAt)
        {
            bComboAdvanceRequested = false;
            ComboBufferExpireAt = 0.0f;
            return;
        }
    }

    const FEEComboSpec* Combo = nullptr;

    if (ActiveComboId != NAME_None)
    {
        Combo = Combos.FindByPredicate([&](const FEEComboSpec& C){ return C.ComboId == ActiveComboId; });
    }

    if (!Combo)
    {
        Combo = Combos.FindByPredicate([&](const FEEComboSpec& C)
        {
            return C.Steps.Num() > 0 && C.Steps[0].AttackId == LastAttackId;
        });
        if (Combo) ActiveComboId = Combo->ComboId;
    }
    if (!Combo || Combo->Steps.Num() == 0) { bComboAdvanceRequested = false; return; }

    // Compute next index based on our known ActiveComboStep (seeded at TryAttackById)
    int32 NextIndex = (ActiveComboStep < 0) ? 0 : ActiveComboStep + 1;
    if (!Combo->Steps.IsValidIndex(NextIndex))
    {
        // End of combo
        ActiveComboId = NAME_None;
        ActiveComboStep = -1;
        bComboAdvanceRequested = false;
        ComboBufferExpireAt = 0.0f;
        return;
    }
    ActiveComboStep = NextIndex;

    const FEEComboStep& Step = Combo->Steps[ActiveComboStep];
    const FEEAttackSpec* Spec = FindAttack(Step.AttackId);
    if (!Spec) { bComboAdvanceRequested = false; return; }

    FEEAttackSpec Local = *Spec;
    if (Step.DamageOverride > 0.f)        Local.BaseDamage = Step.DamageOverride;
    if (Step.CritChanceOverride >= 0.f)   Local.CritChance = Step.CritChanceOverride;
    if (Step.CritMultiplierOverride >= 0.f) Local.CritMultiplier = Step.CritMultiplierOverride;

    // Trigger next step by id. PlayOrJumpMontageSection guarantees no "restart" of first section.
    TryAttackById(Local.AttackId);

    if (UWorld* W = GetWorld())
    {
        ComboResetTime = W->GetTimeSeconds() + Combo->ResetDelay;
    }

    bComboAdvanceRequested = false;
    ComboBufferExpireAt = 0.0f;
}
#pragma endregion

float UCombatComponent::GetComboCooldownRemaining(FName ComboId) const
{
    if (!GetWorld()) return 0.f;
    if (const float* Until = ComboCooldownUntil.Find(ComboId))
    {
        const float Now = GetWorld()->GetTimeSeconds();
        return FMath::Max(0.f, *Until - Now);
    }
    return 0.f;
}

void UCombatComponent::ClearAllComboCooldowns()
{
    ComboCooldownUntil.Reset();
}
