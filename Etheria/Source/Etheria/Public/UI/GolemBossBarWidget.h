/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemBossBarWidget - Header"
 * Notes: Boss health-bar widget for the arena Golem fight (name + HP bar). UGolemBossComponent creates / shows /
 *        hides and feeds it automatically — no Event-Graph wiring needed. Reparent a WBP to this class and name a
 *        Text Block "BossNameText" and a Progress Bar "HealthBar" to auto-bind them (both optional).
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GolemBossBarWidget.generated.h"

class UTextBlock;
class UProgressBar;

UCLASS(Abstract, Blueprintable)
class ETHERIA_API UGolemBossBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set the boss title shown on the bar. Override the BP event to skin it; the default fills BossNameText if present. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golem|UI") void SetBossName(const FText& BossName);

	/** Update the HP target from current/max HP. The bar then DRAINS smoothly toward it over time (see DrainSpeed),
	 *  instead of snapping — the first call (fight start) snaps so it doesn't animate down from full. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golem|UI") void SetBossHealth(float Current, float Max);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Auto-bound children (all optional): a Text Block "BossNameText", a Progress Bar "HealthBar" (front, current HP), and a
	 *  Progress Bar "DamageGhostBar" (place it BEHIND HealthBar, fill it WHITE — it shows the just-lost chunk draining down). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> BossNameText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> DamageGhostBar;

	/** Drain speed in bar-fraction per second (steady rate). 0.6 = the whole bar empties in ~1.7 s. 0 = snap instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golem|UI", meta = (ClampMin = "0")) float DrainSpeed = 0.6f;

private:
	void RefreshBars(); // push TargetPercent (front) + DisplayedPercent (white ghost) onto the bars

	float TargetPercent = 1.f;     // the real current HP fraction (front bar, instant)
	float DisplayedPercent = 1.f;  // the white ghost: lags above Target and drains down to it
	bool bHealthInitialized = false;
};
