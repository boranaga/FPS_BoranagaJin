// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InventorySlotWidget.h"
#include "UI/InventoryUIWidget.h"
#include "UI/ItemToolWidget.h"
#include "UI/UIType.h"
#include "UI/InventoryDragDropOperation.h"
#include "Data/ItemData.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

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
            
            ItemToolWidget->OnUseItemRequested.AddUObject(this, &UInventorySlotWidget::OnUseItemRequested);
            ItemToolWidget->OnDropItemRequested.AddUObject(this, &UInventorySlotWidget::OnDropItemRequested);

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

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    //UE_LOG(LogTemp, Error, TEXT("UInventorySlotWidget::NativeOnMouseButtonDown)"));

    //if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    //{
    //    UE_LOG(LogTemp, Error, TEXT("UInventorySlotWidget::NativeOnMouseButtonDown)"));

    //    return UWidgetBlueprintLibrary::DetectDragIfPressed(
    //        InMouseEvent,
    //        this,
    //        EKeys::LeftMouseButton
    //    ).NativeReply;
    //}

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventorySlotWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    //UE_LOG(LogTemp, Error, TEXT("UInventorySlotWidget::NativeOnMouseButtonDown)"));

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        //UE_LOG(LogTemp, Error, TEXT("UInventorySlotWidget::NativeOnMouseButtonDown)"));

        return UWidgetBlueprintLibrary::DetectDragIfPressed(
            InMouseEvent,
            this,
            EKeys::LeftMouseButton
        ).NativeReply;
    }

    return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

    UInventoryDragDropOperation* DragOperation = NewObject<UInventoryDragDropOperation>();

    if (!DragOperation)
    {
        return;
    }

    //UE_LOG(LogTemp, Error, TEXT("UInventorySlotWidget::NativeOnDragDetected)"));

    DragOperation->DraggedSlotWidget = this;
    DragOperation->FromIndex = Index;

    // 드래그 중 보이는 위젯
    DragOperation->DefaultDragVisual = this;
    DragOperation->Pivot = EDragPivot::MouseDown;

    OutOperation = DragOperation;
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

    UInventoryDragDropOperation* DragOperation =
        Cast<UInventoryDragDropOperation>(InOperation);

    if (!DragOperation) { return; }
    if (!OwnerInventoryWidget) { return; }

    const FVector2D ScreenPosition = InDragDropEvent.GetScreenSpacePosition();
    const bool bInsideInventory = OwnerInventoryWidget->IsScreenPositionInsideInventory(ScreenPosition);
    if (!bInsideInventory)
    {
        OwnerInventoryWidget->RequestDropInventorySlot(DragOperation->FromIndex);
    }
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation);

    if (!DragOperation)
    {
        return false;
    }

    if (!OwnerInventoryWidget)
    {
        return false;
    }

    //UE_LOG(LogTemp, Error, TEXT("UInventorySlotWidget::NativeOnDrop"));

    const int32 FromIndex = DragOperation->FromIndex;
    const int32 ToIndex = Index;

    OwnerInventoryWidget->RequestSwapInventorySlots(FromIndex, ToIndex);

    return true;
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

void UInventorySlotWidget::LoadItemDataTable()
{
    if (ItemDataTable.IsNull()) return;
    LoadedItemTable = ItemDataTable.LoadSynchronous();
}

void UInventorySlotWidget::ClearItemSlotData()
{
    ItemQuantity = 0;
    ItemName = EItemName::ItemName_None;

    if (TextItemQuantity)
    {
        TextItemQuantity->SetText(FText::GetEmpty());
    }

    if (ItemIcon)
    {
        ItemIcon->SetBrushFromTexture(nullptr);
    }

    if (OverlayInventorySlot)
    {
        OverlayInventorySlot->SetVisibility(ESlateVisibility::Collapsed);
    }
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

void UInventorySlotWidget::OnUseItemRequested()
{
    UE_LOG(LogTemp, Error, TEXT("void UInventorySlotWidget::OnUseItemRequested()"));
    if (!OwnerInventoryWidget) { return; }
    OwnerInventoryWidget->RequestUseInventorySlot(Index);
}

void UInventorySlotWidget::OnDropItemRequested()
{
    UE_LOG(LogTemp, Error, TEXT("void UInventorySlotWidget::OnDropItemRequested()"));
    if (!OwnerInventoryWidget) { return; }
    OwnerInventoryWidget->RequestDropInventorySlot(Index);
}
