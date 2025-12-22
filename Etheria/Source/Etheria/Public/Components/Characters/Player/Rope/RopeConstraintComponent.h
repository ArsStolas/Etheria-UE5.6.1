/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeConstraintComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeConstraintComponent.generated.h"

#if UE_BUILD_SHIPPING
	#define CONSTRAINT_LOG(Category, Verbosity, Format, ...)
#else
	#define CONSTRAINT_LOG(Category, Verbosity, Format, ...) \
	if (bConstraintDebugMode) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)
#endif

class APlayerCharacter;
class ARopeAttachPoint;
class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRopeTensioned);

/**
 * Manages a rope constraint for the player character when attached to a rope anchor point
 * Applies physics constraints to simulate rope tension
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeConstraintComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URopeConstraintComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	FORCEINLINE bool IsActive() const { return bIsActive; }
	FORCEINLINE ARopeAttachPoint* GetAnchor() const { return Anchor.Get(); }
	FORCEINLINE float GetRopeLength() const { return RopeLength; }

	void ActivateConstraint(ARopeAttachPoint* InAnchor, float InRopeLength);

	void DeactivateConstraint();

	void SetRopeLength(float NewLength);

	UPROPERTY(BlueprintAssignable)
	FOnRopeTensioned OnRopeTensioned;

private:
	void ApplyConstraint();

	APlayerCharacter* OwnerCharacter = nullptr;
	UCharacterMovementComponent* MoveComp = nullptr;

	TWeakObjectPtr<ARopeAttachPoint> Anchor;

	float RopeLength = 0.f;
	bool bIsActive = false;
	
	UPROPERTY(EditAnywhere, Category="Rope|Debug")
	bool bConstraintDebugMode = false;
};

