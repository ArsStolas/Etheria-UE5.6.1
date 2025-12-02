/*
* Etheria's End Project, 2025
* Created by: ArsStolas
* Last Updated by: ArsStolas
* Class: BaseEnemy - Source
*/

#include "Characters/AI/Enemies/BaseEnemy.h"
#include "Components/Combat/CombatComponent.h"
#include "AIController.h"
#include "Perception/AISense_Sight.h"
#include "Characters/BaseCharacter.h"
#include "Characters/Players/PlayerCharacter.h"
#include "TimerManager.h"

ABaseEnemy::ABaseEnemy() { bIsHostile = true; }

void ABaseEnemy::BeginPlay()
{
    Super::BeginPlay();
    bIsAttacking = false;
    ResetComboState();

    if (CombatComponent)
    {
        CombatComponent->OnAttackEnded.AddDynamic(this, &ABaseEnemy::OnComboEndHandler);
        // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] Registered OnAttackEnded delegate"));
    }
    // else
        // UE_LOG(LogTemp, Error, TEXT("[ENEMY] CombatComponent is NULL!"));
}

bool ABaseEnemy::CanIdleMove() const { return CurrentState == EEnemyState::Idle; }

void ABaseEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bUseBehaviorTree)
    {
        float AttackRange = 0.f;
        if (CombatComponent && CombatComponent->IsValidLowLevel())
        {
            const TArray<FAttackSpecConfig>& Attacks = CombatComponent->GetAttacks();
            if (Attacks.Num() > 0)
                AttackRange = Attacks[0].Range;
        }

        if ((CurrentState == EEnemyState::Fighting) && TargetActor)
        {
            float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
            if (Dist > AttackRange)
            {
                // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] Target too far - Stopping attacks, switch to Chase"));
                CurrentState = EEnemyState::Chase;
                StopAttackCycle();
                StartChasePlayer();
                ResetComboState();
                return;
            }
        }

        switch (CurrentState)
        {
        case EEnemyState::Idle: break;
        case EEnemyState::Chase:
            if (TargetActor && AttackRange > 0.f)
            {
                float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
                if (Dist < AttackRange)
                {
                    CurrentState = EEnemyState::Fighting;
                    StartAttackCycle();
                }
            }
            break;
        default: ;
        }
    }
}

void ABaseEnemy::HandlePerception()
{
    if (!AIPerception) return;
    TArray<AActor*> PerceivedActors;
    AIPerception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

    bool bSawPlayer = false;
    for (AActor* Actor : PerceivedActors)
    {
        if (Actor->IsA(APlayerCharacter::StaticClass()))
        {
            TargetActor = Actor;
            bSawPlayer = true;
            break;
        }
    }
    if (bSawPlayer)
    {
        if (CurrentState != EEnemyState::Chase)
        {
            CurrentState = EEnemyState::Chase;
            StartChasePlayer();
        }
    }
    else
    {
        TargetActor = nullptr;
        StopAttackCycle();
        CurrentState = EEnemyState::Idle;
        ResetComboState();
    }
}

void ABaseEnemy::StartChasePlayer()
{
    if (AAIController* AICon = Cast<AAIController>(GetController()))
    {
        AICon->StopMovement();
        if (TargetActor)
            AICon->MoveToActor(TargetActor);
    }
}

void ABaseEnemy::StartAttackCycle()
{
    if (bIsAttacking) return;
    bIsAttacking = true;
    ResetComboState();

    SingleAttackId = NAME_None;
    SelectedComboId = NAME_None;

    if (!CombatComponent) { StopAttackCycle(); return; }

    const TArray<FComboSpecConfig>& Combos = CombatComponent->GetCombos();
    // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] StartAttackCycle: BP combo count: %d"), Combos.Num());

    TArray<const FComboSpecConfig*> ValidCombos;
    for (const FComboSpecConfig& Combo : Combos)
    {
        // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] Combo BP found: %s (steps: %d)"), *Combo.ComboId.ToString(), Combo.Steps.Num());
        if (Combo.Steps.Num() > 0) ValidCombos.Add(&Combo);
    }

    float DoComboRand = FMath::FRand();
    // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] Combo roll value: %.2f"), DoComboRand);
    const float DoComboChance = 0.3f;
    const bool bShouldDoCombo = (DoComboRand < DoComboChance) && ValidCombos.Num() > 0;
    bComboInProgress = bShouldDoCombo;
    ComboStep = 0;

    if (bShouldDoCombo)
    {
        int32 ComboIdx = FMath::RandRange(0, ValidCombos.Num()-1);
        const FComboSpecConfig* ChosenCombo = ValidCombos[ComboIdx];
        SelectedComboId = ChosenCombo->ComboId;
        ComboLength = ChosenCombo->Steps.Num();
        // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] Choose RANDOM COMBO '%s' (steps: %d)"), *SelectedComboId.ToString(), ComboLength);
    }
    else if (ValidCombos.Num() > 0 && ValidCombos[0]->Steps.Num() > 0)
    {
        ComboLength = 1;
        SelectedComboId = NAME_None;
        SingleAttackId = ValidCombos[0]->Steps[0].AttackId;
        // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] Will perform ONLY first attack of first combo: '%s' (70%% single attack pattern)"), *SingleAttackId.ToString());
    }
    else
    {
        StopAttackCycle();
        return;
    }

    AttackEnemy();
}

