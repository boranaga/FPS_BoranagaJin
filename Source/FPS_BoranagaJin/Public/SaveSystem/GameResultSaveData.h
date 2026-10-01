#pragma once

#include "CoreMinimal.h"
#include "GameEndReason.h"
#include "GameResultSaveData.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FGameResultSaveData
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FGuid ResultID;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FString OriginalSlotName;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	EGameEndReason EndReason = EGameEndReason::None;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FName FinalLevelName = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	double TotalPlayTimeSeconds = 0.0;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FDateTime CompletedAt;

public:
	bool IsValid() const
	{
		return ResultID.IsValid() && !OriginalSlotName.IsEmpty() && EndReason != EGameEndReason::None;
	}

	FString GetFormattedPlayTime() const
	{
		const int64 TotalSeconds = FMath::Max<int64>(0, static_cast<int64>(TotalPlayTimeSeconds));

		const int64 Hours = TotalSeconds / 3600;
		const int64 Minutes = (TotalSeconds % 3600) / 60;
		const int64 Seconds = TotalSeconds % 60;

		return FString::Printf(TEXT("%02lld:%02lld:%02lld"), Hours, Minutes, Seconds);
	}
};
