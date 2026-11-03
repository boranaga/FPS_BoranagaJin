
#pragma once

#include "CoreMinimal.h"
#include "Items/InventoryName.h"
#include "QuickSlotSaveData.generated.h"

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FQuickSlotSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    int32 QuickSlotIndex = INDEX_NONE;

    UPROPERTY()
    EInventoryName SourceInventory = EInventoryName::Item;

    UPROPERTY()
    int32 SourceSlotIndex = INDEX_NONE;

    // Unique runtime item identity.
    UPROPERTY()
    FGuid ItemInstanceID;
};
