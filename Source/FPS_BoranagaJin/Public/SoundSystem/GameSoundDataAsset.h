#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoundData.h"
#include "GameSoundDataAsset.generated.h"

UCLASS(BlueprintType)
class FPS_BORANAGAJIN_API UGameSoundDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    const FGameSoundData* FindSoundData(ESoundID SoundID) const;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    TMap<ESoundID, FGameSoundData> SoundDataMap;
};