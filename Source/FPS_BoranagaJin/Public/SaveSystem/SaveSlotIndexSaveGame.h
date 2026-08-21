#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveSystem/SaveSlotInfo.h"
#include "SaveSlotIndexSaveGame.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API USaveSlotIndexSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FSaveSlotInfo> Slots;
};