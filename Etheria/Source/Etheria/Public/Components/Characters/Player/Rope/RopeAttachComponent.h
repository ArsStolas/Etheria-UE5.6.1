#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RopeAttachComponent.generated.h"

class APlayerCharacter;
class ARopeAttachPoint;
class URopeLockComponent;
class UCableComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API URopeAttachComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URopeAttachComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Connecte la corde au point verrouillé */
	void AttachRope(ARopeAttachPoint* TargetPoint);

	/** Détache la corde */
	void DetachRope();

	/** Met à jour visuellement la corde à chaque frame */
	void UpdateRope();

	/** Change le mesh ou le matériel de la corde */
	UFUNCTION(BlueprintCallable, Category="Rope|Visual")
	void SetRopeMesh(USkeletalMesh* NewMesh);

	UFUNCTION(BlueprintCallable, Category="Rope|Visual")
	void SetRopeMaterial(UMaterialInterface* NewMaterial);
	
	bool IsAttached() const { return AttachedPoint.IsValid(); }
	
	FORCEINLINE ARopeAttachPoint* GetAttachedPoint() const { return AttachedPoint.Get(); }

protected:
	/** Le joueur propriétaire */
	APlayerCharacter* OwnerCharacter = nullptr;

	/** Le lock component à écouter */
	URopeLockComponent* LockComponent = nullptr;

	/** Le point actuellement attaché */
	TWeakObjectPtr<ARopeAttachPoint> AttachedPoint;

	/** Le composant visuel (cable ou skeletal mesh) */
	UPROPERTY(VisibleAnywhere, Category="Rope|Visual")
	UCableComponent* CableComponent = nullptr;

	/** Paramètres éditables */
	UPROPERTY(EditAnywhere, Category="Rope|Visual")
	float CableWidth = 5.f;

	UPROPERTY(EditAnywhere, Category="Rope|Visual")
	float CableLengthOffset = 10.f;

	UPROPERTY(EditAnywhere, Category="Rope|Visual")
	UMaterialInterface* RopeMaterial;

	UPROPERTY(EditAnywhere, Category="Rope|Visual")
	USkeletalMesh* RopeMesh;
	
	UPROPERTY(EditDefaultsOnly, Category="Rope")
	FName RopeStartSocketName = TEXT("hand_r");

private:
	/** Callback quand le lock change */
	UFUNCTION()
	void OnLockedPointChanged(ARopeAttachPoint* NewLockedPoint);
};
