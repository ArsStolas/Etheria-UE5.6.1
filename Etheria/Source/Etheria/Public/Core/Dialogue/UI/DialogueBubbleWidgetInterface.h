/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDialogueBubbleWidgetInterface" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DialogueBubbleWidgetInterface.generated.h"

class AActor;

UINTERFACE(BlueprintType)
class UDialogueBubbleWidgetInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Implement this on your bubble widget (e.g. WBP_DialogueBubble) so C++ can set data without knowing widget internals.
 */
class IDialogueBubbleWidgetInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Dialogue|Bubble")
    void SetBubbleText(const FText& Text);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Dialogue|Bubble")
    void SetSpeaker(AActor* Speaker);
};
