#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveDataTypes.h"
#include "SaveGameCustom.generated.h"

UCLASS()
class FPS_BORANAGAJIN_API USaveGameCustom : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 SaveVersion = 1;

    UPROPERTY()
    FName SavedLevelName;

    UPROPERTY()
    FName CheckpointID;

    UPROPERTY()
    FPlayerSaveData PlayerData;

    UPROPERTY()
    TArray<FWorldActorSaveData> WorldActorData;

    UPROPERTY()
    TSet<FName> CompletedTutorialIDs;

    UPROPERTY()
    FDateTime SavedAt;
};