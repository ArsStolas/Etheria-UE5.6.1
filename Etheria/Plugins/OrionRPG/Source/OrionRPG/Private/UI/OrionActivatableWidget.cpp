// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "UI/OrionActivatableWidget.h"
#include "Input/CommonUIInputTypes.h"
#include "Editor/WidgetCompilerLog.h"
#include "ICommonInputModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OrionActivatableWidget)

#define LOCTEXT_NAMESPACE "OrionRPG"

UOrionActivatableWidget::UOrionActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}


void UOrionActivatableWidget::NativeDestruct()
{
	for (FUIActionBindingHandle Handle : BindingHandles)
	{
		if (Handle.IsValid())
		{
			Handle.Unregister();
		}
	}
	BindingHandles.Empty();

	Super::NativeDestruct();
}

void UOrionActivatableWidget::RegisterBinding(FDataTableRowHandle InputAction, const FInputActionExecutedDelegate& Callback, FOrionInputActionBindingHandle& BindingHandle, FText OverrideDisplayName, const bool bShouldDisplayInActionBar, const bool bConsumeInput)
{
	FBindUIActionArgs BindArgs(InputAction, FSimpleDelegate::CreateLambda([InputAction, Callback]()
		{
			Callback.ExecuteIfBound(InputAction.RowName);
		}));
	BindArgs.bDisplayInActionBar = bShouldDisplayInActionBar;
	BindArgs.OverrideDisplayName = OverrideDisplayName;
	BindArgs.bConsumeInput = bConsumeInput;

	BindingHandle.Handle = RegisterUIActionBinding(BindArgs);
	BindingHandles.Add(BindingHandle.Handle);
}

void UOrionActivatableWidget::UnregisterBinding(FOrionInputActionBindingHandle BindingHandle)
{
	RemoveActionBinding(BindingHandle.Handle);

	if (BindingHandle.Handle.IsValid())
	{
		BindingHandle.Handle.Unregister();
		BindingHandles.Remove(BindingHandle.Handle);
	}
}

void UOrionActivatableWidget::UnregisterAllBindings()
{
	for (FUIActionBindingHandle Handle : BindingHandles)
	{
		Handle.Unregister();
	}
	BindingHandles.Empty();
}

void UOrionActivatableWidget::SetBindingDisplayName(FOrionInputActionBindingHandle BindingHandle, FText NewDisplayName)
{
	if (BindingHandle.Handle.IsValid())
	{
		BindingHandle.Handle.SetDisplayName(NewDisplayName);
	}
}


#if WITH_EDITOR

void UOrionActivatableWidget::ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, class IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledWidgetTree(BlueprintWidgetTree, CompileLog);

	
}

#endif

#undef LOCTEXT_NAMESPACE
