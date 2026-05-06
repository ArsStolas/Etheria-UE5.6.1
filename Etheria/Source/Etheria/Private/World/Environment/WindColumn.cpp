/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: WindColumn - Source
*/

#include "World/Environment/WindColumn.h"

#include "Characters/Players/PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/Characters/CharacterStateComponent.h"
#include "Core/System/EtheriaGameplayTags.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogWindColumn, Log, All);

#if UE_BUILD_SHIPPING
	#define WIND_COLUMN_LOG(Verbosity, Format, ...)
	#define WIND_COLUMN_SCREEN(Key, Color, Format, ...)
#else
	#define WIND_COLUMN_LOG(Verbosity, Format, ...) \
		if (bWindDebugMode) UE_LOG(LogWindColumn, Verbosity, Format, ##__VA_ARGS__)
	#define WIND_COLUMN_SCREEN(Key, Color, Format, ...) \
		if (bWindDebugMode && GEngine) GEngine->AddOnScreenDebugMessage(Key, 0.1f, Color, FString::Printf(Format, ##__VA_ARGS__))
#endif

AWindColumn::AWindColumn()
{
	PrimaryActorTick.bCanEverTick = true;

	WindArea = CreateDefaultSubobject<UBoxComponent>(TEXT("WindArea"));
	RootComponent = WindArea;
	WindArea->InitBoxExtent(FVector(200.f, 200.f, 500.f));
	WindArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WindArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	WindArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WindArea->SetHiddenInGame(true);
	WindArea->SetVisibility(false);

	WindArea->OnComponentBeginOverlap.AddDynamic(this, &AWindColumn::OnBeginOverlap);
	WindArea->OnComponentEndOverlap.AddDynamic(this, &AWindColumn::OnEndOverlap);

	ColumnNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ColumnNiagara"));
	ColumnNiagaraComponent->SetupAttachment(RootComponent);
	ColumnNiagaraComponent->SetAbsolute(true, true, true);
	ColumnNiagaraComponent->SetAutoActivate(false);
	ColumnNiagaraComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ColumnNiagaraComponent->SetHiddenInGame(false);
	ColumnNiagaraComponent->SetVisibility(true, true);

	ExitRingNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ExitRingNiagara"));
	ExitRingNiagaraComponent->SetupAttachment(RootComponent);
	ExitRingNiagaraComponent->SetAbsolute(true, true, true);
	ExitRingNiagaraComponent->SetAutoActivate(false);
	ExitRingNiagaraComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExitRingNiagaraComponent->SetHiddenInGame(false);
	ExitRingNiagaraComponent->SetVisibility(true, true);
}

void AWindColumn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ConfigureVisuals();
}

void AWindColumn::BeginPlay()
{
	Super::BeginPlay();

	ConfigureVisuals();

	WIND_COLUMN_LOG(Log, TEXT("[WindColumn] Debug enabled | Extent=(%.0f, %.0f, %.0f) Lift=%.0f ExitBoost=%.0f"),
		GetColumnScaledExtent().X,
		GetColumnScaledExtent().Y,
		GetColumnScaledExtent().Z,
		LiftForce,
		ExitBoostForce);
}

void AWindColumn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bWindDebugMode)
	{
		return;
	}

	DrawWindDebug();

	WIND_COLUMN_SCREEN(24, ActiveTimers.Num() > 0 ? FColor::Green : FColor::Orange,
		TEXT("[WindColumn] Players:%d | ColumnFX:%s | RingFX:%s | Lift:%.0f | Boost:%.0f"),
		ActiveTimers.Num(),
		ColumnNiagaraSystem ? TEXT("OK") : TEXT("NONE"),
		ExitRingNiagaraSystem ? TEXT("OK") : TEXT("NONE"),
		LiftForce,
		ExitBoostForce);
}

void AWindColumn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (TPair<APlayerCharacter*, FTimerHandle>& ActiveTimer : ActiveTimers)
	{
		GetWorldTimerManager().ClearTimer(ActiveTimer.Value);
	}

	ActiveTimers.Empty();
	BoostedThisStay.Empty();

	Super::EndPlay(EndPlayReason);
}

void AWindColumn::ConfigureVisuals()
{
	if (WindArea)
	{
		WindArea->SetHiddenInGame(!bWindDebugMode);
		WindArea->SetVisibility(bWindDebugMode);
	}

	ConfigureColumnNiagara();
	ConfigureExitRingNiagara();
}

