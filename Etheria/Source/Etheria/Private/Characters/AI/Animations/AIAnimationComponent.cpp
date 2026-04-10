/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Class: AIAnimationComponent - Source
 */

#include "Characters/AI/Animations/AIAnimationComponent.h"

#include "Characters/AI/BaseAICharacter.h"
#include "Animation/AnimInstance.h"

UAIAnimationComponent::UAIAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAIAnimationComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ABaseAICharacter>(GetOwner());

	if (OwnerCharacter)
	{
		if (UAnimInstance* AnimInst = OwnerCharacter->GetMesh()->GetAnimInstance())
		{
			AnimInst->OnMontageEnded.AddDynamic(this, &UAIAnimationComponent::HandleMontageEnded);
		}
	}
}

/* ─────────────────── Play API ─────────────────── */

UAnimMontage* UAIAnimationComponent::PlayRandomIdle()
{
	if (IdleMontages.Num() == 0) return nullptr;
	const int32 Index = FMath::RandRange(0, IdleMontages.Num() - 1);
	return PlayMontageInternal(IdleMontages[Index], IdlePlayRate);
}

UAnimMontage* UAIAnimationComponent::PlayIdleByIndex(int32 Index)
{
	if (!IdleMontages.IsValidIndex(Index)) return nullptr;
	return PlayMontageInternal(IdleMontages[Index], IdlePlayRate);
}

UAnimMontage* UAIAnimationComponent::PlayMovement()
{
	return PlayMontageInternal(MovementMontage);
}

UAnimMontage* UAIAnimationComponent::PlayRandomAttack()
{
	if (AttackMontages.Num() == 0) return nullptr;
	const int32 Index = FMath::RandRange(0, AttackMontages.Num() - 1);
	return PlayMontageInternal(AttackMontages[Index]);
}

UAnimMontage* UAIAnimationComponent::PlayAttackByIndex(int32 Index)
{
	if (!AttackMontages.IsValidIndex(Index)) return nullptr;
	return PlayMontageInternal(AttackMontages[Index]);
}

UAnimMontage* UAIAnimationComponent::PlayInteraction()
{
	return PlayMontageInternal(InteractionMontage);
}

UAnimMontage* UAIAnimationComponent::PlayHitReaction()
{
	return PlayMontageInternal(HitReactionMontage);
}

UAnimMontage* UAIAnimationComponent::PlayDeath()
{
	return PlayMontageInternal(DeathMontage);
}

void UAIAnimationComponent::StopCurrentMontage(float BlendOut)
{
	if (!OwnerCharacter) return;

	if (UAnimInstance* AnimInst = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		AnimInst->StopAllMontages(BlendOut);
	}
	CurrentMontage = nullptr;
}

bool UAIAnimationComponent::IsPlayingMontage() const
{
	if (!OwnerCharacter) return false;
	if (const UAnimInstance* AnimInst = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		return AnimInst->IsAnyMontagePlaying();
	}
	return false;
}

/* ─────────────────── Internal ─────────────────── */

UAnimMontage* UAIAnimationComponent::PlayMontageInternal(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage || !OwnerCharacter) return nullptr;

	if (UAnimInstance* AnimInst = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		const float Duration = AnimInst->Montage_Play(Montage, PlayRate);
		if (Duration > 0.f)
		{
			CurrentMontage = Montage;
			OnAIMontageStarted.Broadcast(Montage);
			return Montage;
		}
	}
	return nullptr;
}

void UAIAnimationComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == CurrentMontage)
	{
		OnAIMontageEnded.Broadcast(Montage);
		CurrentMontage = nullptr;
	}
}
