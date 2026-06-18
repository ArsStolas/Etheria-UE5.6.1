/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: UCinematicDialogueWidget
 * Notes: Base UMG class for the cinematic dialogue box. Reparent your widget to this.
 *        Every function is a BlueprintNativeEvent: name your sub-widgets with the conventional
 *        names below to get a working default, or override the event in the widget graph.
 *        Conventional widget names : SpeakerText, DialogueText, SpeakerBox, DialogueBox,
 *        NextPrompt, SkipBar, NextKeyText.
 *        Conventional animation names: NextPromptFadeIn, NextPromptFadeOut.
 */
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "CinematicDialogueWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UWidget;
class UWidgetAnimation;

UCLASS(Abstract, Blueprintable, meta = (ToolTip = "Base class for the cinematic dialogue UMG. Reparent your widget to this."))
class ETHERIA_API UCinematicDialogueWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /* ═══════════ Driven by UCinematicDirectorComponent ═══════════ */

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Etheria|Cinematic|Widget")
    void SetSpeaker(const FText& Speaker, bool bHasSpeaker);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Etheria|Cinematic|Widget")
    void SetDialogueText(const FText& Text);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Etheria|Cinematic|Widget")
    void SetDialogueVisible(bool bVisible);

    // Fade the "Next" prompt in (plays NextPromptFadeIn if present).
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Etheria|Cinematic|Widget")
    void ShowNextPrompt();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Etheria|Cinematic|Widget")
    void HideNextPrompt();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Etheria|Cinematic|Widget")
    void SetSkipBarVisible(bool bVisible);

    // 0..1 fill of the hold-to-skip bar.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Etheria|Cinematic|Widget")
    void SetSkipProgress(float Progress01);

    // Key glyph shown on the Next prompt.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Etheria|Cinematic|Widget")
    void SetNextKey(const FKey& Key);

protected:
    /* ═══════════ Optional bound widgets (name them in UMG to enable the defaults) ═══════════ */

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Etheria|Cinematic|Widget")
    TObjectPtr<UTextBlock> SpeakerText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Etheria|Cinematic|Widget")
    TObjectPtr<UTextBlock> DialogueText;

    // Container of the speaker name; collapsed when there is no speaker. Falls back to SpeakerText.
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Etheria|Cinematic|Widget")
    TObjectPtr<UWidget> SpeakerBox;

    // Container of the whole dialogue box (speaker + text). Toggled by SetDialogueVisible.
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Etheria|Cinematic|Widget")
    TObjectPtr<UWidget> DialogueBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Etheria|Cinematic|Widget")
    TObjectPtr<UWidget> NextPrompt;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Etheria|Cinematic|Widget")
    TObjectPtr<UProgressBar> SkipBar;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Etheria|Cinematic|Widget")
    TObjectPtr<UTextBlock> NextKeyText;

    /* ═══════════ Optional bound animations ═══════════ */

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> NextPromptFadeIn;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> NextPromptFadeOut;
};
