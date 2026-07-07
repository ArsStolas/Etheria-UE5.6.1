/**
 * Etheria's End Project, 2025
 * Created by: ArsStolas
 * Last Updated by: ArsStolas
 * Class: "GolemCrystal - Source"
 */

#include "Characters/AI/Boss/GolemCrystal.h"

#include "Characters/AI/Boss/GolemBossComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"

AGolemCrystal::AGolemCrystal()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only ticks while rising out of the ground

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	// SOLID: BLOCKS the player so you can't walk through it (the project's "Pawn" profile blocks Pawn, so a Pawn-typed
	// blocker stops the player capsule), while ECC_Pawn keeps it found by the player's melee object-trace. Camera +
	// Visibility are left ignored so the crystal never shoves the camera or blocks line-of-sight / aim traces.
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetCollisionObjectType(ECC_Pawn);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	MeshComp->SetGenerateOverlapEvents(false);
}

void AGolemCrystal::BeginPlay()
{
	Super::BeginPlay();

	// Climb out of the ground instead of popping in: start buried, then ease up to the planted spot in Tick.
	if (RiseHeight > 0.f && RiseDuration > 0.f)
	{
		RiseTargetLoc = GetActorLocation();
		SetActorLocation(RiseTargetLoc - FVector(0.f, 0.f, RiseHeight));
		RiseElapsed = 0.f;
		bRising = true;
		SetActorTickEnabled(true);
	}
}

void AGolemCrystal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bRising) return;

	RiseElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(RiseElapsed / FMath::Max(0.01f, RiseDuration), 0.f, 1.f);
	const float Eased = FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f); // fast out of the ground, settle at the top

	FVector Loc = RiseTargetLoc;
	Loc.Z = FMath::Lerp(RiseTargetLoc.Z - RiseHeight, RiseTargetLoc.Z, Eased);
	SetActorLocation(Loc);

	if (Alpha >= 1.f) { bRising = false; SetActorTickEnabled(false); }
}

void AGolemCrystal::InitAsArenaCrystal(UGolemBossComponent* InOwnerComp, int32 InCrystalId)
{
	OwnerComp = InOwnerComp;
	CrystalId = InCrystalId;
	bIsBig = false;
	OnCrystalSpawned(false); // SPAWN anim hook
}

void AGolemCrystal::InitAsBigCrystal(UGolemBossComponent* InOwnerComp)
{
	OwnerComp = InOwnerComp;
	CrystalId = -1;
	bIsBig = true;
	OnCrystalSpawned(true); // SPAWN anim hook
}

void AGolemCrystal::NotifyDamaged(int32 InHitsRemaining, int32 InHitsTotal)
{
	if (bShattered) return;
	const int32 Total = FMath::Max(1, InHitsTotal);
	const float Frac = FMath::Clamp(1.f - (float)FMath::Max(0, InHitsRemaining) / (float)Total, 0.f, 1.f);
	OnCrystalDamaged(InHitsRemaining, Total, Frac); // A-BIT-BROKEN anim hook
}

void AGolemCrystal::Shatter()
{
	if (bShattered) return;
	bShattered = true;

	SetActorEnableCollision(false);   // can't be hit again
	OnCrystalShattered();             // FULLY-BROKEN anim hook (BP plays the break anim/VFX; mesh stays for it to animate)
	SetLifeSpan(FMath::Max(0.05f, ShatterLifetime)); // destroyed after the anim — pointer stays valid for in-flight BP hit logic
}

float AGolemCrystal::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	UGolemBossComponent* Comp = OwnerComp.Get();
	if (!Comp || DamageAmount <= 0.f) return Applied;

	AActor* HitInstigator = DamageCauser ? DamageCauser : (EventInstigator ? EventInstigator->GetPawn() : nullptr);

	// Route the hit to the boss brain — it calls back NotifyDamaged() (a-bit-broken) or Shatter() (broken) to drive the anim.
	if (bIsBig) Comp->HitBigCrystal(DamageAmount, HitInstigator);
	else        Comp->HitArenaCrystal(CrystalId, DamageAmount, HitInstigator);

	return DamageAmount;
}
