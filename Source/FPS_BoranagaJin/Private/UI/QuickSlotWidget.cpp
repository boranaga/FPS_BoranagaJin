
#include "UI/QuickSlotWidget.h"
#include "UI/QuickSlotEntryWidget.h"

//#include "Items/InventorySystemComponent.h"
#include "Items/QuickSlot.h"
#include "Items/InventorySlot.h"
#include "Items/Item.h"

#include "Engine/Texture2D.h"

void UQuickSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshQuickSlots();
}

void UQuickSlotWidget::NativeDestruct()
{
    //if (UInventorySystemComponent* Inventory = InventoryComponent.Get())
    //{
    //    Inventory->OnQuickSlotInventoryChanged.RemoveAll(this);
    //    Inventory->OnQuickSlotSelectionChanged.RemoveAll(this);
    //}

    Super::NativeDestruct();
}

//void UQuickSlotWidget::InitializeQuickSlotWidget(UInventorySystemComponent* InInventory)
//{
//    //if (UInventorySystemComponent* OldInventory = InventoryComponent.Get())
//    //{
//    //    OldInventory->OnQuickSlotInventoryChanged.RemoveAll(this);
//    //    OldInventory->OnQuickSlotSelectionChanged.RemoveAll(this);
//    //}
//
//    //InventoryComponent = InInventory;
//
//    //if (!IsValid(InInventory))
//    //{
//    //    RefreshQuickSlots();
//    //    return;
//    //}
//
//    //InInventory->OnQuickSlotInventoryChanged.AddUObject(this, &UQuickSlotWidget::RefreshQuickSlots);
//    //InInventory->OnQuickSlotSelectionChanged.AddUObject(this, &UQuickSlotWidget::HandleQuickSlotSelectionChanged);
//
//    RefreshQuickSlots();
//}

void UQuickSlotWidget::HandleQuickSlotSelectionChanged(int32 PreviousIndex, int32 NewIndex)
{
    RefreshQuickSlots();
}

FText UQuickSlotWidget::GetSlotTypeText(int32 SlotIndex) const
{
    switch (SlotIndex)
    {
    case 0:
    case 1:
        return FText::FromString(TEXT("PRIMARY"));

    case 2:
        return FText::FromString(TEXT("SECONDARY"));

    case 3:
        return FText::FromString(TEXT("THROWABLE"));

    case 4:
        return FText::FromString(TEXT("UTILITY"));

    default:
        return FText::GetEmpty();
    }
}

UTexture2D* UQuickSlotWidget::FindItemIcon(EItemName ItemName) const
{
    if (const TObjectPtr<UTexture2D>* Found = ItemIcons.Find(ItemName))
    {
        return Found->Get();
    }

    return nullptr;
}

