/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "AIAnimationComponent - Source"
 * Notes:
 *   DirectPlayback — Mesh::PlayAnimation(Montage). Action timer tracks end.
 *   AnimBlueprint  — Feeds GroundSpeed/Direction/Velocity/FallSpeed to ABP every tick.
 *                    ABP locomotion untouched. Montage_Play for actions. Callback tracks end.
 */

#include "Characters/AI/Animations/AIAnimationComponent.h"
#include "Characters/AI/BaseAICharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "KismetAnimationLibrary.h"

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
		// Direct mode — force SingleNode, no ABP
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		if (IdleBaseMontage) { PlayOnMesh(IdleBaseMontage, true); CurrentLocomotionMontage = IdleBaseMontage; }
	}
	else
	{
		// ABP mode — leave animation mode alone, bind montage end callback
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

	if (AnimationMode == EAIAnimationMode::AnimBlueprint)
	{
		// Feed ABP variables every frame
		UpdateABPVariables();

		// ABP mode uses HandleMontageEnded callback — no timer tracking needed
		// But we still need bIsPlayingAction state for external queries
	}
	else // DirectPlayback
	{
		if (bIsPlayingAction)
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
				UpdateLocomotionDirect();
			}
			return;
		}

		if (!bLocomotionPaused && !bIsPlayingIdleVariation)
			UpdateLocomotionDirect();
	}
}

/* ═══════════ ABP Variable Feeding ═══════════ */

void UAIAnimationComponent::UpdateABPVariables()
{
	if (!OwnerCharacter) return;
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh) return;
	UAnimInstance* Anim = Mesh->GetAnimInstance();
	if (!Anim) return;

	const UCharacterMovementComponent* MC = OwnerCharacter->GetCharacterMovement();
	if (!MC) return;

	const FVector Vel = MC->Velocity;

	SetABPVector(Anim, ABP_VelocityName, Vel);
	SetABPFloat(Anim, ABP_GroundSpeedName, Vel.Size2D());
	SetABPFloat(Anim, ABP_FallSpeedName, Vel.Z);
	SetABPFloat(Anim, ABP_DirectionName,
		UKismetAnimationLibrary::CalculateDirection(Vel, OwnerCharacter->GetActorRotation()));
}

void UAIAnimationComponent::SetABPFloat(UAnimInstance* Anim, FName Name, float Value)
{
	if (Name == NAME_None) return;
	if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(Anim->GetClass(), Name))
	{
		Prop->SetPropertyValue_InContainer(Anim, Value);
		return;
	}
	// UE5 might use double
	if (FDoubleProperty* Prop = FindFProperty<FDoubleProperty>(Anim->GetClass(), Name))
	{
		Prop->SetPropertyValue_InContainer(Anim, static_cast<double>(Value));
	}
}

void UAIAnimationComponent::SetABPVector(UAnimInstance* Anim, FName Name, const FVector& Value)
{
	if (Name == NAME_None) return;
	if (FStructProperty* Prop = FindFProperty<FStructProperty>(Anim->GetClass(), Name))
	{
		if (Prop->Struct == TBaseStructure<FVector>::Get())
		{
			void* Ptr = Prop->ContainerPtrToValuePtr<void>(Anim);
			*static_cast<FVector*>(Ptr) = Value;
		}
	}
}

/* ═══════════ Direct Mode Locomotion ═══════════ */

void UAIAnimationComponent::UpdateLocomotionDirect()
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
	if (AnimationMode == EAIAnimationMode::AnimBlueprint) return; // ABP handles locomotion

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

/* ═══════════ Action Montages ═══════════ */

UAnimMontage* UAIAnimationComponent::PlayRandomIdle()
{
	if (IdleVariations.Num() == 0) return nullptr;
	UAnimMontage* R = PlayActionMontage(IdleVariations[FMath::RandRange(0, IdleVariations.Num() - 1)], IdlePlayRate);
	if (R) bIsPlayingIdleVariation = true;
	return R;
}

UAnimMontage* UAIAnimationComponent::PlayIdleByIndex(int32 I)
{
	if (!IdleVariations.IsValidIndex(I)) return nullptr;
	UAnimMontage* R = PlayActionMontage(IdleVariations[I], IdlePlayRate);
	if (R) bIsPlayingIdleVariation = true;
	return R;
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
	StopCurrentAction(); // Hit interrupts everything
	return PlayActionMontage(HitReactionMontages[FMath::RandRange(0, HitReactionMontages.Num() - 1)]);
}

UAnimMontage* UAIAnimationComponent::PlayDeath() { return PlayRandomDeath(); }

UAnimMontage* UAIAnimationComponent::PlayRandomDeath()
{
	if (DeathMontages.Num() == 0) return nullptr;
	UAnimMontage* M = DeathMontages[FMath::RandRange(0, DeathMontages.Num() - 1)];
	if (!M) return nullptr;

	if (AnimationMode == EAIAnimationMode::DirectPlayback)
		PlayOnMesh(M, false);
	else
		PlayViaMontageSystem(M);

	bIsPlayingAction = true;
	bLocomotionPaused = true;
	CurrentActionMontage = M;
	ActionTimer = 999.f;
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
		UAnimMontage* Played = PlayViaMontageSystem(Montage, PlayRate);
		if (!Played) return nullptr;
		bIsPlayingAction = true;
		bLocomotionPaused = true;
		CurrentActionMontage = Montage;
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
			if (CurrentActionMontage) Anim->Montage_Stop(ActionBlendTime, CurrentActionMontage);
	}

	bIsPlayingAction = false;
	bLocomotionPaused = false;
	bIsPlayingIdleVariation = false;
	CurrentActionMontage = nullptr;
	ActionTimer = 0.f;
	OnAIAnimEnded.Broadcast(Stopped);

	if (AnimationMode == EAIAnimationMode::DirectPlayback)
	{
		CurrentLocomotionMontage = nullptr;
		UpdateLocomotionDirect();
	}
}

/* ═══════════ ABP Callback ═══════════ */

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

UAnimMontage* UAIAnimationComponent::PlayViaMontageSystem(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage || !OwnerCharacter) return nullptr;
	if (UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		const float Dur = Anim->Montage_Play(Montage, PlayRate, EMontagePlayReturnType::MontageLength, 0.f, true);
		return Dur > 0.f ? Montage : nullptr;
	}
	return nullptr;
}
