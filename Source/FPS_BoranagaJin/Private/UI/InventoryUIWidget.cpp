// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InventoryUIWidget.h"
#include "UI/InventorySlotWidget.h"

#include "Components/WrapBox.h"


void UInventoryUIWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	CreateInventorySlots();
}

void UInventoryUIWidget::CreateInventorySlots()
{
	if (!WrapBoxInventory) return;

	WrapBoxInventory->ClearChildren();

	//TODO: for¹®À¸·Î ¤¡¤¡
	UInventorySlotWidget* NewInventorySlot = CreateWidget<UInventorySlotWidget>(GetWorld(), InventorySlotWidgetClass);

	if (NewInventorySlot)
	{
		WrapBoxInventory->AddChildToWrapBox(NewInventorySlot);
	}


}