void AWindColumn::ConfigureColumnNiagara() const
{
	if (!ColumnNiagaraComponent)
	{
		return;
	}

	if (!ColumnNiagaraSystem || !WindArea)
	{
		ColumnNiagaraComponent->DeactivateImmediate();
		ColumnNiagaraComponent->SetVisibility(false, true);
		return;
	}

	const FVector ScaledExtent = GetColumnScaledExtent();
	const FVector BoxSize(
		FMath::Max(ScaledExtent.Z * 2.f, 1.f),
		FMath::Max(ScaledExtent.X * 2.f * ColumnVisualRadiusMultiplier, 1.f),
		FMath::Max(ScaledExtent.Y * 2.f * ColumnVisualRadiusMultiplier, 1.f));

	const float Radius = FMath::Max(ScaledExtent.X, ScaledExtent.Y) * ColumnVisualRadiusMultiplier;

	ColumnNiagaraComponent->SetAsset(ColumnNiagaraSystem);
	ColumnNiagaraComponent->SetWorldLocation(WindArea->GetComponentLocation());
	ColumnNiagaraComponent->SetWorldRotation(WindArea->GetUpVector().Rotation());
	ColumnNiagaraComponent->SetWorldScale3D(FVector(ColumnNiagaraComponentScale));
	ColumnNiagaraComponent->SetVisibility(true, true);
	ColumnNiagaraComponent->SetHiddenInGame(false);

	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.ColumnHeight"), ScaledExtent.Z * 2.f);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.ColumnRadius"), Radius);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.StreamRadius"), Radius);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.InnerRadius"), Radius);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.FlowSpeed"), LiftForce);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.LiftForce"), LiftForce);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.ColumnOpacity"), ColumnOpacity);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.StreamOpacity"), ColumnOpacity);
	ColumnNiagaraComponent->SetVariablePosition(TEXT("User.ColumnCenter"), WindArea->GetComponentLocation());
	ColumnNiagaraComponent->SetVariablePosition(TEXT("User.ColumnTop"), GetColumnWorldTop());
	ColumnNiagaraComponent->SetVariableVec3(TEXT("User.StreamDirection"), WindArea->GetUpVector());

	ColumnNiagaraComponent->SetVariableVec3(TEXT("User.Box Size"), BoxSize);
	ColumnNiagaraComponent->SetVariableVec3(TEXT("User.Box_Size"), BoxSize);
	ColumnNiagaraComponent->SetVariableVec3(TEXT("User.BoxSize"), BoxSize);
	ColumnNiagaraComponent->SetVariableVec3(TEXT("User.Wind_Box Size"), BoxSize);
	ColumnNiagaraComponent->SetVariableVec3(TEXT("User.Wind_Box_Size"), BoxSize);
	ColumnNiagaraComponent->SetVariableVec3(TEXT("User.WindBoxSize"), BoxSize);
	ColumnNiagaraComponent->SetVariableVec3(TEXT("User.Leaves_Box Size"), BoxSize);
	ColumnNiagaraComponent->SetVariableVec3(TEXT("User.Leaves_Box_Size"), BoxSize);
	ColumnNiagaraComponent->SetVariableVec3(TEXT("User.LeavesBoxSize"), BoxSize);

	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.Wind_Spawn Rate"), ColumnWindSpawnRate);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.Wind_Spawn_Rate"), ColumnWindSpawnRate);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.WindSpawnRate"), ColumnWindSpawnRate);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.Leaves_Spawn Rate"), ColumnLeavesSpawnRate);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.Leaves_Spawn_Rate"), ColumnLeavesSpawnRate);
	ColumnNiagaraComponent->SetVariableFloat(TEXT("User.LeavesSpawnRate"), ColumnLeavesSpawnRate);

	ColumnNiagaraComponent->Activate(true);
}

