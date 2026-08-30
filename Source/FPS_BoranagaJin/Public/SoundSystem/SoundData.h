#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "SoundID.h"
#include "SoundData.generated.h"

USTRUCT(BlueprintType)
struct FGameSoundData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<USoundBase> Sound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float VolumeMultiplier = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float PitchMultiplier = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float RandomVolumeRange = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float RandomPitchRange = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<USoundAttenuation> Attenuation = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<USoundConcurrency> Concurrency = nullptr;

public:
    float GetRandomizedVolume() const
    {
        return VolumeMultiplier + FMath::FRandRange(-RandomVolumeRange, RandomVolumeRange);
    }

    float GetRandomizedPitch() const
    {
        return PitchMultiplier + FMath::FRandRange(-RandomPitchRange, RandomPitchRange);
    }
};