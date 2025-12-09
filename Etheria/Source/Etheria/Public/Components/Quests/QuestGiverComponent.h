/**
 * Etheria's End Project, 2025
 * Created by:  0nnen
 * Last Updated by: 0nnen
 * Class: "QuestGiverComponent" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuestGiverComponent.generated.h"

class UQuestDefinition;
class UQuestComponent;

/**
 * Component placed on NPCs that can offer quests to players.
 * Only contains minimal logic: which quests are available and how to give them.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UQuestGiverComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UQuestGiverComponent();

protected:
    virtual void BeginPlay() override;

public:
    /** Quests this NPC can offer. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QuestGiver")
    TArray<TObjectPtr<UQuestDefinition>> questsToOffer;

    /**
     * @brief Offer a quest to the given QuestComponent (ex: player).
     * Typically called from dialogue / interaction.
     */
    UFUNCTION(BlueprintCallable, Category = "QuestGiver")
    bool OfferQuestToTarget(UQuestComponent* TargetQuestComponent, UQuestDefinition* QuestDef);

    /**
     * @brief Offers all quests that are not yet active on the target.
     */
    UFUNCTION(BlueprintCallable, Category = "QuestGiver")
    void OfferAllAvailableToTarget(UQuestComponent* TargetQuestComponent);
};
