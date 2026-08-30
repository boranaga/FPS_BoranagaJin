#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameAudioSettings.generated.h"

class UGameSoundDataAsset;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Game Audio Settings"))
class FPS_BORANAGAJIN_API UGameAudioSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<UGameSoundDataAsset> SoundDataAsset;
};