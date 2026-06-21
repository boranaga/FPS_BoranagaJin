// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ItemToolWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"

void UItemToolWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UItemToolWidget::NativeConstruct()
{
    Super::NativeConstruct();
    //SetItemToolPosition();
}

void UItemToolWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
    bMouseHovered = true;
    OnToolWidgetEnter.Broadcast();
}

void UItemToolWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    bMouseHovered = false;
    OnToolWidgetLeave.Broadcast();

    //SetVisibility(ESlateVisibility::Hidden);
    //RemoveFromParent();
}

void UItemToolWidget::SetItemToolPosition()
{
    if (UWorld* World = GetWorld())
    {
        FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(World);
        UWidgetLayoutLibrary::SlotAsCanvasSlot(Border_ItemTool)->SetPosition(MousePosition);

        //UE_LOG(LogTemp, Warning, TEXT("DPI Scaled Mouse Position: X= %f, Y= %f"), MousePosition.X, MousePosition.Y);
    }
}
