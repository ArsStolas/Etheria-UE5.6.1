/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UDialogueWorldSubsystem" - Header
 */
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/Dialogue/Data/DialogueSystemTypes.h"
#include "DialogueWorldSubsystem.generated.h"

class ADialogueBubbleActor;
class UPlayerDialogueComponent;
class UUserWidget;

/**
 * Orchestrates dialogue sessions and speech bubbles at the World level.
 * - One active dialogue session at a time (expandable later)
 * - Speech bubbles are pooled via ADialogueBubbleActor for performance
 */
UCLASS(BlueprintType)
class ETHERIA_API UDialogueWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem lifecycle
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Session API
    UFUNCTION(BlueprintCallable, Category="Dialogue|Session")
    bool StartDialogueSession(const FDialogueSessionRequest& Request);

    UFUNCTION(BlueprintCallable, Category="Dialogue|Session")
    void EndDialogueSession(EDialogueEndReason Reason);

    UFUNCTION(BlueprintPure, Category="Dialogue|Session")
    bool IsDialogueActive() const { return bDialogueActive; }

    /** Advance to the next line (used when bAutoPlayLines is false). */
    UFUNCTION(BlueprintCallable, Category="Dialogue|Session")
    void NextLine();

    /** Skip current line timer and jump to next. */
    UFUNCTION(BlueprintCallable, Category="Dialogue|Session")
    void SkipCurrentLine();

    /** Broadcast choices for UI (dialogue pauses until SubmitChoice is called, if your BP decides so). */
    UFUNCTION(BlueprintCallable, Category="Dialogue|Choices")
    void RequestChoices(const TArray<FDialogueChoice>& Choices);

    UFUNCTION(BlueprintCallable, Category="Dialogue|Choices")
    void SubmitChoice(int32 ChoiceId);

    // Bubbles API
    UFUNCTION(BlueprintCallable, Category="Dialogue|Bubble")
    bool ShowBubble(AActor* Speaker, const FDialogueBubbleRequest& Request);

    UFUNCTION(BlueprintCallable, Category="Dialogue|Bubble")
    void HideBubble(AActor* Speaker);

    UFUNCTION(BlueprintCallable, Category="Dialogue|Bubble")
    void HideAllBubbles();

    // Delegates (bind in BP)
    UPROPERTY(BlueprintAssignable, Category="Dialogue|Events")
    FOnDialogueSessionStarted OnDialogueSessionStarted;

    UPROPERTY(BlueprintAssignable, Category="Dialogue|Events")
    FOnDialogueSessionEnded OnDialogueSessionEnded;

    UPROPERTY(BlueprintAssignable, Category="Dialogue|Events")
    FOnDialogueLineStarted OnDialogueLineStarted;

    UPROPERTY(BlueprintAssignable, Category="Dialogue|Events")
    FOnDialogueChoicesRequested OnDialogueChoicesRequested;

private:
    // Config
    UPROPERTY(EditAnywhere, Category="Dialogue|Config")
    TSubclassOf<ADialogueBubbleActor> BubbleActorClass;

    UPROPERTY(EditAnywhere, Category="Dialogue|Config")
    TSubclassOf<UUserWidget> DefaultBubbleWidgetClass;

    UPROPERTY(EditAnywhere, Category="Dialogue|Config", meta=(ClampMin="0"))
    int32 PrewarmBubblePoolSize = 8;

    /** If > 0, bubbles are hidden when far from player (cm). 0 disables distance culling. */
    UPROPERTY(EditAnywhere, Category="Dialogue|Config", meta=(ClampMin="0.0"))
    float MaxBubbleDistance = 0.0f;

    // Session state
    bool bDialogueActive = false;
    bool bWaitingForChoice = false;

    FDialogueSessionRequest ActiveRequest;
    int32 CurrentLineIndex = INDEX_NONE;

    FTimerHandle LineTimerHandle;

    // Bubble pooling
    TArray<TObjectPtr<ADialogueBubbleActor>> BubblePool;
    TMap<TWeakObjectPtr<AActor>, TObjectPtr<ADialogueBubbleActor>> ActiveBubbles;
    TMap<TWeakObjectPtr<AActor>, FTimerHandle> BubbleTimers;

    FTimerHandle BubbleCullTimer;

    // Internal helpers
    void PrewarmPool();
    ADialogueBubbleActor* AcquireBubble();
    void ReleaseBubble(ADialogueBubbleActor* Bubble);

    void HideBubbleWeak(TWeakObjectPtr<AActor> Speaker);

    void PlayLine(int32 LineIndex);
    void HandleLineTimer();

    void ApplyDialoguePolicy();
    void RestoreDialoguePolicy();

    UPlayerDialogueComponent* ResolvePlayerDialogueComponent() const;

    void StartBubbleCulling();
    void StopBubbleCulling();
    void HandleBubbleCullingTick();

    void UpdateDialogueCameraTarget(const FDialogueLine& Line);
};
