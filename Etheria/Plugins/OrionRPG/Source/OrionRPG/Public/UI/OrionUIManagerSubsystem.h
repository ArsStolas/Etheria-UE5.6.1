// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Widgets/CommonActivatableWidgetContainer.h" 
#include "Framework/Application/NavigationConfig.h"
#include "OrionUIManagerSubsystem.generated.h"


UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_UI_Layer_Game);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_UI_Layer_GameMenu);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_UI_Layer_Menu);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_UI_Layer_Modal);

class ORIONRPG_API FOrionNavigationConfig : public FNavigationConfig
{
public:
	FOrionNavigationConfig() {
		KeyEventRules.Reset();
		//WASD causing the the enhanced input not working the first time playing. need to find a way to register and unregister whenever system try to open menu.
		/*KeyEventRules.Emplace(EKeys::A, EUINavigation::Left);
		KeyEventRules.Emplace(EKeys::D, EUINavigation::Right);
		KeyEventRules.Emplace(EKeys::W, EUINavigation::Up);
		KeyEventRules.Emplace(EKeys::S, EUINavigation::Down);*/
		KeyEventRules.Emplace(EKeys::Left, EUINavigation::Left);
		KeyEventRules.Emplace(EKeys::Right, EUINavigation::Right);
		KeyEventRules.Emplace(EKeys::Up, EUINavigation::Up);
		KeyEventRules.Emplace(EKeys::Down, EUINavigation::Down);
		KeyEventRules.Emplace(EKeys::Gamepad_DPad_Left, EUINavigation::Left);
		KeyEventRules.Emplace(EKeys::Gamepad_DPad_Right, EUINavigation::Right);
		KeyEventRules.Emplace(EKeys::Gamepad_DPad_Up, EUINavigation::Up);
		KeyEventRules.Emplace(EKeys::Gamepad_DPad_Down, EUINavigation::Down);

		KeyEventRules.Emplace(EKeys::Gamepad_LeftShoulder, EUINavigation::Previous);
		KeyEventRules.Emplace(EKeys::Gamepad_RightShoulder, EUINavigation::Next);
		KeyEventRules.Emplace(EKeys::X, EUINavigation::Previous);
		KeyEventRules.Emplace(EKeys::C, EUINavigation::Next);
	}
};

class UOrionPrimaryGameLayout;
class UOrionGridContainerBase;

UCLASS()
class ORIONRPG_API UOrionUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UOrionUIManagerSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "UI Subsystem")
	TObjectPtr<UOrionPrimaryGameLayout> PrimaryLayout;


private:
	TSharedRef<FOrionNavigationConfig> OrionNavigationConfig;
};

class ULocalPlayer;


UCLASS()
class ORIONRPG_API UOrionUILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "OrionLibrary|UI")
	static FName SuspendInputForPlayer(APlayerController* PlayerController, FName SuspendReason);

	static FName SuspendInputForPlayer(ULocalPlayer* LocalPlayer, FName SuspendReason);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "OrionLibrary|UI")
	static void ResumeInputForPlayer(APlayerController* PlayerController, FName SuspendToken);

	static void ResumeInputForPlayer(ULocalPlayer* LocalPlayer, FName SuspendToken);

	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintCosmetic, Category = "OrionLibrary|UI", meta = (WorldContext = "WorldContextObject"))
	static UCommonActivatableWidgetContainerBase* GetLayerWidget(const UObject* WorldContextObject, UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag LayerName);


	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintCosmetic, Category = "OrionLibrary|UI", meta = (WorldContext = "WorldContextObject"))
	static class UCommonActivatableWidget* GetActiveWidgetFromLayer(const UObject* WorldContextObject, UPARAM(meta = (Categories = "UI.Layer")) FGameplayTag LayerName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|UI", meta = (WorldContext = "WorldContextObject"))
	static UOrionUIManagerSubsystem* GetOrionUISubsystem(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OrionLibrary|UI")
	static UWidget* GetFocusedWidget();
	
	
private:
	static int32 InputSuspensions;
};