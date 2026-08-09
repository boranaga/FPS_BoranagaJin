#pragma once

#include "CoreMinimal.h"
#include "SaveSystem/InventorySaveData.h"
#include "SaveDataTypes.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FPlayerSaveData
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame)
    FTransform PlayerTransform = FTransform::Identity;

    UPROPERTY(SaveGame)
    float CurrentHealth = 100.f;

    UPROPERTY(SaveGame)
    TArray<FInventorySlotSaveData> ItemInventorySlots;

    UPROPERTY(SaveGame)
    TArray<FInventorySlotSaveData> WeaponInventorySlots;

    UPROPERTY(SaveGame)
    TArray<FInventorySlotSaveData> ThrowableWeaponInventorySlots;

    UPROPERTY(SaveGame)
    int32 EquippedWeaponIndex = INDEX_NONE;

    //UPROPERTY(SaveGame, BlueprintReadOnly)
    //FName EquippedWeaponID = NAME_None; //TODO: 굳이?

    void Reset()
    {
        PlayerTransform = FTransform::Identity;
        CurrentHealth = 0.0f;
        ItemInventorySlots.Reset();
        EquippedWeaponIndex = INDEX_NONE;
        //EquippedWeaponID = NAME_None;
    }
};

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FWorldActorSaveData
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame, BlueprintReadOnly)
    FGuid InstanceID;

    UPROPERTY(SaveGame)
    TSoftClassPtr<AActor> ActorClass;

    UPROPERTY(SaveGame, BlueprintReadOnly)
    FTransform ActorTransform = FTransform::Identity;

    /**
     * UPROPERTY(SaveGame) 데이터의 직렬화 결과입니다.
     */
    UPROPERTY(SaveGame, BlueprintReadOnly)
    TArray<uint8> ActorData;

    //--------------------------------------------------

    UPROPERTY(SaveGame)
    FName ActorSaveID = NAME_None;

    UPROPERTY(SaveGame)
    bool bRuntimeSpawned = false;

    UPROPERTY(SaveGame)
    bool bDestroyed = false;

    bool IsValid() const
    {
        return InstanceID.IsValid();
    }
};

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FCheckpointSaveData
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame, BlueprintReadOnly)
    FName CheckpointID = NAME_None;
    UPROPERTY(SaveGame, BlueprintReadOnly)
    FTransform RespawnTransform = FTransform::Identity;
public:
    bool IsValid() const
    {
        return !CheckpointID.IsNone();
    }

    void Reset()
    {
        CheckpointID = NAME_None;
        RespawnTransform = FTransform::Identity;
    }
};