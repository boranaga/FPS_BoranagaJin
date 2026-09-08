#include "SoundSystem/FootstepSoundSet.h"

ESoundID FFootstepSoundSet::FindSoundID(EPhysicalSurface SurfaceType) const
{
	const TEnumAsByte<EPhysicalSurface> SurfaceKey = SurfaceType;

	if (const ESoundID* FoundSoundID = SurfaceSoundMap.Find(SurfaceKey))
	{
		return *FoundSoundID;
	}

	return DefaultSoundID;
}