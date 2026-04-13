/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AIAnimationComponent - Source"
 * Notes: PlayAnimation(Montage) works without ABP because UAnimMontage is a UAnimationAsset.
 *        Mesh MUST be in AnimationSingleNode mode. Notifies fire normally.
 */

#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Characters/AI/BaseAICharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"

UAIAnimationComponent::UAIAnimationComponent() { PrimaryComponentTick.bCanEverTick = true; }

void UAIAnimationComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ABaseAICharacter>(GetOwner());
	if (OwnerCharacter)
	{
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		{
			Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			if (IdleBaseMontage) { PlayOnMesh(IdleBaseMontage, true); CurrentLocomotionMontage = IdleBaseMontage; }
		}
	}
}

void UAIAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bIsPlayingAction)
	{
		ActionTimer -= DeltaTime;
		if (ActionTimer <= 0.f)
		{
			UAnimMontage* Finished = CurrentActionMontage;
			bIsPlayingAction = false; bLocomotionPaused = false;
			CurrentActionMontage = nullptr; ActionTimer = 0.f;
			OnAIAnimEnded.Broadcast(Finished);
			CurrentLocomotionMontage = nullptr;
			UpdateLocomotion();
		}
		return;
	}
	if (!bLocomotionPaused) UpdateLocomotion();
}

void UAIAnimationComponent::UpdateLocomotion()
{
	if (!OwnerCharacter) return;
	const UCharacterMovementComponent* MC = OwnerCharacter->GetCharacterMovement();
	if (!MC) return;

	if (MC->IsFalling()) { SetLocomotionState(EAILocomotionState::Falling); return; }

	const FVector Vel = MC->Velocity;
	const float Speed = Vel.Size2D();
	if (Speed < IdleSpeedThreshold) { SetLocomotionState(EAILocomotionState::Idle); return; }

	const bool bFwd = FVector::DotProduct(OwnerCharacter->GetActorForwardVector(), Vel.GetSafeNormal2D()) >= 0.f;
	if (Speed >= RunSpeedThreshold)
		SetLocomotionState(bFwd ? EAILocomotionState::RunForward : EAILocomotionState::RunBackward);
	else
		SetLocomotionState(bFwd ? EAILocomotionState::WalkForward : EAILocomotionState::WalkBackward);
}

void UAIAnimationComponent::SetLocomotionState(EAILocomotionState NewState)
{
	if (CurrentLocomotionState == NewState && CurrentLocomotionMontage) return;
	CurrentLocomotionState = NewState;

	UAnimMontage* T = nullptr;
	switch (NewState)
	{
	case EAILocomotionState::Idle:         T = IdleBaseMontage; break;
	case EAILocomotionState::WalkForward:  T = WalkForwardMontage; break;
	case EAILocomotionState::WalkBackward: T = WalkBackwardMontage ? WalkBackwardMontage.Get() : WalkForwardMontage.Get(); break;
	case EAILocomotionState::RunForward:   T = RunForwardMontage; break;
	case EAILocomotionState::RunBackward:  T = RunBackwardMontage ? RunBackwardMontage.Get() : RunForwardMontage.Get(); break;
	case EAILocomotionState::Falling:      T = FallingMontage; break;
	case EAILocomotionState::Landing:      T = LandingMontage; break;
	}

	if (T && T != CurrentLocomotionMontage) { PlayOnMesh(T, true); CurrentLocomotionMontage = T; }
	OnLocomotionStateChanged.Broadcast();
}

UAnimMontage* UAIAnimationComponent::PlayRandomIdle()      { if (IdleVariations.Num() == 0) return nullptr; return PlayActionMontage(IdleVariations[FMath::RandRange(0, IdleVariations.Num()-1)], IdlePlayRate); }
UAnimMontage* UAIAnimationComponent::PlayIdleByIndex(int32 I)  { if (!IdleVariations.IsValidIndex(I)) return nullptr; return PlayActionMontage(IdleVariations[I], IdlePlayRate); }
UAnimMontage* UAIAnimationComponent::PlayRandomAttack()    { if (AttackMontages.Num() == 0) return nullptr; return PlayActionMontage(AttackMontages[FMath::RandRange(0, AttackMontages.Num()-1)]); }
UAnimMontage* UAIAnimationComponent::PlayAttackByIndex(int32 I) { if (!AttackMontages.IsValidIndex(I)) return nullptr; return PlayActionMontage(AttackMontages[I]); }
UAnimMontage* UAIAnimationComponent::PlayInteraction()     { return PlayActionMontage(InteractionMontage); }
UAnimMontage* UAIAnimationComponent::PlayHitReaction()     { return PlayRandomHitReaction(); }
UAnimMontage* UAIAnimationComponent::PlayRandomHitReaction() { if (HitReactionMontages.Num() == 0) return nullptr; return PlayActionMontage(HitReactionMontages[FMath::RandRange(0, HitReactionMontages.Num()-1)]); }
UAnimMontage* UAIAnimationComponent::PlayDeath()           { return PlayRandomDeath(); }

UAnimMontage* UAIAnimationComponent::PlayRandomDeath()
{
	if (DeathMontages.Num() == 0) return nullptr;
	UAnimMontage* M = DeathMontages[FMath::RandRange(0, DeathMontages.Num()-1)];
	if (M) { PlayOnMesh(M, false); bIsPlayingAction = true; bLocomotionPaused = true; CurrentActionMontage = M; ActionTimer = 999.f; OnAIAnimStarted.Broadcast(M); }
	return M;
}

UAnimMontage* UAIAnimationComponent::PlayActionMontage(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage || !OwnerCharacter) return nullptr;
	PlayOnMesh(Montage, false, PlayRate);
	bIsPlayingAction = true; bLocomotionPaused = true;
	CurrentActionMontage = Montage;
	ActionTimer = Montage->GetPlayLength() / FMath::Max(PlayRate, 0.01f);
	OnAIAnimStarted.Broadcast(Montage);
	return Montage;
}

void UAIAnimationComponent::StopCurrentAction()
{
	if (!bIsPlayingAction) return;
	UAnimMontage* S = CurrentActionMontage;
	bIsPlayingAction = false; bLocomotionPaused = false;
	CurrentActionMontage = nullptr; ActionTimer = 0.f;
	OnAIAnimEnded.Broadcast(S);
	CurrentLocomotionMontage = nullptr; UpdateLocomotion();
}

void UAIAnimationComponent::PlayOnMesh(UAnimMontage* Montage, bool bLoop, float PlayRate)
{
	if (!Montage || !OwnerCharacter) return;
	if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
	{
		Mesh->PlayAnimation(Montage, bLoop);
		Mesh->SetPlayRate(PlayRate);
	}
}