void UQuickSlotWidget::RefreshQuickSlots()
{
    //UQuickSlotEntryWidget* Entries[5] =
    //{
    //    QuickSlot1,
    //    QuickSlot2,
    //    QuickSlot3,
    //    QuickSlot4,
    //    QuickSlot5
    //};

    //UInventorySystemComponent* Inventory = InventoryComponent.Get();

    //if (!IsValid(Inventory))
    //{
    //    for (int32 Index = 0; Index < 5; ++Index)
    //    {
    //        if (!IsValid(Entries[Index]))
    //        {
    //            continue;
    //        }

    //        Entries[Index]->UpdateSlot(
    //            Index,
    //            FText::GetEmpty(),
    //            GetSlotTypeText(Index),
    //            nullptr,
    //            0,
    //            false,
    //            false
    //        );
    //    }

    //    return;
    //}

    //const TArray<FQuickSlot>& QuickSlots =
    //    Inventory->GetQuickSlotInventory();

    //const int32 CurrentIndex =
    //    Inventory->GetCurrentQuickSlotIndex();

    //for (int32 Index = 0; Index < 5; ++Index)
    //{
    //    UQuickSlotEntryWidget* Entry = Entries[Index];

    //    if (!IsValid(Entry))
    //    {
    //        continue;
    //    }

    //    AItem* Item = nullptr;
    //    int32 ItemCount = 0;

    //    if (QuickSlots.IsValidIndex(Index))
    //    {
    //        const FQuickSlot& QuickSlot = QuickSlots[Index];

    //        const TArray<FInventorySlot>* SourceInventory =
    //            Inventory->GetInventoryConst(
    //                QuickSlot.SourceInventory
    //            );

    //        if (SourceInventory)
    //        {
    //            // Find the referenced item in the actual inventory.
    //            for (const FInventorySlot& SourceSlot : *SourceInventory)
    //            {
    //                if (SourceSlot.IsEmpty())
    //                {
    //                    continue;
    //                }

    //                for (const TObjectPtr<AItem>& StoredItem : SourceSlot.Items)
    //                {
    //                    if (StoredItem.Get() == QuickSlot.GetItem() &&
    //                        IsValid(StoredItem.Get()))
    //                    {
    //                        Item = StoredItem.Get();
    //                        ItemCount = SourceSlot.Num();
    //                        break;
    //                    }
    //                }

    //                if (Item)
    //                {
    //                    break;
    //                }
    //            }
    //        }
    //    }

    //    const bool bHasItem = IsValid(Item);
    //    const bool bSelected = CurrentIndex == Index && bHasItem;

    //    FText ItemName = FText::GetEmpty();
    //    UTexture2D* ItemIcon = nullptr;

    //    if (bHasItem)
    //    {
    //        const EItemName ItemNameEnum = Item->GetItemName();

    //        if (const UEnum* Enum = StaticEnum<EItemName>())
    //        {
    //            ItemName = Enum->GetDisplayNameTextByValue(
    //                static_cast<int64>(ItemNameEnum)
    //            );
    //        }

    //        ItemIcon = FindItemIcon(ItemNameEnum);
    //    }

    //    Entry->UpdateSlot(
    //        Index,
    //        ItemName,
    //        GetSlotTypeText(Index),
    //        ItemIcon,
    //        ItemCount,
    //        bHasItem,
    //        bSelected
    //    );
    //}
}

void UQuickSlotWidget::UpdateQuickSlots(const TArray<FQuickSlot>& QuickSlots, int32 CurrIdx)
{
    UQuickSlotEntryWidget* Entries[5] =
    {
        QuickSlot1,
        QuickSlot2,
        QuickSlot3,
        QuickSlot4,
        QuickSlot5
    };

    if (QuickSlots.IsEmpty())
    {
        for (int32 Index = 0; Index < 5; ++Index)
        {
            if (!IsValid(Entries[Index]))
            {
                continue;
            }

            Entries[Index]->UpdateSlot(
                Index,
                FText::GetEmpty(),
                GetSlotTypeText(Index),
                nullptr,
                0,
                false,
                false
            );
        }

        return;
    }

    for (int32 Index = 0; Index < 5; ++Index)
    {
        UQuickSlotEntryWidget* Entry = Entries[Index];

        if (!IsValid(Entry)) { continue; }

        AItem* Item = nullptr;
        int32 ItemCount = 0;

        if (QuickSlots.IsValidIndex(Index))
        {
            const FQuickSlot& QuickSlot = QuickSlots[Index];

            Item = QuickSlot.GetItem();
            ItemCount = QuickSlot.Count;
        }

        const bool bHasItem = IsValid(Item);
        const bool bSelected = CurrIdx == Index && bHasItem;

        FText ItemName = FText::GetEmpty();
        UTexture2D* ItemIcon = nullptr;

        if (bHasItem)
        {
            const EItemName ItemNameEnum = Item->GetItemName();

            if (const UEnum* Enum = StaticEnum<EItemName>())
            {
                ItemName = Enum->GetDisplayNameTextByValue(static_cast<int64>(ItemNameEnum));
            }

            ItemIcon = FindItemIcon(ItemNameEnum);
        }

        Entry->UpdateSlot(
            Index,
            ItemName,
            GetSlotTypeText(Index),
            ItemIcon,
            ItemCount,
            bHasItem,
            bSelected
        );
    }

    UE_LOG(LogTemp, Error, TEXT("void UQuickSlotWidget::UpdateQuickSlots(const TArray<FQuickSlot>& QuickSlots, int32 CurrIdx)"));
}
