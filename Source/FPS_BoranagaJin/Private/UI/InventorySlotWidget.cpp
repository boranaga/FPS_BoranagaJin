// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InventorySlotWidget.h"
#include "Data/ItemData.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"

void UInventorySlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    SetItemData();
}

void UInventorySlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UInventorySlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UInventorySlotWidget::SetItemData()
{
    if (ItemDataTable.IsNull() || ItemRowName.IsNone()) return;
    LoadedItemTable = ItemDataTable.LoadSynchronous();
    if (!LoadedItemTable) return;

    FItemData* ItemData = LoadedItemTable->FindRow<FItemData>(ItemRowName, TEXT("LoadItemData"));
    if (!ItemData)
    {       
        OverlayInventorySlot->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    ItemIcon->SetBrushFromTexture(ItemData->ItemImage);
    TextItemQuantity->SetText(FText::FromString(FString::Printf(TEXT("%d"), ItemQuantity)));
    OverlayInventorySlot->SetVisibility(ESlateVisibility::Visible);
}
