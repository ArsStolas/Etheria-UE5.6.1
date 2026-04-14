/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AIAnimationComponent - Source"
 * Notes: Two playback modes:
 *   DirectPlayback — SkeletalMesh::PlayAnimation(Montage). No ABP. Works for creatures.
 *   AnimBlueprint  — AnimInstance::Montage_Play(). Requires ABP with DefaultSlot. For humanoids.
 *   Idle variations play to completion. Actions pause locomotion with crossfade.
 */

#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Characters/AI/BaseAICharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

UAIAnimationComponent::UAIAnimationComponent() { PrimaryComponentTick.bCanEverTick = true; }

void UAIAnimationComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ABaseAICharacter>(GetOwner());
	if (!OwnerCharacter) return;

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh) return;

	if (AnimationMode == EAIAnimationMode::DirectPlayback)
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		if (IdleBaseMontage) { PlayOnMesh(IdleBaseMontage, true); CurrentLocomotionMontage = IdleBaseMontage; }
	}
	else
	{
		// ABP mode — bind montage end callback
		if (UAnimInstance* Anim = Mesh->GetAnimInstance())
		{
			Anim->OnMontageEnded.AddDynamic(this, &UAIAnimationComponent::HandleMontageEnded);
			bMontageCallbackBound = true;
		}
	}
}

void UAIAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// In DirectPlayback mode, we track action end via timer
	if (AnimationMode == EAIAnimationMode::DirectPlayback && bIsPlayingAction)
	{
		ActionTimer -= DeltaTime;
		if (ActionTimer <= 0.f)
		{
			UAnimMontage* Finished = CurrentActionMontage;
			bIsPlayingAction = false;
			bLocomotionPaused = false;
			bIsPlayingIdleVariation = false;
			CurrentActionMontage = nullptr;
			ActionTimer = 0.f;
			OnAIAnimEnded.Broadcast(Finished);
			CurrentLocomotionMontage = nullptr;
			UpdateLocomotion();
		}
		return;
	}

	// In ABP mode, the HandleMontageEnded callback handles action end

	// Don't update locomotion if paused or playing an idle variation
	if (!bLocomotionPaused && !bIsPlayingIdleVariation)
	{
		UpdateLocomotion();
	}
}

/* ═══════════ Locomotion ═══════════ */

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

	if (T && T != CurrentLocomotionMontage)
	{
		if (AnimationMode == EAIAnimationMode::DirectPlayback)
		{
			PlayOnMesh(T, true);
		}
		else
		{
			PlayViaMontageSystem(T);
		}
		CurrentLocomotionMontage = T;
	}

	OnLocomotionStateChanged.Broadcast();
}

/* ═══════════ Action Montages ═══════════ */

UAnimMontage* UAIAnimationComponent::PlayRandomIdle()
{
	if (IdleVariations.Num() == 0) return nullptr;
	UAnimMontage* M = IdleVariations[FMath::RandRange(0, IdleVariations.Num() - 1)];
	if (!M) return nullptr;

	UAnimMontage* Result = PlayActionMontage(M, IdlePlayRate);
	if (Result) bIsPlayingIdleVariation = true;
	return Result;
}

UAnimMontage* UAIAnimationComponent::PlayIdleByIndex(int32 I)
{
	if (!IdleVariations.IsValidIndex(I)) return nullptr;
	UAnimMontage* Result = PlayActionMontage(IdleVariations[I], IdlePlayRate);
	if (Result) bIsPlayingIdleVariation = true;
	return Result;
}

UAnimMontage* UAIAnimationComponent::PlayRandomAttack()
{
	if (AttackMontages.Num() == 0) return nullptr;
	return PlayActionMontage(AttackMontages[FMath::RandRange(0, AttackMontages.Num() - 1)]);
}

UAnimMontage* UAIAnimationComponent::PlayAttackByIndex(int32 I)
{
	if (!AttackMontages.IsValidIndex(I)) return nullptr;
	return PlayActionMontage(AttackMontages[I]);
}

UAnimMontage* UAIAnimationComponent::PlayInteraction() { return PlayActionMontage(InteractionMontage); }

UAnimMontage* UAIAnimationComponent::PlayHitReaction() { return PlayRandomHitReaction(); }

