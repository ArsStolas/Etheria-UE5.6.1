/**
 * Etheria's End Project, 2025
 * Created by: Zhailendra
 * Class: EtheriaBaseGameMode - Source
*/

#include "Core/EtheriaBaseGameMode.h"

AEtheriaBaseGameMode::AEtheriaBaseGameMode()
{
	static ConstructorHelpers::FClassFinder<APlayerController> PCBPClass(TEXT("/Game/Game_Settings/Controllers/BP_EtheriaBasePlayerController"));
	if (PCBPClass.Succeeded())
	{
		PlayerControllerClass = PCBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/Characters/Players/BP_PlayerCharacter"));
	if (PlayerPawnBPClass.Succeeded())
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AEtheriaBaseGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld() && (GetWorld()->IsNetMode(NM_Standalone) || HasAuthority()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] BaseGameMode initialized"));
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] PlayerController: %s"), *PlayerControllerClass->GetName());
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] PlayerCharacter: %s"), *DefaultPawnClass->GetName());
	}
}