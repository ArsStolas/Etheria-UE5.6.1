/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "ADialogueBubbleActor" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DialogueBubbleActor.generated.h"

class UWidgetComponent;
class UUserWidget;
class AActor;

/**
 * Lightweight pooled actor that displays a speech bubble widget in world space, attached to a speaker.
 * Spawned/pooled by UDialogueWorldSubsystem.
 */
UCLASS(BlueprintType)
class ETHERIA_API ADialogueBubbleActor : public AActor
{
    GENERATED_BODY()

public:
    ADialogueBubbleActor();

    UFUNCTION(BlueprintCallable, Category="Dialogue|Bubble")
    void ActivateForSpeaker(AActor* InSpeaker, TSubclassOf<UUserWidget> InWidgetClass, const FVector& InRelativeOffset);

    UFUNCTION(BlueprintCallable, Category="Dialogue|Bubble")
    void Deactivate();

    UFUNCTION(BlueprintCallable, Category="Dialogue|Bubble")
    void SetText(const FText& InText);

    UFUNCTION(BlueprintPure, Category="Dialogue|Bubble")
    bool IsActive() const { return bIsActive; }

    UFUNCTION(BlueprintPure, Category="Dialogue|Bubble")
    AActor* GetSpeaker() const { return Speaker.Get(); }

    UFUNCTION(BlueprintCallable, Category="Dialogue|Bubble")
    void SetBillboardEnabled(bool bEnabled);

protected:
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Dialogue|Bubble")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, Category="Dialogue|Bubble")
    TObjectPtr<UWidgetComponent> WidgetComponent;

    UPROPERTY()
    TWeakObjectPtr<AActor> Speaker;

    bool bIsActive = false;
    bool bBillboardToCamera = true;

    UPROPERTY(EditAnywhere, Category="Dialogue|Bubble", meta=(ClampMin="0.0"))
    float BillboardInterpSpeed = 12.0f;

    void UpdateBillboard(float DeltaSeconds);
};
