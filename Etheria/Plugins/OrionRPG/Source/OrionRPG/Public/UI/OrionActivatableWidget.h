// Copyright 2025 Ivan Chandra. All Rights Reserved.


#pragma once

#include "CommonActivatableWidget.h"
#include <Delegates/DelegateCombinations.h>
#include "OrionActivatableWidget.generated.h"

struct FUIInputConfig;

UENUM(BlueprintType)
enum class EOrionWidgetInputMode : uint8
{
	Default,
	GameAndMenu,
	Game,
	Menu
};

USTRUCT(BlueprintType)
struct FOrionInputActionBindingHandle
{
	GENERATED_BODY()

public:
	struct FUIActionBindingHandle Handle;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FInputActionExecutedDelegate, FName, ActionName);

// An activatable widget that automatically drives the desired input config when activated
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class UOrionActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UOrionActivatableWidget(const FObjectInitializer& ObjectInitializer);

public:

#if WITH_EDITOR
	virtual void ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, class IWidgetCompilerLog& CompileLog) const override;
#endif

protected:
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Orion Activatable Widget")
	void RegisterBinding(FDataTableRowHandle InputAction, const FInputActionExecutedDelegate& Callback, FOrionInputActionBindingHandle& BindingHandle, FText OverrideDisplayName, const bool bShouldDisplayInActionBar = true, const bool bConsumeInput = true);

	UFUNCTION(BlueprintCallable, Category = "Orion Activatable Widget")
	void UnregisterBinding(FOrionInputActionBindingHandle BindingHandle);

	UFUNCTION(BlueprintCallable, Category = "Orion Activatable Widget")
	void UnregisterAllBindings();

	UFUNCTION(BlueprintCallable, Category = "Orion Activatable Widget")
	void SetBindingDisplayName(FOrionInputActionBindingHandle BindingHandle, FText NewDisplayName);
protected:
	/** The desired input mode to use while this UI is activated, for example do you want key presses to still reach the game/player controller? */
	UPROPERTY(EditDefaultsOnly, Category = Input)
	EOrionWidgetInputMode InputConfig = EOrionWidgetInputMode::Default;

	/** The desired mouse behavior when the game gets input. */
	UPROPERTY(EditDefaultsOnly, Category = Input)
	EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;

private:
	TArray<struct FUIActionBindingHandle> BindingHandles;
};
