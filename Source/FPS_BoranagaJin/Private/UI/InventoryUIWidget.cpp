// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InventoryUIWidget.h"
#include "UI/InventorySlotWidget.h"
#include "Items/InventorySlot.h"

#include "Components/WrapBox.h"


void UInventoryUIWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    //LoadItemDataTable();
}

void UInventoryUIWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    CachedGeometry = MyGeometry;
}

//void UInventoryUIWidget::LoadItemDataTable()
//{
//	if (ItemDataTable.IsNull()) return;
//	LoadedItemTable = ItemDataTable.LoadSynchronous();
//}

void UInventoryUIWidget::CreateInventorySlots(int32 InventorySlotCount)
{
    if (!IsValid(WrapBoxInventory) || !InventorySlotWidgetClass || InventorySlotCount < 0) return;
    if (InventorySlotWidgets.Num() == InventorySlotCount && WrapBoxInventory->GetChildrenCount() == InventorySlotCount)
    {
        bool bAllValid = true;
        for (UInventorySlotWidget* Widget : InventorySlotWidgets) bAllValid &= IsValid(Widget);
        if (bAllValid) return;
    }
    WrapBoxInventory->ClearChildren();
    InventorySlotWidgets.Reset();
    InventorySlotWidgets.SetNumZeroed(InventorySlotCount);
    for (int32 Index = 0; Index < InventorySlotCount; ++Index)
    {
        UInventorySlotWidget* Widget = CreateWidget<UInventorySlotWidget>(GetWorld(), InventorySlotWidgetClass);
        if (!IsValid(Widget)) continue;
        Widget->SetIndex(Index);
        Widget->SetOwnerInventoryWidget(this);
        InventorySlotWidgets[Index] = Widget;
        Widget->SetVisibility(ESlateVisibility::Visible);
        WrapBoxInventory->AddChildToWrapBox(Widget);
        Widget->ClearItemSlotData();
    }
}

void UInventoryUIWidget::UpdateInventorySlots(const TArray<FInventorySlot>& Inventory)
{
    // Restore may change the number of slots; retain stable index mapping.
    CreateInventorySlots(Inventory.Num());
    for (int32 Index = 0; Index < InventorySlotWidgets.Num(); ++Index)
    {
        UInventorySlotWidget* Widget = InventorySlotWidgets[Index];
        if (!IsValid(Widget)) continue;
        if (Inventory.IsValidIndex(Index) && !Inventory[Index].IsEmpty())
            Widget->SetItemSlotData(Inventory[Index].ItemID, Inventory[Index].Num());
        else Widget->ClearItemSlotData();
    }
}

void UInventoryUIWidget::RequestSwapInventorySlots(int32 FromIndex, int32 ToIndex)
{
    OnSwapInventorySlotsRequestedDelegate.Broadcast(InventoryName, FromIndex, ToIndex);
}

void UInventoryUIWidget::RequestDropInventorySlot(int32 SlotIndex)
{
    OnDropInventorySlotRequestedDelegate.Broadcast(InventoryName, SlotIndex);
}

void UInventoryUIWidget::RequestUseInventorySlot(int32 SlotIndex)
{
    OnUseInventorySlotRequestedDelegate.Broadcast(InventoryName, SlotIndex);
}

bool UInventoryUIWidget::IsScreenPositionInsideInventory(const FVector2D& ScreenPosition) const
{
    const FVector2D LocalPosition =
        CachedGeometry.AbsoluteToLocal(ScreenPosition);

    const FVector2D Size = CachedGeometry.GetLocalSize();

    return LocalPosition.X >= 0.f &&
        LocalPosition.Y >= 0.f &&
        LocalPosition.X <= Size.X &&
        LocalPosition.Y <= Size.Y;
}

