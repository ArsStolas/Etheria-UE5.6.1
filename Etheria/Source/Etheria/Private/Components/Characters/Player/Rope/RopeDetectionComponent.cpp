// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Characters/Player/Rope/RopeDetectionComponent.h"

#include "Characters/Players/PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "World/Rope/RopeAttachPoint.h"

URopeDetectionComponent::URopeDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URopeDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<APlayerCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
}

void URopeDetectionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	DetectAttachPoint();
}

void URopeDetectionComponent::DetectAttachPoint()
{
	if (!OwnerCharacter || !Camera) return;

	// --- Cache caméra (IMPORTANT) ---
	CachedCameraLocation = Camera->GetComponentLocation();
	CachedCameraForward = Camera->GetForwardVector();

	float BestScore = -FLT_MAX;
	ARopeAttachPoint* BestPoint = nullptr;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARopeAttachPoint::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		ARopeAttachPoint* Point = Cast<ARopeAttachPoint>(Actor);
		if (!Point) continue;

		FString FailReason;
		bool bValid = IsValidPoint(Point, FailReason);

		// --- Debug points ---
		if (bDebugDraw)
		{
			FColor Color = bValid ? FColor::Yellow : FColor::Red;
			DrawDebugSphere(GetWorld(), Point->GetActorLocation(), 25.f, 8, Color, false, 0.f);
			if (!bValid)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RopeDetection] Point %s invalid: %s"), *Point->GetName(), *FailReason);
			}
		}

		if (!bValid) continue;

		// --- Scoring ---
		const FVector ToPoint = Point->GetActorLocation() - CachedCameraLocation;
		const float Distance = ToPoint.Size();
		const float Dot = FVector::DotProduct(CachedCameraForward, ToPoint.GetSafeNormal());

		// Pondération : angle prioritaire
		float Score = Dot * 2000.f + (1.f - Distance / MaxDetectionDistance) * 500.f;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestPoint = Point;
		}
	}

	CurrentPoint = BestPoint;

	// --- Debug BestPoint ---
	if (bDebugDraw && BestPoint)
	{
		DrawDebugSphere(
			GetWorld(),
			BestPoint->GetActorLocation(),
			BestPoint->DetectionRadius,
			16,
			FColor::Green,
			false,
			0.f,
			0,
			2.f
		);
	}

	// --- Debug cone ---
	if (bDebugDraw)
	{
		const float ConeLength = MaxDetectionDistance;
		const float ConeHalfAngleRad = FMath::DegreesToRadians(DetectionHalfAngle);

		DrawDebugCone(
			GetWorld(),
			CachedCameraLocation,
			CachedCameraForward,
			ConeLength,
			ConeHalfAngleRad,
			ConeHalfAngleRad,
			16,
			FColor::Cyan,
			false,
			0.f,
			0,
			1.5f
		);
	}
}

bool URopeDetectionComponent::IsValidPoint(ARopeAttachPoint* Point, FString& OutFailReason) const
{
	OutFailReason = "";

	if (!Point)
	{
		OutFailReason = "Null Point";
		return false;
	}

	if (!OwnerCharacter)
	{
		OutFailReason = "Invalid Owner";
		return false;
	}

	UCharacterStateComponent* StateComp = OwnerCharacter->GetStateComponent();
	if (!StateComp)
	{
		OutFailReason = "No StateComponent";
		return false;
	}

	if (StateComp->IsInMovementState(EtheriaTags::State_Movement_Airborne_Diving))
	{
		OutFailReason = "Diving";
		return false;
	}

	if (StateComp->IsInLifeState(EtheriaTags::State_Life_Dead))
	{
		OutFailReason = "Dead";
		return false;
	}

	// --- Camera context ---
	const FVector CameraLoc = CachedCameraLocation;
	const FVector CameraForward = CachedCameraForward;
	const FVector PointLoc = Point->GetActorLocation();
	const FVector PlayerLoc = OwnerCharacter->GetActorLocation();

	// Distance max caméra
	const float CamDistance = FVector::Dist(CameraLoc, PointLoc);
	if (CamDistance > MaxDetectionDistance)
	{
		OutFailReason = "Too Far (Camera)";
		return false;
	}

	// Angle caméra
	const FVector ToPoint = (PointLoc - CameraLoc).GetSafeNormal();
	const float Dot = FVector::DotProduct(CameraForward, ToPoint);
	if (Dot < MinCameraDot)
	{
		OutFailReason = "Outside Camera Cone";
		return false;
	}

	// Line of Sight vers le centre du point (sphere)
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);
	Params.AddIgnoredActor(Point);

	const FVector TargetLoc = PointLoc + FVector(0, 0, Point->DetectionRadius * 0.5f);

	bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, CameraLoc, TargetLoc, ECC_Visibility, Params);
	if (bBlocked)
	{
		if (bDebugDraw)
		{
			DrawDebugLine(GetWorld(), CameraLoc, TargetLoc, FColor::Orange, false, 0.f, 0, 1.f);
		}
		OutFailReason = "LOS Blocked";
		return false;
	}

	// Hauteur minimale relative au joueur (pas la caméra)
	const float HeightDelta = PointLoc.Z - PlayerLoc.Z;
	if (HeightDelta < MinHeightAboveCamera)
	{
		OutFailReason = "Too Low (Player)";
		return false;
	}

	return true;
}