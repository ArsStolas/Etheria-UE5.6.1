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
#include "Animation/AnimSingleNodeInstance.h"
#include "KismetAnimationLibrary.h"

UAIAnimationComponent::UAIAnimationComponent() { PrimaryComponentTick.bCanEverTick = true; }

namespace
{

	FName ResolveABPFloatName(UClass* C, FName Configured, std::initializer_list<FName> Fallbacks)
	{
		if (Configured.IsNone()) return NAME_None;
		auto IsFloatLike = [C](const FName& N) { return FindFProperty<FFloatProperty>(C, N) || FindFProperty<FDoubleProperty>(C, N); };
		if (IsFloatLike(Configured)) return Configured;
		for (const FName& N : Fallbacks)
			if (IsFloatLike(N)) return N;
		return NAME_None;
	}

	FName ResolveABPVectorName(UClass* C, FName Configured, std::initializer_list<FName> Fallbacks)
	{
		if (Configured.IsNone()) return NAME_None;
		auto IsVector = [C](const FName& N)
		{
			const FStructProperty* P = FindFProperty<FStructProperty>(C, N);
			return P && P->Struct == TBaseStructure<FVector>::Get();
		};
		if (IsVector(Configured)) return Configured;
		for (const FName& N : Fallbacks)
			if (IsVector(N)) return N;
		return NAME_None;
	}

	FBoolProperty* ResolveABPBoolProp(UClass* C, std::initializer_list<FName> Candidates)
	{
		for (const FName& N : Candidates)
			if (FBoolProperty* P = FindFProperty<FBoolProperty>(C, N)) return P;
		return nullptr;
	}
}

void UAIAnimationComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ABaseAICharacter>(GetOwner());

}

void UAIAnimationComponent::EnsureModeInitialized()
{
	if (bModeInitialized || !OwnerCharacter) return;
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh) return;
	bModeInitialized = true;

	const bool bHasLocomotionMontages = (WalkForwardMontage || RunForwardMontage || IdleBaseMontage);
	if (bAutoCorrectAnimationMode)
	{
		if (AnimationMode == EAIAnimationMode::DirectPlayback && !bHasLocomotionMontages && Mesh->GetAnimClass())
		{
			AnimationMode = EAIAnimationMode::AnimBlueprint;
			UE_LOG(LogTemp, Warning, TEXT("[%s] AIAnimation: DirectPlayback had no locomotion montages but the mesh has an ABP — auto-switched to AnimBlueprint mode (set bAutoCorrectAnimationMode=false to opt out)."),
				*OwnerCharacter->GetName());
		}
		else if (AnimationMode == EAIAnimationMode::AnimBlueprint && !Mesh->GetAnimClass() && bHasLocomotionMontages)
		{
			AnimationMode = EAIAnimationMode::DirectPlayback;
			UE_LOG(LogTemp, Warning, TEXT("[%s] AIAnimation: AnimBlueprint mode had no anim class on the mesh but locomotion montages are assigned — auto-switched to DirectPlayback (set bAutoCorrectAnimationMode=false to opt out)."),
				*OwnerCharacter->GetName());
		}
	}

	if (AnimationMode == EAIAnimationMode::DirectPlayback)
	{

		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		if (IdleBaseMontage) { PlayOnMesh(IdleBaseMontage, true); CurrentLocomotionMontage = IdleBaseMontage; }
	}
	else
	{

		if (Mesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint && Mesh->GetAnimClass())
			Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}
}

void UAIAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	EnsureModeInitialized();

	if (AnimationMode == EAIAnimationMode::AnimBlueprint)
	{

		UpdateABPVariables(DeltaTime);

	}
	else
	{
		if (bIsPlayingAction)
		{

			float RateScale = 1.f;
			if (OwnerCharacter)
				if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
					RateScale = Mesh->GlobalAnimRateScale;
			ActionTimer -= DeltaTime * FMath::Max(RateScale, 0.f);
			if (ActionTimer <= 0.f)
			{
				UAnimMontage* Finished = CurrentActionMontage;
				bIsPlayingAction = false;
				bLocomotionPaused = false;
				bIsPlayingIdleVariation = false;
				bCurrentActionIsHitReact = false;
				CurrentActionMontage = nullptr;
				ActionTimer = 0.f;
				SmoothedPlayRate = 1.f;
				OnAIAnimEnded.Broadcast(Finished);

				if (!bIsPlayingAction)
				{
					CurrentLocomotionMontage = nullptr;
					UpdateLocomotionDirect();
				}
			}
			return;
		}

		if (!bLocomotionPaused && !bIsPlayingIdleVariation)
		{
			UpdateLocomotionDirect();
			ApplyLocomotionSpeedMatch(DeltaTime);
		}
	}
}

void UAIAnimationComponent::ApplyLocomotionSpeedMatch(float DeltaTime)
{
	if (!OwnerCharacter) return;
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	const UCharacterMovementComponent* MC = OwnerCharacter->GetCharacterMovement();
	if (!Mesh || !MC) return;

	const float Speed = MC->Velocity.Size2D();

	const bool bMovingState =
		CurrentLocomotionState != EAILocomotionState::Idle &&
		CurrentLocomotionState != EAILocomotionState::Falling &&
		CurrentLocomotionState != EAILocomotionState::Landing;
	if (Speed < IdleSpeedThreshold || !bMovingState)
	{
		if (!FMath::IsNearlyEqual(SmoothedPlayRate, 1.f)) { SmoothedPlayRate = 1.f; Mesh->SetPlayRate(1.f); }
		return;
	}

	const bool bRunning = (CurrentLocomotionState == EAILocomotionState::RunForward || CurrentLocomotionState == EAILocomotionState::RunBackward);

	const float MeshScale = FMath::Max(Mesh->GetComponentScale().X, 0.05f);
	const float Ref = (bRunning ? RunRefSpeed : WalkRefSpeed) * MeshScale;
	const float Target = FMath::Clamp(Speed / FMath::Max(Ref, 1.f),
		bRunning ? FMath::Max(0.6f, PlayRateMin) : PlayRateMin, FMath::Max(PlayRateMax, 1.f));

	SmoothedPlayRate = FMath::FInterpTo(SmoothedPlayRate, Target, DeltaTime, PlayRateInterpSpeed);
	Mesh->SetPlayRate(SmoothedPlayRate);
}

