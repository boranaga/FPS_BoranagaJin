// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InventoryUIWidget.h"
#include "UI/InventorySlotWidget.h"
#include "UI/UIManagerComponent.h"
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
	InventorySlotWidgets.Empty();

	for (int32 i = 0; i < InventorySlotCount; i++)
	{
		UInventorySlotWidget* NewInventorySlot = CreateWidget<UInventorySlotWidget>(GetWorld(), InventorySlotWidgetClass);

		if (NewInventorySlot)
		{
			NewInventorySlot->SetOwnerUIManager(OwnerUIManager);

			NewInventorySlot->SetIndex(i);
			NewInventorySlot->SetOwnerInventoryWidget(this);

			InventorySlotWidgets.Add(NewInventorySlot);

			NewInventorySlot->SetVisibility(ESlateVisibility::Visible);
			WrapBoxInventory->AddChildToWrapBox(NewInventorySlot);
		}
	}
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

	//------------------------------

	for (int32 i = 0; i < InventorySlotWidgets.Num(); i++)
	{
		if (!InventorySlotWidgets.IsValidIndex(i) || !InventorySlotWidgets[i])
		{
			continue;
		}

		if (Inventory.IsValidIndex(i) && !Inventory[i].IsEmpty())
		{
			InventorySlotWidgets[i]->SetItemSlotData(
				Inventory[i].ItemID,
				Inventory[i].Count
			);
		}
		else
		{
			InventorySlotWidgets[i]->ClearItemSlotData();
		}
	}
}

void UInventoryUIWidget::SwapInventorySlots(int32 FromIndex, int32 ToIndex)
{
	//if (FromIndex == ToIndex)
	//{
	//	return;
	//}

	//if (!InventorySlotWidgets.IsValidIndex(FromIndex) ||
	//	!InventorySlotWidgets.IsValidIndex(ToIndex))
	//{
	//	return;
	//}

	//InventorySlotWidgets.Swap(FromIndex, ToIndex);

	//WrapBoxInventory->ClearChildren();

	//for (int32 i = 0; i < InventorySlotWidgets.Num(); i++)
	//{
	//	UInventorySlotWidget* SlotWidget = InventorySlotWidgets[i];

	//	if (!SlotWidget)
	//	{
	//		continue;
	//	}

	//	SlotWidget->SetIndex(i);
	//	WrapBoxInventory->AddChildToWrapBox(SlotWidget);
	//}

	//--------------------------------

	OwnerUIManager->RequestSwapInventorySlots(InventoryName, FromIndex, ToIndex);
}

void UInventoryUIWidget::RequestSwapInventorySlots(FName InInventoryName, int32 FromIndex, int32 ToIndex)
{
	OwnerUIManager->RequestSwapInventorySlots(InInventoryName, FromIndex, ToIndex);
}

