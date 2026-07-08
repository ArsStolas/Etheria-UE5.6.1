/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "EtheriaCameraShakes" - Source
 */

#include "Camera/EtheriaCameraShakes.h"

#include "Shakes/PerlinNoiseCameraShakePattern.h"

UEtheriaCameraShake_HitLight::UEtheriaCameraShake_HitLight(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Restart the same instance instead of stacking when multiple targets are hit the same frame.
	bSingleInstance = true;

	UPerlinNoiseCameraShakePattern* Pattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("RootShakePattern"));

	Pattern->Duration = 0.22f;
	Pattern->BlendInTime = 0.02f;
	Pattern->BlendOutTime = 0.14f;

	Pattern->Pitch.Amplitude = 1.0f;
	Pattern->Pitch.Frequency = 28.f;
	Pattern->Yaw.Amplitude = 0.8f;
	Pattern->Yaw.Frequency = 24.f;
	Pattern->Roll.Amplitude = 0.5f;
	Pattern->Roll.Frequency = 20.f;

	Pattern->X.Amplitude = 3.0f;
	Pattern->X.Frequency = 24.f;
	Pattern->Y.Amplitude = 2.5f;
	Pattern->Y.Frequency = 22.f;
	Pattern->Z.Amplitude = 2.0f;
	Pattern->Z.Frequency = 26.f;

	SetRootShakePattern(Pattern);
}

UEtheriaCameraShake_HitHeavy::UEtheriaCameraShake_HitHeavy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;

	UPerlinNoiseCameraShakePattern* Pattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("RootShakePattern"));

	Pattern->Duration = 0.42f;
	Pattern->BlendInTime = 0.02f;
	Pattern->BlendOutTime = 0.28f;

	Pattern->Pitch.Amplitude = 2.4f;
	Pattern->Pitch.Frequency = 18.f;
	Pattern->Yaw.Amplitude = 1.8f;
	Pattern->Yaw.Frequency = 16.f;
	Pattern->Roll.Amplitude = 1.4f;
	Pattern->Roll.Frequency = 14.f;

	Pattern->X.Amplitude = 7.0f;
	Pattern->X.Frequency = 18.f;
	Pattern->Y.Amplitude = 5.5f;
	Pattern->Y.Frequency = 16.f;
	Pattern->Z.Amplitude = 5.0f;
	Pattern->Z.Frequency = 18.f;

	Pattern->FOV.Amplitude = 1.5f;
	Pattern->FOV.Frequency = 12.f;

	SetRootShakePattern(Pattern);
}

UEtheriaCameraShake_Land::UEtheriaCameraShake_Land(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;

	UPerlinNoiseCameraShakePattern* Pattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("RootShakePattern"));

	Pattern->Duration = 0.32f;
	Pattern->BlendInTime = 0.01f;
	Pattern->BlendOutTime = 0.22f;

	Pattern->Pitch.Amplitude = 2.2f;
	Pattern->Pitch.Frequency = 15.f;
	Pattern->Roll.Amplitude = 0.6f;
	Pattern->Roll.Frequency = 12.f;

	Pattern->Z.Amplitude = 7.0f;
	Pattern->Z.Frequency = 17.f;
	Pattern->Y.Amplitude = 2.0f;
	Pattern->Y.Frequency = 14.f;

	SetRootShakePattern(Pattern);
}
