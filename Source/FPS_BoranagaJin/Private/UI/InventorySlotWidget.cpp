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


    OverlayInventorySlot->SetVisibility(ESlateVisibility::Visible);
    //SetItemData();

    LoadItemDataTable();
    // UE_LOG(LogTemp, Error, TEXT("UInventorySlotWidget::NativePreConstruct()"));
}

void UInventorySlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UInventorySlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UInventorySlotWidget::LoadItemDataTable()
{
    if (ItemDataTable.IsNull()) return;
    LoadedItemTable = ItemDataTable.LoadSynchronous();
}

void UInventorySlotWidget::SetItemSlotData(FName ItemDataRowName, int32 InItemQuantity)
{
    if (!LoadedItemTable) return;
    FItemData* ItemData = LoadedItemTable->FindRow<FItemData>(ItemDataRowName, TEXT("LoadItemData"));
    if (!ItemData)
    {
        //OverlayInventorySlot->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    ItemIcon->SetBrushFromTexture(ItemData->ItemImage);
    ItemQuantity = InItemQuantity;
    TextItemQuantity->SetText(FText::FromString(FString::Printf(TEXT("%d"), ItemQuantity)));
    OverlayInventorySlot->SetVisibility(ESlateVisibility::Visible);
}

void UInventorySlotWidget::SetItemData()
{
    //if (ItemDataTable.IsNull() || ItemRowName.IsNone()) return;
    //LoadedItemTable = ItemDataTable.LoadSynchronous();
    //if (!LoadedItemTable) return;

    //FItemData* ItemData = LoadedItemTable->FindRow<FItemData>(ItemRowName, TEXT("LoadItemData"));
    //if (!ItemData)
    //{       
    //    OverlayInventorySlot->SetVisibility(ESlateVisibility::Collapsed);
    //    return;
    //}

    //ItemIcon->SetBrushFromTexture(ItemData->ItemImage);
    //TextItemQuantity->SetText(FText::FromString(FString::Printf(TEXT("%d"), ItemQuantity)));
    //OverlayInventorySlot->SetVisibility(ESlateVisibility::Visible);
}
