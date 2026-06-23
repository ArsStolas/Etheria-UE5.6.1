/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
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
		{
			UpdateLocomotionDirect();
			ApplyLocomotionSpeedMatch();
		}
	}
}

void UAIAnimationComponent::ApplyLocomotionSpeedMatch()
{
	if (!OwnerCharacter) return;
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	const UCharacterMovementComponent* MC = OwnerCharacter->GetCharacterMovement();
	if (!Mesh || !MC) return;

	const float Speed = MC->Velocity.Size2D();
	if (Speed < IdleSpeedThreshold) return; // idle clip plays at its own rate

	const bool bRunning = (CurrentLocomotionState == EAILocomotionState::RunForward || CurrentLocomotionState == EAILocomotionState::RunBackward);
	const float Ref = bRunning ? RunRefSpeed : WalkRefSpeed;
	// Match the clip's play rate to actual capsule speed so feet don't slide between the coarse walk/run buckets.
	Mesh->SetPlayRate(FMath::Clamp(Speed / FMath::Max(Ref, 1.f), 0.6f, 1.6f));
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

	const FVector Dir2D = Vel.GetSafeNormal2D();
	const float FwdDot = FVector::DotProduct(OwnerCharacter->GetActorForwardVector(), Dir2D);
	if (Speed >= RunSpeedThreshold)
	{
		SetLocomotionState(FwdDot >= 0.f ? EAILocomotionState::RunForward : EAILocomotionState::RunBackward);
		return;
	}
	// Walking: pick a lateral strafe state when moving more sideways than forward/back (combat orbit while facing target).
	const float RightDot = FVector::DotProduct(OwnerCharacter->GetActorRightVector(), Dir2D);
	if (FMath::Abs(RightDot) > FMath::Abs(FwdDot))
		SetLocomotionState(RightDot >= 0.f ? EAILocomotionState::WalkRight : EAILocomotionState::WalkLeft);
	else
		SetLocomotionState(FwdDot >= 0.f ? EAILocomotionState::WalkForward : EAILocomotionState::WalkBackward);
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
	case EAILocomotionState::WalkLeft:     T = WalkLeftMontage  ? WalkLeftMontage.Get()  : WalkForwardMontage.Get(); break;
	case EAILocomotionState::WalkRight:    T = WalkRightMontage ? WalkRightMontage.Get() : WalkForwardMontage.Get(); break;
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
	int32 Idx = FMath::RandRange(0, IdleVariations.Num() - 1);
	if (IdleVariations.Num() > 1 && Idx == LastIdleIndex) Idx = (Idx + 1) % IdleVariations.Num(); // no immediate repeat
	LastIdleIndex = Idx;
	UAnimMontage* R = PlayActionMontage(IdleVariations[Idx], IdlePlayRate);
	if (R) bIsPlayingIdleVariation = true;
	return R;
}

UAnimMontage* UAIAnimationComponent::PlayRandomActivity()
{
	if (ActivityMontages.Num() == 0) return nullptr;
	int32 Idx = FMath::RandRange(0, ActivityMontages.Num() - 1);
	if (ActivityMontages.Num() > 1 && Idx == LastActivityIndex) Idx = (Idx + 1) % ActivityMontages.Num(); // no immediate repeat
	LastActivityIndex = Idx;
	UAnimMontage* R = PlayActionMontage(ActivityMontages[Idx], IdlePlayRate);
	if (R) bIsPlayingIdleVariation = true; // plays to completion like an idle variation
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

UAnimMontage* UAIAnimationComponent::PlayDirectionalHitReaction(const FVector& WorldHitDir)
{
	if (!OwnerCharacter || WorldHitDir.IsNearlyZero()) return PlayRandomHitReaction();

	// WorldHitDir is the direction the hit TRAVELLED (attacker→us); flip it to point toward the attacker, then
	// express it in local space: +X front, +Y right.
	const FVector Local = OwnerCharacter->GetActorTransform().InverseTransformVectorNoScale((-WorldHitDir).GetSafeNormal2D());

	const TArray<TObjectPtr<UAnimMontage>>* Pool;
	if (FMath::Abs(Local.X) >= FMath::Abs(Local.Y)) Pool = (Local.X >= 0.f) ? &HitReactFront : &HitReactBack;
	else                                            Pool = (Local.Y >= 0.f) ? &HitReactRight : &HitReactLeft;

	if (Pool && Pool->Num() > 0)
	{
		StopCurrentAction(); // a hit interrupts everything
		return PlayActionMontage((*Pool)[FMath::RandRange(0, Pool->Num() - 1)]);
	}
	return PlayRandomHitReaction(); // no directional clips authored → generic pool
}

UAnimMontage* UAIAnimationComponent::PlayStartle()
{
	// DirectPlayback creatures express startle through their flee locomotion; a montage here would pause locomotion
	// and make them slide. Startle is an ABP (humanoid) feature where montages layer over the locomotion graph.
	if (AnimationMode == EAIAnimationMode::DirectPlayback) return nullptr;
	return StartleMontage ? PlayActionMontage(StartleMontage) : nullptr;
}

UAnimMontage* UAIAnimationComponent::PlayMenace()
{
	// The AI holds still while menacing, so a montage is safe even in DirectPlayback (no foot-slide).
	return MenaceMontage ? PlayActionMontage(MenaceMontage) : nullptr;
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
