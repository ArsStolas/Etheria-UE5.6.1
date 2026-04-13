/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAIAnimInstance - Source"
 * Notes: Minimal AnimInstance. Montage playback is driven by AIAnimationComponent.
 */

#include "Characters/AI/Animations/BaseAIAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBaseAIAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
}

void UBaseAIAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter) return;

	if (const UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement())
	{
		GroundSpeed = MoveComp->Velocity.Size2D();
		bIsFalling = MoveComp->IsFalling();
	}
}