void AWindColumn::ConfigureExitRingNiagara() const
{
	if (!ExitRingNiagaraComponent)
	{
		return;
	}

	if (!ExitRingNiagaraSystem || !WindArea)
	{
		ExitRingNiagaraComponent->DeactivateImmediate();
		ExitRingNiagaraComponent->SetVisibility(false, true);
		return;
	}

	const FVector ScaledExtent = GetColumnScaledExtent();
	const float RingRadius = FMath::Max(ScaledExtent.X, ScaledExtent.Y) * ExitRingRadiusMultiplier;
	const float RingDiameter = FMath::Max(RingRadius * 2.f, 1.f);
	const FVector RingBoxSize(20.f, RingDiameter, RingDiameter);
	const float RingWidthScale = FMath::Max(RingRadius / FMath::Max(ExitRingReferenceRadius, 1.f), 0.01f);
	const float FinalRingScale = ColumnNiagaraComponentScale * ExitRingNiagaraScale * RingWidthScale;
	const FRotator BaseRotation = WindArea->GetUpVector().Rotation();

	ExitRingNiagaraComponent->SetAsset(ExitRingNiagaraSystem);
	ExitRingNiagaraComponent->SetWorldLocation(GetExitRingWorldLocation());
	ExitRingNiagaraComponent->SetWorldRotation(FRotator(
		BaseRotation.Pitch + ExitRingRotationOffset.Pitch,
		BaseRotation.Yaw + ExitRingRotationOffset.Yaw,
		BaseRotation.Roll + ExitRingRotationOffset.Roll));
	ExitRingNiagaraComponent->SetWorldScale3D(FVector(FinalRingScale));
	ExitRingNiagaraComponent->SetVisibility(true, true);
	ExitRingNiagaraComponent->SetHiddenInGame(false);

	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.RingRadius"), RingRadius);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.RingDiameter"), RingDiameter);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.StreamRadius"), RingRadius);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.InnerRadius"), RingRadius);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.BoundaryRadius"), RingRadius);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.BoundaryRingRadius"), RingRadius);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.BoundaryRingDiameter"), RingDiameter);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.BoundaryOpacity"), ExitRingOpacity);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.BoundaryRingOpacity"), ExitRingOpacity);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.RingScale"), FinalRingScale);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.RingWidthScale"), RingWidthScale);
	ExitRingNiagaraComponent->SetVariableFloat(TEXT("User.RingRadiusMultiplier"), ExitRingRadiusMultiplier);
	ExitRingNiagaraComponent->SetVariableVec3(TEXT("User.StreamDirection"), WindArea->GetUpVector());
	ExitRingNiagaraComponent->SetVariablePosition(TEXT("User.RingCenter"), GetExitRingWorldLocation());
	ExitRingNiagaraComponent->SetVariablePosition(TEXT("User.BoundaryRingCenter"), GetExitRingWorldLocation());
	ExitRingNiagaraComponent->SetVariableVec3(TEXT("User.Box Size"), RingBoxSize);
	ExitRingNiagaraComponent->SetVariableVec3(TEXT("User.Box_Size"), RingBoxSize);
	ExitRingNiagaraComponent->SetVariableVec3(TEXT("User.BoxSize"), RingBoxSize);
	ExitRingNiagaraComponent->SetVariableVec3(TEXT("User.Ring_Box Size"), RingBoxSize);
	ExitRingNiagaraComponent->SetVariableVec3(TEXT("User.Ring_Box_Size"), RingBoxSize);
	ExitRingNiagaraComponent->SetVariableVec3(TEXT("User.RingBoxSize"), RingBoxSize);

	ExitRingNiagaraComponent->Activate(true);
}

void AWindColumn::DrawWindDebug() const
{
	if (!GetWorld() || !WindArea)
	{
		return;
	}

	const FVector Center = WindArea->GetComponentLocation();
	const FVector Extent = GetColumnScaledExtent();
	const FVector Top = GetColumnWorldTop();
	const FVector RingCenter = GetExitRingWorldLocation();
	const float RingRadius = FMath::Max(Extent.X, Extent.Y) * ExitRingRadiusMultiplier;

	DrawDebugBox(
		GetWorld(),
		Center,
		Extent,
		WindArea->GetComponentQuat(),
		FColor::Cyan,
		false,
		0.f,
		0,
		DebugDrawThickness);

	DrawDebugDirectionalArrow(
		GetWorld(),
		Center,
		Top,
		80.f,
		FColor::Green,
		false,
		0.f,
		0,
		DebugDrawThickness);

	DrawDebugCircle(
		GetWorld(),
		RingCenter,
		RingRadius,
		48,
		FColor::Blue,
		false,
		0.f,
		0,
		DebugDrawThickness,
		WindArea->GetRightVector(),
		WindArea->GetForwardVector(),
		false);

	const FString DebugText = FString::Printf(
		TEXT("WindColumn\nNiagara:%s Ring:%s\nPlayers:%d Lift:%.0f Damping:%.2f\nBoost:%.0f Margin:%.0f Interval:%.3f"),
		ColumnNiagaraSystem ? TEXT("OK") : TEXT("NONE"),
		ExitRingNiagaraSystem ? TEXT("OK") : TEXT("NONE"),
		ActiveTimers.Num(),
		LiftForce,
		VerticalDamping,
		ExitBoostForce,
		ExitTopMargin,
		GetSafeApplyInterval());
	DrawDebugString(GetWorld(), RingCenter + WindArea->GetUpVector() * 120.f, DebugText, nullptr, FColor::White, 0.f, false);
}