UAnimMontage* UAIAnimationComponent::PlayRandomHitReaction()
{
	if (HitReactionMontages.Num() == 0) return nullptr;
	// Hit reactions can interrupt idle variations and other actions
	StopCurrentAction();
	return PlayActionMontage(HitReactionMontages[FMath::RandRange(0, HitReactionMontages.Num() - 1)]);
}

UAnimMontage* UAIAnimationComponent::PlayDeath() { return PlayRandomDeath(); }

UAnimMontage* UAIAnimationComponent::PlayRandomDeath()
{
	if (DeathMontages.Num() == 0) return nullptr;
	UAnimMontage* M = DeathMontages[FMath::RandRange(0, DeathMontages.Num() - 1)];
	if (!M) return nullptr;

	if (AnimationMode == EAIAnimationMode::DirectPlayback)
	{
		PlayOnMesh(M, false);
	}
	else
	{
		PlayViaMontageSystem(M);
	}

	bIsPlayingAction = true;
	bLocomotionPaused = true;
	CurrentActionMontage = M;
	ActionTimer = 999.f; // Death never auto-resumes
	OnAIAnimStarted.Broadcast(M);
	return M;
}

UAnimMontage* UAIAnimationComponent::PlayActionMontage(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage || !OwnerCharacter) return nullptr;

	if (AnimationMode == EAIAnimationMode::DirectPlayback)
	{
		PlayOnMesh(Montage, false, PlayRate);
		bIsPlayingAction = true;
		bLocomotionPaused = true;
		CurrentActionMontage = Montage;
		ActionTimer = Montage->GetPlayLength() / FMath::Max(PlayRate, 0.01f);
	}
	else
	{
		// ABP mode — stop current locomotion montage, play action via Montage_Play
		if (UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance())
		{
			if (CurrentLocomotionMontage)
			{
				Anim->Montage_Stop(ActionBlendTime, CurrentLocomotionMontage);
				CurrentLocomotionMontage = nullptr;
			}

			const float Duration = Anim->Montage_Play(Montage, PlayRate);
			if (Duration <= 0.f) return nullptr;

			bIsPlayingAction = true;
			bLocomotionPaused = true;
			CurrentActionMontage = Montage;
			// In ABP mode, HandleMontageEnded handles the end — no timer needed
		}
	}

	OnAIAnimStarted.Broadcast(Montage);
	return Montage;
}

void UAIAnimationComponent::StopCurrentAction()
{
	if (!bIsPlayingAction) return;

	UAnimMontage* Stopped = CurrentActionMontage;

	if (AnimationMode == EAIAnimationMode::AnimBlueprint && OwnerCharacter)
	{
		if (UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance())
		{
			if (CurrentActionMontage)
				Anim->Montage_Stop(ActionBlendTime, CurrentActionMontage);
		}
	}

	bIsPlayingAction = false;
	bLocomotionPaused = false;
	bIsPlayingIdleVariation = false;
	CurrentActionMontage = nullptr;
	ActionTimer = 0.f;
	OnAIAnimEnded.Broadcast(Stopped);

	CurrentLocomotionMontage = nullptr;
	UpdateLocomotion();
}

/* ═══════════ ABP Mode Callback ═══════════ */

void UAIAnimationComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (AnimationMode != EAIAnimationMode::AnimBlueprint) return;

	if (Montage == CurrentActionMontage)
	{
		UAnimMontage* Finished = CurrentActionMontage;
		bIsPlayingAction = false;
		bLocomotionPaused = false;
		bIsPlayingIdleVariation = false;
		CurrentActionMontage = nullptr;
		OnAIAnimEnded.Broadcast(Finished);

		CurrentLocomotionMontage = nullptr;
		UpdateLocomotion();
	}
}

/* ═══════════ Internal Playback ═══════════ */

void UAIAnimationComponent::PlayOnMesh(UAnimMontage* Montage, bool bLoop, float PlayRate)
{
	if (!Montage || !OwnerCharacter) return;
	if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
	{
		Mesh->PlayAnimation(Montage, bLoop);
		Mesh->SetPlayRate(PlayRate);
	}
}

void UAIAnimationComponent::PlayViaMontageSystem(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage || !OwnerCharacter) return;
	if (UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		Anim->Montage_Play(Montage, PlayRate, EMontagePlayReturnType::MontageLength, 0.f, true);
	}
}
