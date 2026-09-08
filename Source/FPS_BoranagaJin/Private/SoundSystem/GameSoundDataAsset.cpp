#include "SoundSystem/GameSoundDataAsset.h"

const FGameSoundData* UGameSoundDataAsset::FindSoundData(ESoundID SoundID) const
{
	if (SoundID == ESoundID::None) { return nullptr; }
    return SoundDataMap.Find(SoundID);
}

ESoundID UGameSoundDataAsset::FindFootstepSoundID(FName FootstepProfile, EPhysicalSurface SurfaceType) const
{
	if (FootstepProfile.IsNone())
	{
		return ESoundID::None;
	}

	const FFootstepSoundSet* SoundSet = FootstepSoundSets.Find(FootstepProfile);

	if (!SoundSet)
	{
		return ESoundID::None;
	}

	return SoundSet->FindSoundID(SurfaceType);
}