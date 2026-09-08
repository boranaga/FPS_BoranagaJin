#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoundData.h"
#include "FootstepSoundSet.h"
#include "GameSoundDataAsset.generated.h"

UCLASS(BlueprintType)
class FPS_BORANAGAJIN_API UGameSoundDataAsset : public UDataAsset
{
    GENERATED_BODY()
public:
    const FGameSoundData* FindSoundData(ESoundID SoundID) const;
    ESoundID FindFootstepSoundID(FName FootstepProfile, EPhysicalSurface SurfaceType) const;
private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (AllowPrivateAccess = "true"))
    TMap<ESoundID, FGameSoundData> SoundDataMap;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footstep", meta = (AllowPrivateAccess = "true"))
    TMap<FName, FFootstepSoundSet> FootstepSoundSets;
};