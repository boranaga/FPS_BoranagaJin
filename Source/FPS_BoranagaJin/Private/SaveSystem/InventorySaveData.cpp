#include "SaveSystem/InventorySaveData.h"

bool FInventorySlotSaveData::ResolveItem(FItemInstanceSaveData& OutItem) const
{
    OutItem.Reset();
    if (Item.IsValid())
    {
        if (Item.ItemNum <= 0) return false;
        OutItem = Item;
        return true;
    }
    if (Items.IsEmpty() || !Items[0].IsValid()) return false;

    // Version 1: one record per actor (sometimes repeated references).
    // Retain the first instance's state; only homogeneous records can be merged.
    OutItem = Items[0];
    int64 Total = 0;
    for (const FItemInstanceSaveData& OldItem : Items)
    {
        if (!OldItem.IsValid() || OldItem.ItemClass != OutItem.ItemClass ||
            OldItem.ItemID != OutItem.ItemID || OldItem.ItemName != OutItem.ItemName)
        {
            OutItem.Reset();
            return false;
        }
        Total += FMath::Max(1, OldItem.ItemNum);
        if (Total > MAX_int32) { OutItem.Reset(); return false; }
    }
    OutItem.ItemNum = FMath::Max(static_cast<int32>(Total), Count);
    return true;
}

int32 FInventorySlotSaveData::GetCount() const
{
    FItemInstanceSaveData Resolved;
    return ResolveItem(Resolved) ? Resolved.ItemNum : 0;
}

void FInventorySlotSaveData::Reset()
{
    Item.Reset();
    Items.Reset();
    Count = 0;
    MaxStackCount = 99;
}
