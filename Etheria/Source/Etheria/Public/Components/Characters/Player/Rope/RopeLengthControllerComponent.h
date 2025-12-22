/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: RopeLengthControllerComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeLengthControllerComponent.generated.h"

#if UE_BUILD_SHIPPING
	#define CLIMB_LOG(Category, Verbosity, Format, ...)
	#define CLIMB_SCREEN_MSG(Key, Color, Format, ...)
#else
	#define CLIMB_LOG(Category, Verbosity, Format, ...) \
	if (bLengthControllerDebugMode) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)
	#define CLIMB_SCREEN_MSG(Key, Color, Format, ...) \
	if (bLengthControllerDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
#endif

class APlayerCharacter;
class URopeSwingComponent;
class URopeAttachComponent;
class URopeConstraintComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeLengthControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URopeLengthControllerComponent();

	virtual void BeginPlay() override;
    
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Called by input system to set climb input */
	void SetClimbInput(float Value);

protected:
	void ProcessClimbing(float DeltaTime);

private:
	/** References */
	UPROPERTY()
	APlayerCharacter* OwnerCharacter = nullptr;
	
	UPROPERTY()
	URopeAttachComponent* AttachComponent = nullptr;

	UPROPERTY()
	URopeConstraintComponent* ConstraintComponent = nullptr;
	
	UPROPERTY()
	URopeSwingComponent* SwingComp = nullptr;

	/** Input */
	float ClimbInput = 0.f;

	/** Climb Settings */
	UPROPERTY(EditAnywhere, Category="Rope|Climb")
	float ClimbSpeed = 300.f;

	UPROPERTY(EditAnywhere, Category="Rope|Climb")
	float MinRopeLength = 500.f;

	UPROPERTY(EditAnywhere, Category="Rope|Climb")
	float MaxRopeLength = 1600.f;

	/** Debug */
	UPROPERTY(EditAnywhere, Category="Rope|LengthController|Debug")
	bool bLengthControllerDebugMode = false;
};
