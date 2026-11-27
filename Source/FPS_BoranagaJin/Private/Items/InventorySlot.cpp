#include "Items/InventorySlot.h"
#include "Items/Weapons/Weapon.h"
#include "Items/Weapons/ThrowableWeapon/ThrowableWeapon.h"
#include "Items/FlashlightItem.h"

void FInventorySlot::ClearSlot()
{
    Item = nullptr;
    ItemName = EItemName::ItemName_None;
    ItemID = NAME_None;
    ItemNum = 0;
}

bool FInventorySlot::IsEmpty() const
{
    return !IsValid(Item.Get()) || Item->GetStackItemNum() <= 0;
}

int32 FInventorySlot::GetStackItemNum() const
{
    return IsValid(Item.Get()) ? FMath::Max(0, Item->GetStackItemNum()) : 0;
}

int32 FInventorySlot::Num() const
{
    if (IsEmpty()) return 0;
    // AThrowableWeapon is a launcher, not one actor per projectile.
    if (Item->IsA(AThrowableWeapon::StaticClass()))
        return FMath::Max(0, Item->GetItemNum());
    return GetStackItemNum();
}

int32 FInventorySlot::Count() const
{
    return Num();
}

bool FInventorySlot::IsSameItem(const EItemName& InItemName) const
{
    return !IsEmpty() && ItemName == InItemName;
}

bool FInventorySlot::CanRepresentStack(const AItem* InItem)
{
    return IsValid(InItem) && InItem->IsStackable() && !InItem->IsWeapon() &&
        !InItem->IsA(AWeapon::StaticClass()) && !InItem->IsA(AThrowableWeapon::StaticClass()) &&
        !InItem->IsA(AFlashlightItem::StaticClass());
}

bool FInventorySlot::CanStack(const EItemName& InItemName, int32 AddCount) const
{
    return AddCount > 0 && IsSameItem(InItemName) && CanRepresentStack(Item.Get()) &&
        static_cast<int64>(GetStackItemNum()) + AddCount <= FMath::Max(1, MaxStackCount);
}

bool FInventorySlot::CanStackWith(const AItem* InItem, int32 AddCount) const
{
    return CanRepresentStack(InItem) && CanStack(InItem->GetItemName(), AddCount) &&
        ItemID == InItem->GetItemID() && Item->GetClass() == InItem->GetClass();
}

bool FInventorySlot::AddItem(AItem* InItem, int32 AddCount)
{
    if (!IsValid(InItem) || InItem->GetItemName() == EItemName::ItemName_None || AddCount <= 0)
        return false;
    if (!IsEmpty())
    {
        // A different actor must be disposed of by the inventory transaction, not the slot.
        return Item.Get() == InItem && AddStack(AddCount);
    }
    const int32 Limit = CanRepresentStack(InItem) ? FMath::Max(1, MaxStackCount) : 1;
    if (AddCount > Limit) return false;
    Item = InItem;
    ItemName = InItem->GetItemName();
    ItemID = InItem->GetItemID();
    Item->SetItemNum(AddCount);
    SynchronizeItemNum();
    return true;
}

bool FInventorySlot::AddStack(int32 AddCount)
{
    if (!CanStack(ItemName, AddCount)) return false;
    Item->SetItemNum(GetStackItemNum() + AddCount);
    SynchronizeItemNum();
    return true;
}

bool FInventorySlot::RemoveItems(int32 RemoveCount)
{
    if (IsEmpty() || RemoveCount <= 0 || RemoveCount > GetStackItemNum()) return false;
    Item->SetItemNum(GetStackItemNum() - RemoveCount);
    if (Item->GetStackItemNum() == 0) ClearSlot();
    else SynchronizeItemNum();
    return true;
}

void FInventorySlot::SynchronizeItemNum()
{
    ItemNum = GetStackItemNum();
    if (ItemNum == 0) ClearSlot();
}

AItem* FInventorySlot::GetItem(int32 ItemIdx) const
{
    return ItemIdx == 0 && !IsEmpty() ? Item.Get() : nullptr;
}
