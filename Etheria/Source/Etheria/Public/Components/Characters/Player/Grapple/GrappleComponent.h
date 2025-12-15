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

	void DetachGrappleWithJump();

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
	float PumpForce = 2500.f;

	// Distance à partir de laquelle le câble commence à devenir rigide
	// Plus bas = câble dur plus tôt
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grapple|Swing", meta=(ClampMin="0.7", ClampMax="1.0"))
	float CableStiffnessStartRatio = 0.9f;

	// Damping appliqué quand le câble est tendu
	// Plus haut = moins de rebonds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grapple|Swing", meta=(ClampMin="0.01", ClampMax="0.2"))
	float CableDamping = 0.08f;

	// Vitesse maximale autorisée pendant le swing (cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grapple|Swing", meta=(ClampMin="800", ClampMax="6000"))
	float MaxSwingSpeed = 1000.f;

	// Damping appliqué quand aucun input n'est utilisé pendant le swing
	// Plus bas = ralentissement brutal, plus haut = ralentissement doux
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grapple|Swing", meta=(ClampMin="0.5", ClampMax="0.99"))
	float NoInputSwingDamping = 0.965f;

	// Ralentissement appliqué au moment où on lâche le grappin
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grapple|Swing", meta=(ClampMin="0.3", ClampMax="1.0"))
	float DetachVelocityPreserveRatio = 0.85f;

	// Boost appliqué quand on saute en étant accroché (cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grapple|Detach", meta=(ClampMin="200", ClampMax="3000"))
	float JumpDetachBoost = 1200.f;

	// Direction du boost de saut (mélange avant / tangent)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grapple|Detach", meta=(ClampMin="0.0", ClampMax="1.0"))
	float JumpForwardBias = 0.4f;

	// Distance minimale au sol pour autoriser un detach par jump (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grapple|Detach", meta=(ClampMin="50", ClampMax="500"))
	float MinDetachHeightFromGround = 150.f;

	// Vitesse minimale requise pour autoriser le jump boost (cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grapple|Detach", meta=(ClampMin="100", ClampMax="2000"))
	float MinSwingSpeedForBoost = 550.f;

private:
	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr;

	UPROPERTY()
	UCableComponent* Cable = nullptr;

	FVector GrappleLocation;
	float CableLength = 0.f;

	void ApplyNoInputDamping(UCharacterMovementComponent* MoveComp, const FVector& GrappleLocation, float DeltaTime);
};
