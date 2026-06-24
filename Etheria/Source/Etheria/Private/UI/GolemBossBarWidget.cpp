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

	if (!bHealthInitialized)
	{
		// First fill (fight start): snap so the bar doesn't visibly drain down from full on spawn.
		bHealthInitialized = true;
		DisplayedPercent = TargetPercent;
		if (HealthBar) HealthBar->SetPercent(DisplayedPercent);
	}
}

void UGolemBossBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!HealthBar || DisplayedPercent == TargetPercent) return;

	// Ease the bar toward the real HP at a steady rate so a big hit drains "little by little" instead of snapping.
	DisplayedPercent = (DrainSpeed <= 0.f)
		? TargetPercent
		: FMath::FInterpConstantTo(DisplayedPercent, TargetPercent, InDeltaTime, DrainSpeed);

	HealthBar->SetPercent(DisplayedPercent);
}
