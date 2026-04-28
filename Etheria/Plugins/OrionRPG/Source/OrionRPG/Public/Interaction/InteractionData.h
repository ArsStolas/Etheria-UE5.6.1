// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InteractionData.generated.h"

UENUM(BlueprintType)
enum class EOrionInteractionType : uint8
{
	/**Interaction trace from camera*/
	E_InteractionCamera		UMETA(DisplayName = "Camera"),
	/**Interaction trace from pawn*/
	E_InteractionPawn		UMETA(DisplayName = "Pawn"),
};
UENUM(BlueprintType)
enum class EOrionInteractionResult : uint8
{
	E_None			UMETA(DisplayName = "None"),
	E_Started		UMETA(DisplayName = "Started"),
	E_Completed		UMETA(DisplayName = "Completed"),
	E_Canceled		UMETA(DisplayName = "Canceled"),
	E_Blocked		UMETA(DisplayName = "Blocked")
};

UENUM(BlueprintType)
enum class EOrionInteractionNetMode : uint8
{
	/**Default, will notify only in server side*/
	E_Server		UMETA(DisplayName = "Server(Host)"),
	/**Notify client only for interaction result*/
	E_OwnerOnly	UMETA(DisplayName = "OwnerOnly(Client)"),
	/**Using multicast to notify interaction result*/
	E_All		UMETA(DisplayName = "All(Multicast)")
};


UENUM(BlueprintType)
enum class EOrionInteractionMode : uint8
{
	None	UMETA(DisplayName = "None"),
	Instant	UMETA(DisplayName = "Instant"),
	Hold	UMETA(DisplayName = "Hold")
};

UENUM(BlueprintType)
enum class EOrionInteractionWidgetType : uint8
{
	E_WorldSpace	UMETA(DisplayName = "World Space"),
	E_ScreenSpace	UMETA(DisplayName = "Screen Space")
};
