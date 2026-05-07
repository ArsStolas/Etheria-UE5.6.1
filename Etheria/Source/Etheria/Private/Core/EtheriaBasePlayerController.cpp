/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: EtheriaPlayerController - Source
*/

#include "Core/EtheriaBasePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Core/System/Notifications/NotificationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Interfaces/Widgets/NotificationWidget.h"
#include "Tests/TestCheatManager.h"

AEtheriaBasePlayerController::AEtheriaBasePlayerController()
{
	CheatClass = UTestCheatManager::StaticClass();
}

void AEtheriaBasePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (!NotificationWidgetClass)
	{
		UE_LOG(LogNotificationSystem, Warning, TEXT("[Notification] NotificationWidgetClass is not set on %s"), *GetName());
		return;
	}

	NotificationWidget = CreateWidget<UNotificationWidget>(this, NotificationWidgetClass);
	if (!NotificationWidget)
	{
		UE_LOG(LogNotificationSystem, Warning, TEXT("[Notification] Failed to create notification widget on %s"), *GetName());
		return;
	}

	NotificationWidget->AddToViewport();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UNotificationSubsystem* NotificationSubsystem = LocalPlayer->GetSubsystem<UNotificationSubsystem>())
		{
			NotificationSubsystem->SetMaxVisibleNotifications(MaxVisibleNotifications);
			NotificationSubsystem->RegisterNotificationWidget(NotificationWidget);
		}
	}
}
