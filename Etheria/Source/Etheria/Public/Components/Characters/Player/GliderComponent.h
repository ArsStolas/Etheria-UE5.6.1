/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GliderComponent - Header
*/
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GliderComponent.generated.h"

class APlayerCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UGliderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGliderComponent();

	void ToggleGliding();
	bool IsGliding() const { return bIsGliderActive; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glider|Settings", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinimumHeight = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Glider|Settings", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DescendingRate = 300.f;

private:
	void StartGliding();
	void StopGliding();
	bool CanStartGliding() const;
	void RecordOriginalSettings();
	void ApplyOriginalSettings();
	void HandleDescent(float DeltaTime);

	bool bIsGliderActive = false;
	FVector CurrentVelocity;

	bool OriginalOrientRotation;
	float OriginalGravityScale;
	float OriginalWalkingSpeed;
	float OriginalDeceleration;
	float OriginalAcceleration;
	float OriginalAirControl;
	bool OriginalDesiredRotation;

	APlayerCharacter* OwnerCharacter = nullptr;
};
