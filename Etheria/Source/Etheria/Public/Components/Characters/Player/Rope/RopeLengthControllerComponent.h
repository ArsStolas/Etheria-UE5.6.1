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
	#define ROPE_LENGHT_LOG(Category, Verbosity, Format, ...)
	#define ROPE_LENGHT_SCREEN_MSG(Key, Color, Format, ...)
#else
	#define ROPE_LENGHT_LOG(Category, Verbosity, Format, ...) \
	if (bLengthControllerDebugMode) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)
	#define ROPE_LENGHT_SCREEN_MSG(Key, Color, Format, ...) \
	if (bLengthControllerDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
#endif

class APlayerCharacter;
class URopeSwingComponent;
class URopePullComponent;
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
	void SetRopeLengthInput(float Value);

protected:
	void ProcessRopeLength(float DeltaTime);

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
	
	UPROPERTY()
	URopePullComponent* PullComp = nullptr;

	/** Input */
	float RopeLengthInput = 0.f;

	/** Climb Settings */
	UPROPERTY(EditAnywhere, Category="Rope|Climb")
	float RopeAdjustSpeed = 300.f;

	/** Debug */
	UPROPERTY(EditAnywhere, Category="Rope|LengthController|Debug")
	bool bLengthControllerDebugMode = false;
};
