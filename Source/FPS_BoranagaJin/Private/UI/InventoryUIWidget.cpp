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

//void UInventoryUIWidget::LoadItemDataTable()
//{
//	if (ItemDataTable.IsNull()) return;
//	LoadedItemTable = ItemDataTable.LoadSynchronous();
//}

void UInventoryUIWidget::CreateInventorySlots(int32 InventorySlotCount)
{
	if (!WrapBoxInventory) return;

	WrapBoxInventory->ClearChildren();

	for (int32 i = 0; i < InventorySlotCount; i++)
	{
		UInventorySlotWidget* NewInventorySlot = CreateWidget<UInventorySlotWidget>(GetWorld(), InventorySlotWidgetClass);
		if (NewInventorySlot)
		{
			InventorySlotWidgets.Add(NewInventorySlot);
			NewInventorySlot->SetVisibility(ESlateVisibility::Visible);
			WrapBoxInventory->AddChildToWrapBox(NewInventorySlot);

			//UE_LOG(LogTemp, Error, TEXT("UInventoryUIWidget::CreateInventorySlots(int32 InventorySlotCount)"));
		}
	}

	//UE_LOG(LogTemp, Error, TEXT("UInventoryUIWidget::CreateInventorySlots(int32 InventorySlotCount)"));
}

void UInventoryUIWidget::UpdateInventorySlots(const TArray<FInventorySlot>& Inventory)
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (!Inventory[i].IsEmpty())
		{
			if (InventorySlotWidgets.IsValidIndex(i) && InventorySlotWidgets[i])
			{
				InventorySlotWidgets[i]->SetItemSlotData(Inventory[i].ItemID, Inventory[i].Count);

				UE_LOG(LogTemp, Error, TEXT("UInventoryUIWidget::UpdateInventorySlots(const TArray<FInventorySlot>& Inventory)"));
			}
		}
	}
}

