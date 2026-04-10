// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "Input/CommonBoundActionButton.h"

#include "OrionBoundActionButton.generated.h"

class UCommonButtonStyle;
class UObject;

/**
 *
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class ORIONRPG_API UOrionBoundActionButton : public UCommonBoundActionButton
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

private:
	void HandleInputMethodChanged(ECommonInputType NewInputMethod);

	UPROPERTY(EditAnywhere, Category = "Styles")
	TSubclassOf<UCommonButtonStyle> KeyboardStyle;

	UPROPERTY(EditAnywhere, Category = "Styles")
	TSubclassOf<UCommonButtonStyle> GamepadStyle;

	UPROPERTY(EditAnywhere, Category = "Styles")
	TSubclassOf<UCommonButtonStyle> TouchStyle;
};
