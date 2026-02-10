/**
 * Etheria's End Project, 2025
 * Created by: "0nnen"
 * Last Updated by: "0nnen"
 * Class: "UWindAudioComponent" - Header
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Audio/ModularSurface/Data/WindAudioProfile.h"
#include "WindAudioComponent.generated.h"

UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class ETHERIA_API UWindAudioComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWindAudioComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
    TObjectPtr<UWindAudioProfile> Profile = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
    bool bAutoEnableWhileFalling = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind")
    bool bForceEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind", meta = (ClampMin = "0.0"))
    float FadeInSeconds = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wind", meta = (ClampMin = "0.0"))
    float FadeOutSeconds = 0.20f;

    UFUNCTION(BlueprintCallable, Category = "Wind")
    void SetEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Wind")
    bool IsEnabled() const { return bEnabled; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    bool bEnabled = false;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> WindAudioComponent = nullptr;

    FTimerHandle UpdateTimerHandle;

    void EnsureAudioComponent();
    void StartTimer();
    void StopTimer();
    void UpdateWind();

    float EvalCurve(const UCurveFloat* Curve, float X, float DefaultValue) const;
};
