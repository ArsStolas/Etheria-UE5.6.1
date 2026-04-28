// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "UI/Foundation/OrionButtonBase.h"
#include "Components/CanvasPanelSlot.h"
#include "CommonActionWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OrionButtonBase)

void UOrionButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	UpdateButtonStyle();
	RefreshButtonText();
	bIsFocused = false;
}

void UOrionButtonBase::UpdateInputActionWidget()
{
	Super::UpdateInputActionWidget();

	UpdateButtonStyle();
	RefreshButtonText();
}

void UOrionButtonBase::SetButtonText(const FText& InText)
{
	bOverride_ButtonText = InText.IsEmpty();
	ButtonText = InText;
	RefreshButtonText();
}

void UOrionButtonBase::RefreshButtonText()
{
	if (bOverride_ButtonText || ButtonText.IsEmpty())
	{
		if (InputActionWidget)
		{
			const FText ActionDisplayText = InputActionWidget->GetDisplayText();
			if (!ActionDisplayText.IsEmpty())
			{
				UpdateButtonText(ActionDisplayText);
				return;
			}
		}
	}

	UpdateButtonText(ButtonText);
}

void UOrionButtonBase::SetHoldData(float InHoldTime, float InHoldRollbackTime)
{
	HoldTime = InHoldTime;
	HoldRollbackTime = InHoldRollbackTime;
}


void UOrionButtonBase::UpdateHoldData(ECommonInputType CurrentInputType)
{
	Super::UpdateHoldData(CurrentInputType);
	
	K2_OnUpdateHoldData();
	
}


void UOrionButtonBase::OnInputMethodChanged(ECommonInputType CurrentInputType)
{
	Super::OnInputMethodChanged(CurrentInputType);

	UpdateButtonStyle();
}

void UOrionButtonBase::NativeOnHovered()
{
	Super::NativeOnHovered();
	SetFocus();
}

void UOrionButtonBase::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bIsFocused = true;
}

void UOrionButtonBase::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bIsFocused = false;
}

void UOrionButtonBase::SetZOrder(int32 InZOrder)
{
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetZOrder(InZOrder);
	}
}