/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDialogueSystemSettings" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DialogueSystemSettings.generated.h"

class ADialogueBubbleActor;
class UUserWidget;

/**
 * Project settings for the Dialogue system (Project Settings -> Game -> Dialogue System).
 * Keeps subsystems data-driven and avoids hardcoding widget classes in C++.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Dialogue System"))
class ETHERIA_API UDialogueSystemSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    static const UDialogueSystemSettings* Get();

    /** Pooled actor used to display bubbles. You can subclass ADialogueBubbleActor in BP. */
    UPROPERTY(Config, EditAnywhere, Category="Bubbles")
    TSubclassOf<ADialogueBubbleActor> BubbleActorClass;

    /** Default widget class for bubbles (WBP_...). Must implement DialogueBubbleWidgetInterface. */
    UPROPERTY(Config, EditAnywhere, Category="Bubbles")
    TSubclassOf<UUserWidget> DefaultBubbleWidgetClass;

    UPROPERTY(Config, EditAnywhere, Category="Bubbles", meta=(ClampMin="0"))
    int32 PrewarmBubblePoolSize = 8;

    /** If > 0, bubbles are hidden when the speaker is far from the player (cm). 0 disables distance culling. */
    UPROPERTY(Config, EditAnywhere, Category="Bubbles", meta=(ClampMin="0.0"))
    float MaxBubbleDistance = 0.0f;
};
