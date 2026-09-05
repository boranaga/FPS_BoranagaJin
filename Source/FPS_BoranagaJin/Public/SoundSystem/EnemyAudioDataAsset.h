#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SoundID.h"
#include "EnemyAudioEvent.h"
#include "EnemyAudioDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FEnemySoundVariation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	ESoundID SoundID = ESoundID::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float Weight = 1.f;
};

USTRUCT(BlueprintType)
struct FEnemyAudioEventData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TArray<FEnemySoundVariation> Variations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PlayProbability = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float CooldownMin = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float CooldownMax = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0"))
	int32 Priority = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	bool bInterruptLowerPriority = false;

	//--------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambient", meta = (ClampMin = "0.1"))
	float AmbientIntervalMin = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambient", meta = (ClampMin = "0.1"))
	float AmbientIntervalMax = 7.f;
};

UCLASS(BlueprintType)
class FPS_BORANAGAJIN_API UEnemyAudioDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	const FEnemyAudioEventData* FindEventData(EEnemyAudioEvent AudioEvent) const;
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Audio", meta = (AllowPrivateAccess = "true"))
	TMap<EEnemyAudioEvent, FEnemyAudioEventData> EventDataMap;
};