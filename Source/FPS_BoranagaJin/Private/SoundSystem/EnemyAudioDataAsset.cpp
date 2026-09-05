
#include "SoundSystem/EnemyAudioDataAsset.h"

const FEnemyAudioEventData* UEnemyAudioDataAsset::FindEventData(EEnemyAudioEvent AudioEvent) const
{
	if (AudioEvent == EEnemyAudioEvent::None)
	{
		return nullptr;
	}

	return EventDataMap.Find(AudioEvent);
}