#include "SoundSystem/GameSoundDataAsset.h"

const FGameSoundData* UGameSoundDataAsset::FindSoundData(ESoundID SoundID) const
{
    return SoundDataMap.Find(SoundID);
}