void UAIAnimationComponent::UpdateABPVariables(float DeltaTime)
{
	if (!OwnerCharacter) return;
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh) return;
	UAnimInstance* Anim = Mesh->GetAnimInstance();
	if (!Anim) return;

	const UCharacterMovementComponent* MC = OwnerCharacter->GetCharacterMovement();
	if (!MC) return;

	const FVector Vel = MC->Velocity;
	const float RawSpeed = Vel.Size2D();

	SmoothedGroundSpeed = FMath::FInterpTo(SmoothedGroundSpeed, RawSpeed, DeltaTime, GroundSpeedInterpSpeed);
	const float RawDir = UKismetAnimationLibrary::CalculateDirection(Vel, OwnerCharacter->GetActorRotation());
	const float DirDelta = FMath::UnwindDegrees(RawDir - SmoothedDirection);
	SmoothedDirection = FMath::UnwindDegrees(SmoothedDirection + DirDelta * FMath::Clamp(DeltaTime * DirectionInterpSpeed, 0.f, 1.f));

	if (CachedABPClass.Get() != Anim->GetClass())
	{
		CachedABPClass = Anim->GetClass();
		UClass* C = Anim->GetClass();
		ResolvedVelocityName    = ResolveABPVectorName(C, ABP_VelocityName, { TEXT("Velocity"), TEXT("CharacterVelocity") });
		ResolvedGroundSpeedName = ResolveABPFloatName(C, ABP_GroundSpeedName, { TEXT("GroundSpeed"), TEXT("Speed"), TEXT("MoveSpeed"), TEXT("CurrentSpeed"), TEXT("WalkSpeed") });
		ResolvedFallSpeedName   = ResolveABPFloatName(C, ABP_FallSpeedName, { TEXT("FallSpeed"), TEXT("VerticalVelocity"), TEXT("ZVelocity") });
		ResolvedDirectionName   = ResolveABPFloatName(C, ABP_DirectionName, { TEXT("Direction"), TEXT("MovementDirection"), TEXT("MoveDirection") });
		ShouldMovePropCached = ResolveABPBoolProp(C, { TEXT("ShouldMove"), TEXT("bShouldMove"), TEXT("IsMoving"), TEXT("bIsMoving"), TEXT("IsAccelerating"), TEXT("bIsAccelerating") });
		IsFallingPropCached  = ResolveABPBoolProp(C, { TEXT("IsFalling"), TEXT("bIsFalling"), TEXT("IsInAir"), TEXT("bIsInAir") });

		if (ResolvedGroundSpeedName == NAME_None && ResolvedVelocityName == NAME_None && !ShouldMovePropCached
			&& !Anim->IsA<UAnimSingleNodeInstance>())
			UE_LOG(LogTemp, Warning, TEXT("[%s] AIAnimation: ABP '%s' exposes none of the expected locomotion variables (GroundSpeed/Speed/Velocity/ShouldMove/IsMoving...) — nav movement will not animate. Set the ABP_* name overrides on AIAnimationComponent to match your ABP."),
				*OwnerCharacter->GetName(), *C->GetName());
	}

	SetABPVector(Anim, ResolvedVelocityName, Vel);
	SetABPFloat(Anim, ResolvedGroundSpeedName, SmoothedGroundSpeed);
	SetABPFloat(Anim, ResolvedFallSpeedName, Vel.Z);
	SetABPFloat(Anim, ResolvedDirectionName, SmoothedDirection);

	if (BoundAnimInstance.Get() != Anim)
	{
		Anim->OnMontageEnded.AddDynamic(this, &UAIAnimationComponent::HandleMontageEnded);
		BoundAnimInstance = Anim;
	}
	bABPShouldMove = bABPShouldMove ? (RawSpeed > 10.f)
	                                : (RawSpeed > FMath::Max(IdleSpeedThreshold * 4.f, 25.f));
	if (ShouldMovePropCached) ShouldMovePropCached->SetPropertyValue_InContainer(Anim, bABPShouldMove);
	if (IsFallingPropCached)  IsFallingPropCached->SetPropertyValue_InContainer(Anim, MC->IsFalling());
}

void UAIAnimationComponent::SetABPFloat(UAnimInstance* Anim, FName Name, float Value)
{
	if (Name == NAME_None) return;
	if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(Anim->GetClass(), Name))
	{
		Prop->SetPropertyValue_InContainer(Anim, Value);
		return;
	}

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

void UAIAnimationComponent::UpdateLocomotionDirect()
{
	if (!OwnerCharacter) return;
	const UCharacterMovementComponent* MC = OwnerCharacter->GetCharacterMovement();
	if (!MC) return;

	if (MC->IsFalling()) { SetLocomotionState(EAILocomotionState::Falling); return; }

	const FVector Vel = MC->Velocity;
	const float Speed = Vel.Size2D();

	const bool bWasIdle = (CurrentLocomotionState == EAILocomotionState::Idle);
	const float MoveEnter = FMath::Max(IdleSpeedThreshold * 2.f, IdleSpeedThreshold + 10.f);
	if (bWasIdle ? (Speed < MoveEnter) : (Speed < IdleSpeedThreshold)) { SetLocomotionState(EAILocomotionState::Idle); return; }

	const FVector Dir2D  = Vel.GetSafeNormal2D();
	const float FwdDot   = FVector::DotProduct(OwnerCharacter->GetActorForwardVector(), Dir2D);
	const float RightDot = FVector::DotProduct(OwnerCharacter->GetActorRightVector(),   Dir2D);

	const bool bWasForward =
		CurrentLocomotionState == EAILocomotionState::WalkForward || CurrentLocomotionState == EAILocomotionState::RunForward ||
		CurrentLocomotionState == EAILocomotionState::WalkLeft   || CurrentLocomotionState == EAILocomotionState::WalkRight  ||
		CurrentLocomotionState == EAILocomotionState::Idle;
	const bool bFwd = bWasForward ? (FwdDot > -0.15f) : (FwdDot > 0.15f);

	const bool bWasStrafing = (CurrentLocomotionState == EAILocomotionState::WalkLeft || CurrentLocomotionState == EAILocomotionState::WalkRight);
	const float StrafeMargin = bWasStrafing ? -0.15f : 0.25f;
	if (FMath::Abs(RightDot) > FMath::Abs(FwdDot) + StrafeMargin)
	{
		SetLocomotionState(RightDot >= 0.f ? EAILocomotionState::WalkRight : EAILocomotionState::WalkLeft);
		return;
	}

	const bool bWasRunning = (CurrentLocomotionState == EAILocomotionState::RunForward || CurrentLocomotionState == EAILocomotionState::RunBackward);
	const float RunExit = RunSpeedThreshold * FMath::Clamp(RunHysteresisFraction, 0.1f, 0.99f);
	if (bWasRunning ? (Speed > RunExit) : (Speed >= RunSpeedThreshold))
	{
		SetLocomotionState(bFwd ? EAILocomotionState::RunForward : EAILocomotionState::RunBackward);
		return;
	}
	SetLocomotionState(bFwd ? EAILocomotionState::WalkForward : EAILocomotionState::WalkBackward);
}

