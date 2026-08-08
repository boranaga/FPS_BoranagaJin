#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveSystem/SaveDataTypes.h"
#include "SaveablePlayerInterface.generated.h"

UINTERFACE(MinimalAPI)
class USaveablePlayerInterface : public UInterface
{
	GENERATED_BODY()
};

class FPS_BORANAGAJIN_API ISaveablePlayerInterface
{
	GENERATED_BODY()
public:
	// const version
	virtual void WritePlayerSaveData(FPlayerSaveData& OutSaveData) const = 0;

	//// non-const version
	//virtual void WritePlayerSaveData(FPlayerSaveData& OutSaveData) = 0;

	virtual void LoadPlayerSaveData(const FPlayerSaveData& SaveData) = 0;
};