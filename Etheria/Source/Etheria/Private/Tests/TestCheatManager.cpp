/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: TestCheatManager - Source
*/

#include "Tests/TestCheatManager.h"

#include "Core/System/Notifications/NotificationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Tests/CharacterComponents/HealthTestManager.h"

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT

void UTestCheatManager::InitCheatManager()
{
	Super::InitCheatManager();

	if (UHealthTestManager* HealthManager = NewObject<UHealthTestManager>(this))
	{
		HealthManager->Initialize(this);
		TestManagers.Add("Health", HealthManager);
	}
}

UBaseTestManager* UTestCheatManager::GetManager(const FString& ManagerName) const
{
	if (TestManagers.Contains(ManagerName))
	{
		return TestManagers[ManagerName];
	}
	UE_LOG(LogTemp, Warning, TEXT("[CheatManager] Manager '%s' not found"), *ManagerName);
	return nullptr;
}

void UTestCheatManager::Test_ApplyDamage(const FString& TargetName, float DamageAmount)
{
	if (const auto Manager = Cast<UHealthTestManager>(GetManager("Health")))
	{
		Manager->Test_ApplyDamage(TargetName, DamageAmount);
	}
}

void UTestCheatManager::Test_Heal(const FString& TargetName, float HealAmount)
{
	if (const auto Manager = Cast<UHealthTestManager>(GetManager("Health")))
	{
		Manager->Test_HealByName(TargetName, HealAmount);
	}
}

void UTestCheatManager::Test_ResetHealth(const FString& TargetName)
{
	if (const auto Manager = Cast<UHealthTestManager>(GetManager("Health")))
	{
		Manager->Test_ResetHealth(TargetName);
	}
}

void UTestCheatManager::Test_LogHealthStats(const FString& TargetName)
{
	if (const auto Manager = Cast<UHealthTestManager>(GetManager("Health")))
	{
		Manager->Test_LogHealthStats(TargetName);
	}
}

void UTestCheatManager::Test_Notify(const FString& Title, const FString& Message)
{
	if (APlayerController* PlayerController = GetOuterAPlayerController())
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UNotificationSubsystem* NotificationSubsystem = LocalPlayer->GetSubsystem<UNotificationSubsystem>())
			{
				FNotificationPayload Payload;
				Payload.Title = FText::FromString(Title);
				Payload.Message = FText::FromString(Message);
				Payload.Type = ENotificationType::Info;
				Payload.Priority = ENotificationPriority::Normal;
				Payload.Duration = 3.0f;
				NotificationSubsystem->ShowNotification(Payload);
			}
		}
	}
}

void UTestCheatManager::Test_NotifyWarning(const FString& Message)
{
	if (APlayerController* PlayerController = GetOuterAPlayerController())
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UNotificationSubsystem* NotificationSubsystem = LocalPlayer->GetSubsystem<UNotificationSubsystem>())
			{
				FNotificationPayload Payload;
				Payload.Title = FText::FromString(TEXT("Warning"));
				Payload.Message = FText::FromString(Message);
				Payload.Type = ENotificationType::Warning;
				Payload.Priority = ENotificationPriority::High;
				Payload.Duration = 4.0f;
				NotificationSubsystem->ShowNotification(Payload);
			}
		}
	}
}

void UTestCheatManager::Test_NotifyQuest(const FString& Message)
{
	if (APlayerController* PlayerController = GetOuterAPlayerController())
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UNotificationSubsystem* NotificationSubsystem = LocalPlayer->GetSubsystem<UNotificationSubsystem>())
			{
				FNotificationPayload Payload;
				Payload.Title = FText::FromString(TEXT("Quest"));
				Payload.Message = FText::FromString(Message);
				Payload.Type = ENotificationType::Quest;
				Payload.Priority = ENotificationPriority::High;
				Payload.Duration = 5.0f;
				NotificationSubsystem->ShowNotification(Payload);
			}
		}
	}
}

void UTestCheatManager::Test_NotifyClear()
{
	if (APlayerController* PlayerController = GetOuterAPlayerController())
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UNotificationSubsystem* NotificationSubsystem = LocalPlayer->GetSubsystem<UNotificationSubsystem>())
			{
				NotificationSubsystem->ClearAllNotifications();
			}
		}
	}
}

#endif
