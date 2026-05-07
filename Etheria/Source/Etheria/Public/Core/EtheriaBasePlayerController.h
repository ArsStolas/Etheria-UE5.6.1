/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: EtheriaPlayerController - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EtheriaBasePlayerController.generated.h"

UCLASS()
class ETHERIA_API AEtheriaBasePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEtheriaBasePlayerController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Notifications")
	TSubclassOf<class UNotificationWidget> NotificationWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Notifications", meta=(ClampMin="1"))
	int32 MaxVisibleNotifications = 3;

private:
	UPROPERTY(Transient)
	TObjectPtr<class UNotificationWidget> NotificationWidget;
};
