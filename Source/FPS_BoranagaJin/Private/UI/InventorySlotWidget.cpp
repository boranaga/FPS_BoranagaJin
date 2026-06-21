// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InventorySlotWidget.h"
#include "UI/ItemToolWidget.h"
#include "UI/UIType.h"
#include "Data/ItemData.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include "Blueprint/SlateBlueprintLibrary.h"

void UInventorySlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();


    OverlayInventorySlot->SetVisibility(ESlateVisibility::Visible);
    //SetItemData();

    LoadItemDataTable();
    // UE_LOG(LogTemp, Error, TEXT("UInventorySlotWidget::NativePreConstruct()"));

    //----------------
    if (ItemToolWidgetClass)
    {
        ItemToolWidget = CreateWidget<UItemToolWidget>(GetWorld(), ItemToolWidgetClass);
        if (ItemToolWidget)
        {
            ItemToolWidget->AddToViewport(static_cast<int32>(EUIZOrder::ItemTool));
            ItemToolWidget->SetVisibility(ESlateVisibility::Hidden);


            ItemToolWidget->OnToolWidgetEnter.AddUObject(this, &UInventorySlotWidget::OnMouseEnterToToolWidget);
            ItemToolWidget->OnToolWidgetLeave.AddUObject(this, &UInventorySlotWidget::OnMouseLeaveFromToolWidget);
            
            //ItemToolWidget->AddToViewport(static_cast<int32>(EUIZOrder::ItemTool));
            //UCanvasPanelSlot* CanvasSlot = ItemToolCanvas->AddChildToCanvas(ItemToolWidget);
            //if (CanvasSlot)
            //{
            //    //CanvasSlot->SetAutoSize(true);

            //    //// 슬롯 기준 오른쪽 아래 위치
            //    //CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
            //    //CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
            //    //CanvasSlot->SetPosition(ItemToolOffset);

            //    //CanvasSlot->SetZOrder(static_cast<int32>(EUIZOrder::ItemTool));
            //}
            //ItemToolWidget->SetVisibility(ESlateVisibility::Hidden);
        }

        //--------------

        //if (ItemToolWidget)
        //{
        //    UCanvasPanelSlot* CanvasSlot = ItemToolCanvas->AddChildToCanvas(ItemToolWidget);

        //    if (CanvasSlot)
        //    {
        //        CanvasSlot->SetAutoSize(true);

        //        // 슬롯 기준 오른쪽 아래 위치
        //        CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
        //        CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
        //        CanvasSlot->SetPosition(ItemToolOffset);

        //        // 필요하면 더 앞에 보이도록
        //        //CanvasSlot->SetZOrder(10);
        //    }

        //    ItemToolWidget->SetVisibility(ESlateVisibility::Hidden);
        //}
    }

}

void UInventorySlotWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UInventorySlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UInventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    bMouseHoveredOnSlotWidget = true;

    if (ItemToolWidget)
    {
        SetItemToolPosition(InGeometry);
        DisplayItemTool();
    }
}

void UInventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    bMouseHoveredOnSlotWidget = false;

    //if (!ItemToolWidget) { return; }
    //if (!ItemToolWidget->IsHoveredToolWidget())
    //{
    //    HideItemTool();
    //}

    GetWorld()->GetTimerManager().SetTimer(
        HideToolWidgetTimerHandle,
        this,
        &UInventorySlotWidget::CheckHideToolWidget,
        0.05f,
        false);

}

FReply UInventorySlotWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    //if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    //{
    //    UE_LOG(LogTemp, Log, TEXT("Right Mouse Button Down"));

    //    // 우클릭 처리
    //    return FReply::Handled();
    //}

    //return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);


    //if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
    //{
    //    DisplayItemTool();

    //    return FReply::Handled();
    //}

    return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
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

void UInventorySlotWidget::SetItemToolPosition(const FGeometry& InGeometry)
{
    if (!ItemToolWidget)
    {
        return;
    }

    const FVector2D SlotSize = InGeometry.GetLocalSize();

    // 슬롯의 오른쪽 아래 지점
    const FVector2D SlotRightBottomAbsolute =
        InGeometry.LocalToAbsolute(SlotSize);

    FVector2D PixelPosition;
    FVector2D ViewportPosition;

    USlateBlueprintLibrary::AbsoluteToViewport(
        GetWorld(),
        SlotRightBottomAbsolute,
        PixelPosition,
        ViewportPosition
    );

    // 슬롯 오른쪽 아래 + 고정 오프셋
    const FVector2D ToolPosition = ViewportPosition + ItemToolOffset;

    ItemToolWidget->SetPositionInViewport(ToolPosition, false);
}

void UInventorySlotWidget::DisplayItemTool()
{
    if (ItemToolWidget)
    {
        //ItemToolWidget->SetItemToolPosition();
        ItemToolWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

void UInventorySlotWidget::HideItemTool()
{
    if (ItemToolWidget)
    {
        ItemToolWidget->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UInventorySlotWidget::CheckHideToolWidget()
{
    if (!bMouseHoveredOnSlotWidget &&
        !ItemToolWidget->IsHoveredToolWidget())
    {
        HideItemTool();
    }
}

void UInventorySlotWidget::OnMouseEnterToToolWidget()
{

}

void UInventorySlotWidget::OnMouseLeaveFromToolWidget()
{
    if (!bMouseHoveredOnSlotWidget)
    {
        HideItemTool();
    }
}
