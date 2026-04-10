// Copyright 2025 Ivan Chandra. All Rights Reserved.

#include "Event/OrionEvent.h"
#include "UObject/UObjectIterator.h"
#include "OrionRPG.h"
#include "QuestComponent.h"
#include "Quest.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderNode_Objective.h"
#include "QuestBuilderNode_Root.h"
#include "QuestBuilderFunctionLibrary.h"	
#include "AssetRegistry/AssetRegistryModule.h"


UOrionEvent::UOrionEvent()
{
	bSetupCompleted = false;
	bPendingToStart = false;
	bLaunchEventOnLoad = false;
	bIsNode = false;
}

void UOrionEvent::BeginSetup(APlayerController* OwnerController, APawn* ControlledPawn)
{
	//UE_LOG(LogTemp, Log, TEXT("Begin Setup for event - %s"), *GetNodeDisplayText());
	bSetupCompleted = false;
	OwningController = OwnerController;
	OwningPawn = ControlledPawn;
	K2_BeginSetup(OwnerController, ControlledPawn);
}

void UOrionEvent::K2_BeginSetup_Implementation(APlayerController* OwnerController, APawn* ControlledPawn)
{
	FinishSetup();
}

void UOrionEvent::BeginEvent(APlayerController* OwnerController, APawn* ControlledPawn)
{
	OwningController = OwnerController;
	OwningPawn = ControlledPawn;
	if (!bSetupCompleted)
	{
		bPendingToStart = true;
		return;
	}
	K2_BeginEvent(OwnerController, ControlledPawn);
	bPendingToStart = false;
}

void UOrionEvent::FinishSetup()
{
	//UE_LOG(LogTemp, Log, TEXT("Finish Setup for event - %s"), *GetNodeDisplayText());
	bSetupCompleted = true;
	if (bPendingToStart)
	{
		BeginEvent(OwningController, OwningPawn);
	}
}

void UOrionEvent::EndEvent()
{
	OnEventFinished.Broadcast();
	K2_EventFinished();
}

FString UOrionEvent::GetNodeDisplayText_Implementation() const
{
	return "Node Description";
}


FString UOrionEvent::GetNodeName()
{
	return NodeName.Len() ? NodeName : GetShortTypeName(this);
}

FString UOrionEvent::GetShortTypeName(UObject* Ob)
{
	if ((Ob == nullptr) || (Ob->GetClass() == nullptr))
	{
		return TEXT("None");
	}

	FString TypeDesc = Ob->GetClass()->GetName();

	if (Ob->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{

		TypeDesc = Ob->GetClass()->GetName().LeftChop(2);
	}

	// Insert space before each capital letter (except the first character) and remove underscores
	// Do not add space if the previous character is uppercase (abbreviation)
	FString Result;
	for (int32 i = 0; i < TypeDesc.Len(); ++i)
	{
		if (TypeDesc[i] != TEXT('_'))
		{
			TCHAR Char = TypeDesc[i];
			if (i > 0 && FChar::IsUpper(Char) && !FChar::IsWhitespace(TypeDesc[i - 1]))
			{
				// Only add space if previous character is not uppercase (not abbreviation)
				if (!FChar::IsUpper(TypeDesc[i - 1]))
				{
					Result += TEXT(" ");
				}
			}
			Result += Char;
		}
	}

	return Result;
}

FString UOrionEvent::GetShortTag(FGameplayTag Tag, int32 Level) const
{
	FString TagString = Tag.ToString();
	TArray<FString> Parts;
	TagString.ParseIntoArray(Parts, TEXT("."), true);

	if (Level <= 0 || Parts.Num() == 0)
	{
		return TagString;
	}

	int32 StartIndex = FMath::Max(Parts.Num() - Level, 0);
	FString Result;
	for (int32 i = StartIndex; i < Parts.Num(); ++i)
	{
		if (!Result.IsEmpty())
		{
			Result += TEXT(".");
		}
		Result += Parts[i];
	}
	return Result;
}

UWorld* UOrionEvent::GetWorld() const
{
	if (HasAllFlags(RF_ClassDefaultObject))
	{
		// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
		return nullptr;
	}


	UObject* Outer = GetOuter();

	while (Outer)
	{
		UWorld* World = Outer->GetWorld();
		if (World)
		{
			return World;
		}

		Outer = Outer->GetOuter();
	}

	return nullptr;
}

#if WITH_EDITOR
void UOrionEvent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
}
#endif
