/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UBarkComponent" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Dialogue/Data/DialogueSystemTypes.h"
#include "BarkComponent.generated.h"

/**
 * Optional component for ambient barks (NPC talking without player interaction).
 * Uses timers (no tick) and can be distance-gated for performance.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Systems), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UBarkComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBarkComponent();

    /** Start emitting barks at random intervals. */
    UFUNCTION(BlueprintCallable, Category="Dialogue|Barks")
    void StartAmbientBarks();

    /** Stop emitting barks. */
    UFUNCTION(BlueprintCallable, Category="Dialogue|Barks")
    void StopAmbientBarks();

    /** Immediately plays a bark bubble once. */
    UFUNCTION(BlueprintCallable, Category="Dialogue|Barks")
    void PlayBarkOnce(const FDialogueBubbleRequest& Request);

    UFUNCTION(BlueprintPure, Category="Dialogue|Barks")
    bool IsRunning() const { return bRunning; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(EditAnywhere, Category="Dialogue|Barks")
    bool bAutoStart = false;

    UPROPERTY(EditAnywhere, Category="Dialogue|Barks", meta=(ClampMin="0.1"))
    float MinInterval = 4.0f;

    UPROPERTY(EditAnywhere, Category="Dialogue|Barks", meta=(ClampMin="0.1"))
    float MaxInterval = 9.0f;

    /** If > 0, barks will only play when the local player is within this distance (cm). */
    UPROPERTY(EditAnywhere, Category="Dialogue|Barks", meta=(ClampMin="0.0"))
    float MaxDistanceToPlayer = 2500.0f;

    /** Pool of possible bark texts. */
    UPROPERTY(EditAnywhere, Category="Dialogue|Barks")
    TArray<FText> BarkTexts;

    bool bRunning = false;
    FTimerHandle BarkTimer;

    void ScheduleNext();
    void HandleBarkTimer();
};
