#pragma once

#include "CoreMinimal.h"
#include "Items/InventorySlot.h"
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
    TArray<FInventorySlot> InventorySlots;

    UPROPERTY(SaveGame)
    int32 EquippedWeaponIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FWorldActorSaveData
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame)
    FName ActorSaveID = NAME_None;

    UPROPERTY(SaveGame)
    bool bDestroyed = false;

    UPROPERTY(SaveGame)
    FTransform Transform = FTransform::Identity;
};