void AWindColumn::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
								 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
								 const FHitResult& SweepResult)
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	if (ActiveTimers.Contains(Player))
	{
		return;
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, Player]()
		{
			if (!IsValid(Player))
			{
				if (FTimerHandle* Handle = ActiveTimers.Find(Player))
				{
					GetWorldTimerManager().ClearTimer(*Handle);
				}
				ActiveTimers.Remove(Player);
				BoostedThisStay.Remove(Player);
				return;
			}

			ApplyLift(Player);
		}),
		GetSafeApplyInterval(),
		true);

	ActiveTimers.Add(Player, TimerHandle);

	WIND_COLUMN_LOG(Log, TEXT("[WindColumn] ENTER %s"), *Player->GetName());
}

void AWindColumn::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
							   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	if (FTimerHandle* Handle = ActiveTimers.Find(Player))
	{
		GetWorldTimerManager().ClearTimer(*Handle);
		ActiveTimers.Remove(Player);
	}

	const FVector PlayerLoc = Player->GetActorLocation();
	const float DistanceFromTop = FVector::DotProduct(PlayerLoc - GetColumnWorldTop(), WindArea->GetUpVector());

	if (DistanceFromTop > -ExitTopMargin && DistanceFromTop < ExitTopMargin)
	{
		UCharacterMovementComponent* MoveComp = Player->GetCharacterMovement();
		if (MoveComp)
		{
			FVector Vel = MoveComp->Velocity;
			Vel.Z += ExitBoostForce;
			MoveComp->Velocity = Vel;

			WIND_COLUMN_LOG(Log, TEXT("[WindColumn] EXIT BOOST %s | VelZ=%.1f"), *Player->GetName(), Vel.Z);

			if (bWindDebugMode)
			{
				DrawDebugString(GetWorld(), PlayerLoc + FVector(0.f, 0.f, 100.f), TEXT("Exit Boost!"), nullptr, FColor::Cyan, 2.f);
			}
		}
	}

	BoostedThisStay.Remove(Player);
}

void AWindColumn::ApplyLift(APlayerCharacter* Player)
{
	if (!Player || !Player->GetStateComponent())
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = Player->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	bool bCanLift = false;

	if (!bAffectOnlyGliding)
	{
		bCanLift = true;
	}
	else
	{
		const FGameplayTag CurrentMovement = Player->GetStateComponent()->CurrentMovementState;
		bCanLift = CurrentMovement == EtheriaTags::State_Movement_Airborne_Gliding ||
				   CurrentMovement == EtheriaTags::State_Movement_Airborne_Diving;
	}

	if (!bCanLift)
	{
		return;
	}

	FVector Vel = MoveComp->Velocity;

	if (Vel.Z < 0.f)
	{
		Vel.Z *= (1.f - VerticalDamping);
	}

	Vel.Z += LiftForce * GetSafeApplyInterval();
	Vel.Z = FMath::Clamp(Vel.Z, -500.f, 2500.f);

	MoveComp->Velocity = Vel;

	const FVector PlayerLoc = Player->GetActorLocation();
	const float DistanceFromTop = FVector::DotProduct(PlayerLoc - GetColumnWorldTop(), WindArea->GetUpVector());

	if (DistanceFromTop >= -ExitTopMargin && !BoostedThisStay.Contains(Player))
	{
		Vel = MoveComp->Velocity;
		Vel.Z += ExitBoostForce;
		MoveComp->Velocity = Vel;

		BoostedThisStay.Add(Player);

		WIND_COLUMN_LOG(Log, TEXT("[WindColumn] TOP BOOST %s | VelZ=%.1f"), *Player->GetName(), Vel.Z);

		if (bWindDebugMode)
		{
			DrawDebugString(GetWorld(), PlayerLoc + FVector(0.f, 0.f, 100.f), TEXT("Top Boost!"), nullptr, FColor::Cyan, 1.f);
		}
	}
	else if (DistanceFromTop < -ExitTopMargin && BoostedThisStay.Contains(Player))
	{
		BoostedThisStay.Remove(Player);
	}
}

float AWindColumn::GetSafeApplyInterval() const
{
	return FMath::Max(ApplyInterval, 0.001f);
}

FVector AWindColumn::GetColumnScaledExtent() const
{
	return WindArea ? WindArea->GetScaledBoxExtent() : FVector::ZeroVector;
}

FVector AWindColumn::GetColumnWorldTop() const
{
	if (!WindArea)
	{
		return GetActorLocation();
	}

	const FVector LocalTop = FVector(0.f, 0.f, WindArea->GetUnscaledBoxExtent().Z);
	return WindArea->GetComponentTransform().TransformPosition(LocalTop);
}

FVector AWindColumn::GetExitRingWorldLocation() const
{
	if (!WindArea)
	{
		return GetActorLocation();
	}

	const FVector LocalRingLocation = FVector(0.f, 0.f, WindArea->GetUnscaledBoxExtent().Z + ExitRingVerticalOffset);
	return WindArea->GetComponentTransform().TransformPosition(LocalRingLocation);
}
