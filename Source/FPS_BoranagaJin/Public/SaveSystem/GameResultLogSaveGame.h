#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveSystem/GameResultSaveData.h"
#include "GameResultLogSaveGame.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API UGameResultLogSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FGameResultSaveData> GameResults;
};