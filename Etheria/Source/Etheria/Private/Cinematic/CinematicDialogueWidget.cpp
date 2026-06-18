/**
 * Etheria's End Project, 2025
 * Created by:  "0nnen"
 * Last Updated by: "0nnen"
 * Class: UCinematicDialogueWidget - Source
 * Notes: Default implementations driving the optionally-bound sub-widgets / animations.
 *        Override any of these events in the widget graph to fully customise the look.
 */


#include "Cinematic/CinematicDialogueWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Widget.h"
#include "Animation/WidgetAnimation.h"

void UCinematicDialogueWidget::SetSpeaker_Implementation(const FText& Speaker, bool bHasSpeaker)
{
    if (SpeakerText)
    {
        SpeakerText->SetText(Speaker);
    }

    if (SpeakerBox)
    {
        SpeakerBox->SetVisibility(bHasSpeaker ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    else if (SpeakerText)
    {
        SpeakerText->SetVisibility(bHasSpeaker ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void UCinematicDialogueWidget::SetDialogueText_Implementation(const FText& Text)
{
    if (DialogueText)
    {
        DialogueText->SetText(Text);
    }
}

void UCinematicDialogueWidget::SetDialogueVisible_Implementation(bool bVisible)
{
    const ESlateVisibility NewVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;

    if (DialogueBox)
    {
        DialogueBox->SetVisibility(NewVisibility);
    }
    else
    {
        // No dedicated box bound: toggle the whole widget.
        SetVisibility(NewVisibility);
    }
}

void UCinematicDialogueWidget::ShowNextPrompt_Implementation()
{
    if (NextPromptFadeIn)
    {
        PlayAnimation(NextPromptFadeIn);
    }
    else if (NextPrompt)
    {
        NextPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

void UCinematicDialogueWidget::HideNextPrompt_Implementation()
{
    if (NextPromptFadeOut)
    {
        PlayAnimation(NextPromptFadeOut);
    }
    else if (NextPrompt)
    {
        NextPrompt->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UCinematicDialogueWidget::SetSkipBarVisible_Implementation(bool bVisible)
{
    if (SkipBar)
    {
        SkipBar->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void UCinematicDialogueWidget::SetSkipProgress_Implementation(float Progress01)
{
    if (SkipBar)
    {
        SkipBar->SetPercent(FMath::Clamp(Progress01, 0.f, 1.f));
    }
}

void UCinematicDialogueWidget::SetNextKey_Implementation(const FKey& Key)
{
    if (NextKeyText)
    {
        NextKeyText->SetText(Key.IsValid() ? Key.GetDisplayName() : FText::GetEmpty());
    }
}
