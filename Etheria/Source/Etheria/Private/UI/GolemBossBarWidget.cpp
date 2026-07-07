/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemBossBarWidget - Source"
 */

#include "UI/GolemBossBarWidget.h"

#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UGolemBossBarWidget::SetBossName_Implementation(const FText& BossName)
{
	if (BossNameText) BossNameText->SetText(BossName);
}

void UGolemBossBarWidget::SetBossHealth_Implementation(float Current, float Max)
{
	TargetPercent = (Max > 0.f) ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f;

	if (!bHealthInitialized) { bHealthInitialized = true; DisplayedPercent = TargetPercent; } // first fill: snap
	if (TargetPercent > DisplayedPercent) DisplayedPercent = TargetPercent;                    // a heal shouldn't leave a white ghost above

	RefreshBars();
}

void UGolemBossBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (DisplayedPercent == TargetPercent) return;

	// The white ghost drains steadily down to the real HP — showing the lost chunk shrinking, then gone.
	DisplayedPercent = (DrainSpeed <= 0.f)
		? TargetPercent
		: FMath::FInterpConstantTo(DisplayedPercent, TargetPercent, InDeltaTime, DrainSpeed);

	RefreshBars();
}

void UGolemBossBarWidget::RefreshBars()
{
	if (DamageGhostBar)
	{
		// Chip effect: the front bar = the real current HP (instant); the WHITE ghost behind drains down to meet it.
		if (HealthBar) HealthBar->SetPercent(TargetPercent);
		DamageGhostBar->SetPercent(DisplayedPercent);
	}
	else if (HealthBar)
	{
		HealthBar->SetPercent(DisplayedPercent); // no ghost bar assigned: the single bar itself drains smoothly (fallback)
	}
}
