// Copyright 2025 Ivan Chandra. All Rights Reserved.


#include "Decorator/OrionDecorator.h"
#include "UObject/UObjectIterator.h"
#include "QuestComponent.h"
#include "Quest.h"
#include "QuestBuilderGraph.h"
#include "QuestBuilderNode.h"
#include "QuestBuilderNode_Root.h"
#include "QuestBuilderFunctionLibrary.h"	
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "OrionDecorator"

UOrionDecorator::UOrionDecorator()
{
	bIsNode = false;
}

bool UOrionDecorator::PerformConditionCheck_Implementation(APlayerController* OwnerController, APawn* ControlledPawn) const
{
	return true;
}

FString UOrionDecorator::GetNodeDisplayText_Implementation() const
{
	return GetName();
}


FString UOrionDecorator::GetShortTag(FGameplayTag Tag, int32 Level) const
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

FString UOrionDecorator::GetNodeName() 
{
	return NodeName.Len() ? NodeName : GetShortTypeName(this);
}

FString UOrionDecorator::GetShortTypeName(UObject* Ob)
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

UWorld* UOrionDecorator::GetWorld() const
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
void UOrionDecorator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
#undef LOCTEXT_NAMESPACE