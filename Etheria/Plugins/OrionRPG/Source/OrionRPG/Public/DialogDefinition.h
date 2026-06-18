// Copyright 2025 Ivan Chandra. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogData.h"
#include "Engine/Light.h"
#include "UObject/NoExportTypes.h"
#include "GameFramework/Pawn.h"
#include "DialogDefinition.generated.h"


UCLASS(abstract, Blueprintable, BlueprintType)
class ORIONRPG_API UDialogDefinition : public UObject
{
	GENERATED_BODY()

public:
	UDialogDefinition();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialog Definition", meta = (DisplayPriority = -1))
	FGuid ID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog Definition", meta=(Categories = "Dialog"))
	FGameplayTag Tag;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Dialog Definition")
	FText DisplayName;

public:

	virtual FText GetDisplayName() const { return DisplayName; }
	virtual TSubclassOf<class AActor> GetActorClass() const { return nullptr; }
	virtual void SetActorClass(TSubclassOf<class AActor> InActorClass) { }
	virtual TSoftClassPtr<class AActor> GetActorSoftClass() const { return nullptr; }
	FGuid GetOrCreateID();
};


UCLASS(Blueprintable, BlueprintType)
class ORIONRPG_API UDialogParticipant : public UDialogDefinition
{
	GENERATED_BODY()

public:
	UDialogParticipant();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true", Category = "Dialog Definition", DisplayThumbnail = "true", AllowedClasses = "/Script/Engine.Texture,/Script/Engine.MaterialInterface,/Script/Engine.SlateTextureAtlasInterface", DisallowedClasses = "/Script/MediaAssets.MediaTexture"))
	TObjectPtr<UObject> ParticipantImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog Definition", meta = (DisplayAfter = "DisplayName", DisplayName = "Pawn Class"))
	TSoftClassPtr<class APawn> PawnClassSoft;
	
	UPROPERTY(Transient)
	TSubclassOf<class APawn> PawnClass;

	virtual TSubclassOf<class AActor> GetActorClass() const override { return PawnClass.Get(); }
	virtual void SetActorClass(TSubclassOf<class AActor> InActorClass) override { PawnClass = Cast<UClass>(InActorClass.Get()); }
	virtual TSoftClassPtr<class AActor> GetActorSoftClass() const override { return TSoftClassPtr<AActor>(PawnClassSoft.ToSoftObjectPath()); }
};

UCLASS(Blueprintable, BlueprintType)
class ORIONRPG_API UDialogPlayerParticipant : public UDialogParticipant
{
	GENERATED_BODY()

public:
	UDialogPlayerParticipant();


	virtual FText GetDisplayName() const { return FText::FromString("PLAYER - " + DisplayName.ToString()); }
};


UCLASS(Blueprintable, BlueprintType)
class ORIONRPG_API UDialogProp : public UDialogDefinition
{
	GENERATED_BODY()

public:
	UDialogProp();


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog Definition", meta = (DisplayAfter = "DisplayName", DisplayName = "Prop Class"))
	TSoftClassPtr<class AActor> PropClassSoft;

	UPROPERTY(Transient)
	TSubclassOf<class AActor> PropClass;

	virtual TSubclassOf<class AActor> GetActorClass() const override { return PropClass; }
	virtual void SetActorClass(TSubclassOf<class AActor> InActorClass) override { PropClass = InActorClass; }
	virtual TSoftClassPtr<class AActor> GetActorSoftClass() const override { return PropClassSoft; }
};

UCLASS(Blueprintable, BlueprintType)
class ORIONRPG_API UDialogCamera : public UDialogDefinition
{
	GENERATED_BODY()

public:
	UDialogCamera();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialog Definition", meta = (DisplayAfter = "DisplayName"))
	TSoftClassPtr<class AActor> CameraClassSoft;

	UPROPERTY(Transient)
	TSubclassOf<class AActor> CameraClass;

	virtual TSubclassOf<class AActor> GetActorClass() const override { return CameraClass; }
	virtual void SetActorClass(TSubclassOf<class AActor> InActorClass) override { CameraClass = InActorClass; }
	virtual TSoftClassPtr<class AActor> GetActorSoftClass() const override { return CameraClassSoft; }

};


UCLASS(Blueprintable, BlueprintType)
class ORIONRPG_API UDialogLight : public UDialogDefinition
{
	GENERATED_BODY()

public:
	UDialogLight();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog Definition", meta = (DisplayAfter = "DisplayName", DisplayName = "Light Class"))
	TSoftClassPtr<class ALight> LightClassSoft;

	UPROPERTY(Transient)
	TSubclassOf<class ALight> LightClass;

	virtual TSubclassOf<class AActor> GetActorClass() const override { return LightClass.Get(); }
	virtual void SetActorClass(TSubclassOf<class AActor> InActorClass) override { LightClass = InActorClass; }
	virtual TSoftClassPtr<class AActor> GetActorSoftClass() const override { return TSoftClassPtr<AActor>(LightClassSoft.ToSoftObjectPath()); }

};




