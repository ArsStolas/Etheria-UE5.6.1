#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DialogData.h"
#include "OrionDialogInterface.generated.h"

UINTERFACE(BlueprintType)
class ORIONRPG_API UOrionDialogInterface : public UInterface
{
	GENERATED_BODY()
};

class ORIONRPG_API IOrionDialogInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Orion|Dialog")
	void OnDialogLineStarted(const FOrionDialogLine& DialogLine);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Orion|Dialog")
	void OnDialogLineEnded(const FOrionDialogLine& DialogLine);
};


/**
* Dialog context for sequence context object purpose.
*/

class SWidget;
class UDialogDefinition;
class AActor;

UINTERFACE()
class ORIONRPG_API UDialogContext : public UInterface
{
	GENERATED_BODY()
};

class ORIONRPG_API IDialogContext
{
	GENERATED_BODY()

public:
	virtual void AddDialogToViewport(class UUserWidget* InWidget) = 0;
	virtual void RemoveDialogFromViewport() = 0;
	virtual AActor* GetDialogDefinitionActor(UDialogDefinition* InDialogDefinition) = 0;
};