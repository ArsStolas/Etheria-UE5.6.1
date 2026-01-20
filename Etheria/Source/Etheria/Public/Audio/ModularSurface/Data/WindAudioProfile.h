/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UWindAudioProfile" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Curves/CurveFloat.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "WindAudioProfile.generated.h"

UCLASS(BlueprintType)
class ETHERIA_API UWindAudioProfile : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
    TObjectPtr<USoundBase> WindLoop = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
    TObjectPtr<UCurveFloat> VolumeCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
    TObjectPtr<UCurveFloat> PitchCurve = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind", meta = (ClampMin = "0.0"))
    float MinSpeed = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind", meta = (ClampMin = "0.01"))
    float UpdateInterval = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
    TObjectPtr<USoundAttenuation> Attenuation = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
    FName SpeedParam = "Speed";

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
    FName ForwardDotParam = "ForwardDot";

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
    FName RightDotParam = "RightDot";
};