void UAIAnimationComponent::SetLocomotionState(EAILocomotionState NewState)
{
	if (AnimationMode == EAIAnimationMode::AnimBlueprint) return;

	if (CurrentLocomotionState == NewState && CurrentLocomotionMontage) return;
	const EAILocomotionState OldState = CurrentLocomotionState;
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

	if (T && T != CurrentLocomotionMontage)
	{

		float PhaseFrac = -1.f;
		const bool bOldMoving = (OldState != EAILocomotionState::Idle && OldState != EAILocomotionState::Falling && OldState != EAILocomotionState::Landing);
		const bool bNewMoving = (NewState != EAILocomotionState::Idle && NewState != EAILocomotionState::Falling && NewState != EAILocomotionState::Landing);
		if (bOldMoving && bNewMoving && CurrentLocomotionMontage && OwnerCharacter)
			if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
				if (UAnimSingleNodeInstance* SN = Mesh->GetSingleNodeInstance())
					if (const float OldLen = CurrentLocomotionMontage->GetPlayLength(); OldLen > 0.f)
						PhaseFrac = FMath::Fmod(SN->GetCurrentTime() / OldLen, 1.f);

		PlayOnMesh(T, true);
		CurrentLocomotionMontage = T;

		if (PhaseFrac >= 0.f && OwnerCharacter)
			if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
				if (UAnimSingleNodeInstance* SN = Mesh->GetSingleNodeInstance())
					SN->SetPosition(PhaseFrac * T->GetPlayLength(), false);
	}
	OnLocomotionStateChanged.Broadcast();
}

UAnimMontage* UAIAnimationComponent::PlayRandomIdle()
{
	if (IdleVariations.Num() == 0) return nullptr;
	int32 Idx = FMath::RandRange(0, IdleVariations.Num() - 1);
	if (IdleVariations.Num() > 1 && Idx == LastIdleIndex) Idx = (Idx + 1) % IdleVariations.Num();
	LastIdleIndex = Idx;
	UAnimMontage* R = PlayActionMontage(IdleVariations[Idx], IdlePlayRate);
	if (R) bIsPlayingIdleVariation = true;
	return R;
}

UAnimMontage* UAIAnimationComponent::PlayRandomActivity()
{
	if (ActivityMontages.Num() == 0) return nullptr;
	int32 Idx = FMath::RandRange(0, ActivityMontages.Num() - 1);
	if (ActivityMontages.Num() > 1 && Idx == LastActivityIndex) Idx = (Idx + 1) % ActivityMontages.Num();
	LastActivityIndex = Idx;
	UAnimMontage* R = PlayActionMontage(ActivityMontages[Idx], IdlePlayRate);
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
	StopCurrentAction();
	UAnimMontage* R = PlayActionMontage(HitReactionMontages[FMath::RandRange(0, HitReactionMontages.Num() - 1)]);
	if (R) bCurrentActionIsHitReact = true;
	return R;
}

