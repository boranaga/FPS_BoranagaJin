#pragma once

#include "CoreMinimal.h"
#include "SaveSystem/InventorySaveData.h"
#include "SaveDataTypes.generated.h"

/** Runtime state only. DataTable and Blueprint configuration remain authoritative. */
USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FBloodTrailComponentSaveData
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    bool bHasData = false;

    UPROPERTY(SaveGame)
    bool bIsBleeding = false;

    // -1: indefinite bleeding; 0: stopped; positive: remaining game seconds.
    UPROPERTY(SaveGame)
    float RemainingBleedingTime = 0.f;

    // -1: no successful drop yet; otherwise elapsed game seconds since last drop.
    UPROPERTY(SaveGame)
    float LastBloodDropAge = -1.f;

    UPROPERTY(SaveGame)
    FVector LastBloodDropLocation = FVector::ZeroVector;

    UPROPERTY(SaveGame)
    int32 NextSequenceIndex = 0;
};

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FPlayerSaveData
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame)
    FTransform PlayerTransform = FTransform::Identity;

    UPROPERTY(SaveGame)
    float CurrentHealth = 100.f; // Existing field: do not duplicate player HP.

    UPROPERTY(SaveGame)
    FBloodTrailComponentSaveData BloodTrailData;

    UPROPERTY(SaveGame)
    TArray<FInventorySlotSaveData> ItemInventorySlots;

    //UPROPERTY(SaveGame)
    //TArray<FInventorySlotSaveData> WeaponInventorySlots;

    UPROPERTY(SaveGame)
    TArray<FInventorySlotSaveData> PrimaryWeaponInventorySlots;

    UPROPERTY(SaveGame)
    TArray<FInventorySlotSaveData> SecondaryWeaponInventorySlots;

    UPROPERTY(SaveGame)
    TArray<FInventorySlotSaveData> ThrowableWeaponInventorySlots;

    UPROPERTY(SaveGame)
    int32 EquippedQuickSlotIndex = INDEX_NONE;

    //UPROPERTY(SaveGame, BlueprintReadOnly)
    //FName EquippedWeaponID = NAME_None; //TODO: 굳이?

    void Reset()
    {
        PlayerTransform = FTransform::Identity;
        CurrentHealth = 0.0f;
        BloodTrailData = FBloodTrailComponentSaveData();
        ItemInventorySlots.Reset();
        EquippedQuickSlotIndex = INDEX_NONE;
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

    // Components are not automatically serialized by Actor->Serialize().
    // False means an older save or an actor without a health component.
    UPROPERTY(SaveGame)
    bool bHasHealthComponentData = false;

    UPROPERTY(SaveGame)
    float HealthComponentCurrentHealth = 100.f;

    UPROPERTY(SaveGame)
    FBloodTrailComponentSaveData BloodTrailData;

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
