/*
* Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: BaseAI - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/BaseCharacter.h"
#include "BaseAI.generated.h"

UCLASS()
class ETHERIA_API ABaseAI : public ABaseCharacter
{
	GENERATED_BODY()

public:
	ABaseAI();

protected:
	virtual void BeginPlay() override;

	virtual void HandlePerception();
	virtual void HandleDecisionMaking();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "AI")
	void OnAIDeath();

public:
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	bool bIsHostile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
	AActor* CurrentTarget = nullptr;
};
