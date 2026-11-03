
#pragma once

#include "CoreMinimal.h"
#include "Items/InventoryName.h"
#include "Items/QuickSlotType.h"
#include "QuickSlot.generated.h"

class AItem;

USTRUCT(BlueprintType)
struct FPS_BORANAGAJIN_API FQuickSlot
{
    GENERATED_BODY()
public:
    // Slot purpose.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EQuickSlotType SlotType = EQuickSlotType::PrimaryWeapon;

    // Source inventory.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EInventoryName SourceInventory = EInventoryName::Weapon;

    // Hint for finding the source slot.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 SourceSlotIndex = INDEX_NONE;

    // Runtime reference, not ownership.
    UPROPERTY(Transient)
    TWeakObjectPtr<AItem> Item;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Count = 0;

    void Clear()
    {
        SourceSlotIndex = INDEX_NONE;
        Item.Reset();
    }

    bool IsEmpty() const
    {
        return !Item.IsValid();
    }

    AItem* GetItem() const
    {
        return Item.Get();
    }
};
