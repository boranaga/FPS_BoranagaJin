#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveDataTypes.h"
#include "SaveGameCustom.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API UFPSGameSave : public USaveGame
{
	GENERATED_BODY()
public:
	UFPSGameSave();
public:
	/**
	 * 저장 데이터 형식이 변경될 때 증가시킵니다.
	 */
	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 SaveVersion = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FName SavedLevelName = NAME_None;

	//UPROPERTY(SaveGame, BlueprintReadOnly)
	//FName CheckpointID = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FCheckpointSaveData CheckpointData;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FPlayerSaveData PlayerData;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FWorldActorSaveData> WorldActorData;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FWorldActorSaveData> RuntimeSpawnedWorldActorData;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TSet<FName> CompletedTutorialIDs;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FDateTime SavedAt;
public:
	const FWorldActorSaveData* FindActorData(const FGuid& SaveID) const
	{
		return WorldActorData.FindByPredicate(
			[&SaveID](const FWorldActorSaveData& Data)
			{
				return Data.InstanceID == SaveID;
			}
		);
	}

	void ResetProgress()
	{
		SavedLevelName = NAME_None;
		//CheckpointID = NAME_None;
		CheckpointData.Reset();
		PlayerData.Reset();
		WorldActorData.Reset();
		CompletedTutorialIDs.Reset();
		SavedAt = FDateTime();
	}
};