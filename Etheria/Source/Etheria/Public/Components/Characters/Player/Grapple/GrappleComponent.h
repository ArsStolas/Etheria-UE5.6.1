/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: GrappleComponent - Header
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
#include "CableComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/Grapple/GrapplePointActor.h"
#include "GrappleComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGrappleComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	//////////////////////////////////////////////////////////////
	// Détection
	//////////////////////////////////////////////////////////////
	void CheckForGrapplePoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple")
	float MaxGrappleDistance = 2000.f;

	UPROPERTY(BlueprintReadOnly)
	AGrapplePointActor* CurrentGrapplePoint = nullptr;

	//////////////////////////////////////////////////////////////
	// Input
	//////////////////////////////////////////////////////////////
	void ToggleGrapple();
	void DetachGrapple();

	UPROPERTY(BlueprintReadOnly)
	bool bIsGrappling = false;

	//////////////////////////////////////////////////////////////
	// Swing
	//////////////////////////////////////////////////////////////
	void HandleSwing(float DeltaTime);

	// Force utilisée pour ramener le joueur vers la longueur maximale du câble
	// Plus la valeur est élevée, plus le câble est rigide (moins élastique)
	// Trop haut = effet "rebond" violent / trop bas = câble mou
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Swing", meta=(ClampMin="1000", ClampMax="50000"))
	float SwingForce = 10000.f;

	// Force appliquée par les inputs du joueur (ZQSD / WASD) pendant le swing
	// Permet d’influencer la direction du mouvement et de prendre de la vitesse
	// Plus haut = contrôle plus fort dans les airs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grapple|Swing", meta=(ClampMin="1000", ClampMax="200000"))
	float SwingControlForce = 50000.f;

	// Force de pumping appliquée tangentiellement au swing
	// Sert à générer de l’inertie comme sur une balançoire
	// Plus haut = gain de vitesse plus rapide quand le timing est bon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grapple|Swing", meta=(ClampMin="500", ClampMax="100000"))
	float PumpForce = 5000.f;

private:
	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr;

	UPROPERTY()
	UCableComponent* Cable = nullptr;

	FVector GrappleLocation;
	float CableLength = 0.f;
};
