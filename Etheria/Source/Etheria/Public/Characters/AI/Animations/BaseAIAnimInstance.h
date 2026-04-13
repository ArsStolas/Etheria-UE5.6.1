/**
 * Etheria's End Project, 2025
 * Created by: Mato
 * Last Updated by: Mato
 * Class: "BaseAIAnimInstance - Header"
 * Notes: Minimal AnimInstance for AI characters. Exists solely so montages can play.
 *        Set this as AnimClass on the Mesh — no Animation Blueprint needed.
 */

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BaseAIAnimInstance.generated.h"

UCLASS()
class ETHERIA_API UBaseAIAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Current horizontal speed — can be read in montage blend logic if needed. */
	UPROPERTY(BlueprintReadOnly, Category = "AI|Locomotion")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "AI|Locomotion")
	bool bIsFalling = false;

private:
	UPROPERTY()
	TObjectPtr<class ACharacter> OwnerCharacter;
};