void ABaseEnemy::AttackEnemy()
{
    if (!bIsAttacking || !CombatComponent || !TargetActor) return;

    float AttackRange = 0.f;
    if (CombatComponent && CombatComponent->IsValidLowLevel())
    {
        const TArray<FAttackSpecConfig>& Attacks = CombatComponent->GetAttacks();
        if (Attacks.Num() > 0)
            AttackRange = Attacks[0].Range;
    }
    float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
    if (Dist > AttackRange)
    {
        // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] AttackEnemy interrupted: Target out of range (combo/single), switch to Chase"));
        CurrentState = EEnemyState::Chase;
        StopAttackCycle();
        StartChasePlayer();
        ResetComboState();
        return;
    }

    if (CombatComponent->IsInCooldown())
    {
        GetWorld()->GetTimerManager().SetTimer(
            EnemyAttackTimerHandle, this, &ABaseEnemy::AttackEnemy, 0.05f, false);
        return;
    }

    if (bComboInProgress && SelectedComboId != NAME_None)
    {
        const TArray<FComboSpecConfig>& Combos = CombatComponent->GetCombos();
        const FComboSpecConfig* Combo = Combos.FindByPredicate([this](const FComboSpecConfig& CS){return CS.ComboId==SelectedComboId;});
        if (!Combo || Combo->Steps.Num() == 0)
        {
            StopAttackCycle();
            return;
        }

        FName AttackIdToPlay = Combo->Steps.IsValidIndex(ComboStep) ? Combo->Steps[ComboStep].AttackId : NAME_None;
        if (AttackIdToPlay == NAME_None)
        {
            StopAttackCycle();
            return;
        }

        FVector ToTarget = (TargetActor->GetActorLocation() - GetActorLocation()); ToTarget.Z = 0;
        SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
        CombatComponent->SetExternalTarget(TargetActor);
        bool DidAttack = CombatComponent->TryAttackById(AttackIdToPlay);
        // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] Combo '%s' - TryAttackById '%s': %s - ComboStep %d/%d"),
        //     *SelectedComboId.ToString(), *AttackIdToPlay.ToString(), DidAttack?TEXT("YES"):TEXT("NO"), ComboStep+1, ComboLength);
    }
    else if (!bComboInProgress && SingleAttackId != NAME_None)
    {
        FVector ToTarget = (TargetActor->GetActorLocation() - GetActorLocation()); ToTarget.Z = 0;
        SetActorRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
        CombatComponent->SetExternalTarget(TargetActor);
        bool DidAttack = CombatComponent->TryAttackById(SingleAttackId);
        // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] TryAttackById Single/FirstComboStep '%s': %s"),
        //     *SingleAttackId.ToString(), DidAttack?TEXT("YES"):TEXT("NO"));
        float Delay = 0.5f;
        const TArray<FAttackSpecConfig>& Attacks = CombatComponent->GetAttacks();
        if (Attacks.Num() > 0) Delay = FMath::Max(Attacks[0].Cooldown, 0.5f);
        GetWorld()->GetTimerManager().SetTimer(EnemyAttackTimerHandle, [this]() {
            StopAttackCycle(); StartAttackCycle();
        }, Delay, false);
    }
    else { StopAttackCycle(); }
}

void ABaseEnemy::OnComboEndHandler(FName AttackId)
{
    // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] OnComboEndHandler entered - bIsAttacking:%d bCombo:%d ComboStep:%d/%d"),
    //     bIsAttacking, bComboInProgress, ComboStep, ComboLength);
    if (!bIsAttacking || !bComboInProgress) return;

    ComboStep++;
    if (ComboStep < ComboLength)
    {
        GetWorld()->GetTimerManager().SetTimer(
            EnemyAttackTimerHandle, this, &ABaseEnemy::TryComboAttackStep, 0.05f, true);
    }
    else
    {
        bComboInProgress = false; ComboStep = 0; StopAttackCycle(); StartAttackCycle();
    }
}

void ABaseEnemy::TryComboAttackStep()
{
    if (!bIsAttacking || !bComboInProgress)
    {
        GetWorld()->GetTimerManager().ClearTimer(EnemyAttackTimerHandle);
        return;
    }

    float AttackRange = 0.f;
    if (CombatComponent && CombatComponent->IsValidLowLevel())
    {
        const TArray<FAttackSpecConfig>& Attacks = CombatComponent->GetAttacks();
        if (Attacks.Num() > 0)
            AttackRange = Attacks[0].Range;
    }
    if (TargetActor && FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation()) > AttackRange)
    {
        // UE_LOG(LogTemp, Warning, TEXT("[ENEMY] TryComboAttackStep interrupted: Target out of range, switch to Chase"));
        GetWorld()->GetTimerManager().ClearTimer(EnemyAttackTimerHandle);
        CurrentState = EEnemyState::Chase;
        StopAttackCycle();
        StartChasePlayer();
        ResetComboState();
        return;
    }

    if (CombatComponent && CombatComponent->IsInCooldown()) return;
    GetWorld()->GetTimerManager().ClearTimer(EnemyAttackTimerHandle);
    if (ComboStep < ComboLength)
    {
        AttackEnemy();
    }
    else
    {
        bComboInProgress = false; ComboStep = 0; StopAttackCycle(); StartAttackCycle();
    }
}

void ABaseEnemy::StopAttackCycle()
{
    bIsAttacking = false;
    GetWorld()->GetTimerManager().ClearTimer(EnemyAttackTimerHandle);
    ResetComboState();
}

void ABaseEnemy::ResetComboState()
{
    bComboInProgress = false;
    ComboStep = 0;
    ComboLength = 0;
    SingleAttackId = NAME_None;
    SelectedComboId = NAME_None;
}