UAnimMontage* UAIAnimationComponent::PlayDirectionalHitReaction(const FVector& WorldHitDir)
{
	if (!OwnerCharacter || WorldHitDir.IsNearlyZero()) return PlayRandomHitReaction();

	const FVector Local = OwnerCharacter->GetActorTransform().InverseTransformVectorNoScale((-WorldHitDir).GetSafeNormal2D());

	const TArray<TObjectPtr<UAnimMontage>>* Pool;
	if (FMath::Abs(Local.X) >= FMath::Abs(Local.Y)) Pool = (Local.X >= 0.f) ? &HitReactFront : &HitReactBack;
	else                                            Pool = (Local.Y >= 0.f) ? &HitReactRight : &HitReactLeft;

	if (Pool && Pool->Num() > 0)
	{
		StopCurrentAction();
		UAnimMontage* R = PlayActionMontage((*Pool)[FMath::RandRange(0, Pool->Num() - 1)]);
		if (R) bCurrentActionIsHitReact = true;
		return R;
	}
	return PlayRandomHitReaction();
}

UAnimMontage* UAIAnimationComponent::PlayStartle()
{

	if (AnimationMode == EAIAnimationMode::DirectPlayback) return nullptr;
	return StartleMontage ? PlayActionMontage(StartleMontage) : nullptr;
}

UAnimMontage* UAIAnimationComponent::PlayMenace()
{

	return MenaceMontage ? PlayActionMontage(MenaceMontage) : nullptr;
}

UAnimMontage* UAIAnimationComponent::PlayDeath() { return PlayRandomDeath(); }

UAnimMontage* UAIAnimationComponent::PlayRandomDeath()
{
	EnsureModeInitialized();
	if (DeathMontages.Num() == 0) return nullptr;
	UAnimMontage* M = DeathMontages[FMath::RandRange(0, DeathMontages.Num() - 1)];
	if (!M) return nullptr;

	if (AnimationMode == EAIAnimationMode::DirectPlayback)
	{
		PlayOnMesh(M, false);
	}
	else if (!PlayViaMontageSystem(M))
	{
		return nullptr;
	}

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
	EnsureModeInitialized();

	bCurrentActionIsHitReact = false;

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

UAnimMontage* UAIAnimationComponent::PlayLoopingAction(UAnimMontage* Montage, float MaxDuration, float PlayRate)
{
	if (!Montage || !OwnerCharacter || MaxDuration <= 0.f) return nullptr;
	EnsureModeInitialized();

	bCurrentActionIsHitReact = false;

	if (AnimationMode == EAIAnimationMode::DirectPlayback)
	{
		PlayOnMesh(Montage, true, PlayRate);
		bIsPlayingAction = true;
		bLocomotionPaused = true;
		CurrentActionMontage = Montage;
		ActionTimer = MaxDuration;
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
	bCurrentActionIsHitReact = false;
	CurrentActionMontage = nullptr;
	ActionTimer = 0.f;
	SmoothedPlayRate = 1.f;
	OnAIAnimEnded.Broadcast(Stopped);

	if (AnimationMode == EAIAnimationMode::DirectPlayback && !bIsPlayingAction)
	{
		CurrentLocomotionMontage = nullptr;
		UpdateLocomotionDirect();
	}
}

void UAIAnimationComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (AnimationMode != EAIAnimationMode::AnimBlueprint) return;

	if (Montage == CurrentActionMontage)
	{

		if (OwnerCharacter)
			if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
				if (UAnimInstance* Anim = Mesh->GetAnimInstance())
					if (Anim->Montage_IsPlaying(Montage)) return;

		UAnimMontage* Finished = CurrentActionMontage;
		bIsPlayingAction = false;
		bLocomotionPaused = false;
		bIsPlayingIdleVariation = false;
		bCurrentActionIsHitReact = false;
		CurrentActionMontage = nullptr;
		OnAIAnimEnded.Broadcast(Finished);
	}
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
