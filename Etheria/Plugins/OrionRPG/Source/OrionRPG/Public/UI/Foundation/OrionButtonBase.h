// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CommonButtonBase.h"

#include "OrionButtonBase.generated.h"

class UObject;
struct FFrame;

UCLASS(Abstract, BlueprintType, Blueprintable)
class UOrionButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "OrionRPG|CommonButton")
	void SetButtonText(const FText& InText);

protected:
	// UUserWidget interface
	virtual void NativePreConstruct() override;
	// End of UUserWidget interface

	// UCommonButtonBase interface
	virtual void UpdateInputActionWidget() override;
	virtual void OnInputMethodChanged(ECommonInputType CurrentInputType) override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	// End of UCommonButtonBase interface

	void RefreshButtonText();

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateButtonText(const FText& InText);

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateButtonStyle();
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Update Hold Data")
	void K2_OnUpdateHoldData();
	
	UFUNCTION(BlueprintCallable, Category = "OrionRPG|UI")
	void SetHoldData(float InHoldTime, float InHoldRollbackTime);

	/**Current Common Button will always override the hold data using this function, so us k2_OnUpdateHoldData -> then Set Hold Data Manually*/
	virtual void UpdateHoldData(ECommonInputType CurrentInputType) override;

	UFUNCTION(BlueprintCallable, Category = "OrionRPG|UI")
	void SetZOrder(int32 InZOrder);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionRPG|UI")
	bool IsButtonFocused() const { return bIsFocused; }

private:
	UPROPERTY(EditAnywhere, Category = "Button", meta = (InlineEditConditionToggle))
	uint8 bOverride_ButtonText : 1;

	UPROPERTY(EditAnywhere, Category = "Button", meta = (editcondition = "bOverride_ButtonText"))
	FText ButtonText;

	UPROPERTY(EditAnywhere, Category = "Button")
	bool bIsFocused;

};

