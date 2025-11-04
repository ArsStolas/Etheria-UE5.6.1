/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Last Updated by: Zhailendra
 * Class: WindColumn - Source
*/

#include "World/Environment/WindColumn.h"
#include "Components/BoxComponent.h"
#include "Characters/Players/PlayerCharacter.h"
#include "Components/Characters/Player/Glider/GliderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

AWindColumn::AWindColumn()
{
    PrimaryActorTick.bCanEverTick = false;

    WindArea = CreateDefaultSubobject<UBoxComponent>(TEXT("WindArea"));
    RootComponent = WindArea;
    WindArea->InitBoxExtent(FVector(200.f, 200.f, 500.f));
    WindArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    WindArea->SetCollisionResponseToAllChannels(ECR_Ignore);
    WindArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    WindArea->SetHiddenInGame(false);
    WindArea->SetVisibility(true);

    WindArea->OnComponentBeginOverlap.AddDynamic(this, &AWindColumn::OnBeginOverlap);
    WindArea->OnComponentEndOverlap.AddDynamic(this, &AWindColumn::OnEndOverlap);

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(RootComponent);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetHiddenInGame(false);
    VisualMesh->SetVisibility(true);
}

void AWindColumn::BeginPlay()
{
    Super::BeginPlay();

    DrawDebugBox(
        GetWorld(),
        GetActorLocation(),
        WindArea->GetScaledBoxExtent(),
        FColor::Cyan,
        true,
        -1.f,
        0,
        4.f
    );
}

void AWindColumn::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                 const FHitResult& SweepResult)
{
    APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
    if (!Player) return;

    if (ActiveTimers.Contains(Player))
        return;

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(
        TimerHandle,
        FTimerDelegate::CreateWeakLambda(this, [this, Player]()
        {
            if (!Player || !IsValid(Player))
            {
                GetWorldTimerManager().ClearTimer(ActiveTimers[Player]);
                ActiveTimers.Remove(Player);
                BoostedThisStay.Remove(Player);
                return;
            }

            ApplyLift(Player);
        }),
        ApplyInterval,
        true
    );

    ActiveTimers.Add(Player, TimerHandle);
}

void AWindColumn::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
    if (!Player) return;

    if (FTimerHandle* Handle = ActiveTimers.Find(Player))
    {
        GetWorldTimerManager().ClearTimer(*Handle);
        ActiveTimers.Remove(Player);
    }

    const FVector PlayerLoc = Player->GetActorLocation();
    const FVector BoxCenter = WindArea->GetComponentLocation();
    const FVector Extent = WindArea->GetScaledBoxExtent();
    const float TopZ = BoxCenter.Z + Extent.Z;
    const float DistanceFromTop = PlayerLoc.Z - TopZ;

    if (DistanceFromTop > -ExitTopMargin && DistanceFromTop < ExitTopMargin)
    {
        UCharacterMovementComponent* MoveComp = Player->GetCharacterMovement();
        if (MoveComp)
        {
            FVector Vel = MoveComp->Velocity;
            Vel.Z += ExitBoostForce;
            MoveComp->Velocity = Vel;

            DrawDebugString(GetWorld(), PlayerLoc + FVector(0,0,100), TEXT("Exit Boost!"), nullptr, FColor::Cyan, 2.f);
        }
    }

    BoostedThisStay.Remove(Player);
}

void AWindColumn::ApplyLift(APlayerCharacter* Player)
{
    if (!Player) return;

    UCharacterMovementComponent* MoveComp = Player->GetCharacterMovement();
    if (!MoveComp) return;

    bool bCanLift = !bAffectOnlyGliding || Player->IsInSpecialMode();
    if (!bCanLift) return;

    FVector Vel = MoveComp->Velocity;

    if (Vel.Z < 0.f)
        Vel.Z *= (1.f - VerticalDamping);

    Vel.Z += LiftForce * ApplyInterval;
    Vel.Z = FMath::Clamp(Vel.Z, -500.f, 2500.f);

    MoveComp->Velocity = Vel;

    const FVector PlayerLoc = Player->GetActorLocation();
    const FVector BoxCenter = WindArea->GetComponentLocation();
    const FVector Extent = WindArea->GetScaledBoxExtent();
    const float TopZ = BoxCenter.Z + Extent.Z;

    if (PlayerLoc.Z >= TopZ - ExitTopMargin && !BoostedThisStay.Contains(Player))
    {
        Vel = MoveComp->Velocity;
        Vel.Z += ExitBoostForce;
        MoveComp->Velocity = Vel;

        BoostedThisStay.Add(Player);

        DrawDebugString(GetWorld(), PlayerLoc + FVector(0,0,100), TEXT("Top Boost!"), nullptr, FColor::Cyan, 1.f);
    }
    else if (PlayerLoc.Z < TopZ - ExitTopMargin && BoostedThisStay.Contains(Player))
    {
        BoostedThisStay.Remove(Player);
    }
}