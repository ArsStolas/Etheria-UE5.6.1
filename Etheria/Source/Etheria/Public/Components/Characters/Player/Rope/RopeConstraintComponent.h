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
	#define CONSTRAINT_SCREEN_MSG(Key, Color, Format, ...)
#else
	#define CONSTRAINT_LOG(Category, Verbosity, Format, ...) \
	if (bConstraintDebugMode) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)
	#define CONSTRAINT_SCREEN_MSG(Key, Color, Format, ...) \
	if (bConstraintDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
#endif

class APlayerCharacter;
class ARopeAttachPoint;
class UCharacterMovementComponent;
class URopeAttachComponent;

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
	FORCEINLINE float GetCurrentPullRopeLength() const { return CurrentPullRopeLength; }
	void SetCurrentPullRopeLength(float NewLength) { CurrentPullRopeLength = FMath::Max(0.f, NewLength); }

	void ActivateConstraint(ARopeAttachPoint* InAnchor, float InRopeLength);

	void DeactivateConstraint();

	void SetRopeLength(float NewLength);

	UPROPERTY(BlueprintAssignable)
	FOnRopeTensioned OnRopeTensioned;

private:
	void ApplyConstraint(float DeltaTime);

	UPROPERTY()
	APlayerCharacter* OwnerCharacter = nullptr;
	
	UPROPERTY()
	UCharacterMovementComponent* MoveComp = nullptr;
	
	UPROPERTY()
	URopeAttachComponent* AttachComponent = nullptr;

	TWeakObjectPtr<ARopeAttachPoint> Anchor;

	float RopeLength = 0.f;
	bool bIsActive = false;
	
	// Tracking Pull
	float CurrentPullRopeLength = 0.f;
	float LastPlayerObjectDistance = 0.f;
	FVector LastObjectLocation = FVector::ZeroVector;

	// Détection blocage robuste
	float BlockedAccumulator = 0.f;          // temps accumulé de tension sans mouvement objet
	float BlockedConfirmDelay = 0.15f;       // secondes avant de confirmer le blocage
	float UnblockedAccumulator = 0.f;        // temps accumulé sans tension
	float UnblockedConfirmDelay = 0.1f;      // secondes avant de confirmer le déblocage
	bool bIsObjectBlocked = false;

	UPROPERTY(EditAnywhere, Category="Rope|Pull|Debug")
	float BlockDetectionSensitivity = 3.f;   // cm/frame en dessous desquels on considère l'objet immobile
	
	/** Smoothing for constraint application (prevents jitter) */
	UPROPERTY(EditAnywhere, Category="Rope|Constraint")
	float ConstraintSmoothness = 0.8f;
	
	UPROPERTY(EditAnywhere, Category="Rope|Debug")
	bool bConstraintDebugMode = false;
};
