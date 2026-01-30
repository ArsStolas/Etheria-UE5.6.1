/*
* Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: BaseQuestGiver - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Characters/AI/BaseAI.h"
#include "Components/Quests/QuestGiverComponent.h"
#include "BaseQuestGiver.generated.h"

UCLASS()
class ETHERIA_API ABaseQuestGiver : public ABaseAI
{
 GENERATED_BODY()

public:
 ABaseQuestGiver();

protected:
 virtual void BeginPlay() override;

public:
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
 UQuestGiverComponent* QuestGiverComp;
};
