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
				InventorySlotWidgets[i]->SetItemSlotData(Inventory[i].ItemID, Inventory[i].Num());
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
				Inventory[i].Num()
			);
		}
		else
		{
			InventorySlotWidgets[i]->ClearItemSlotData();
		}
	}
}

void UInventoryUIWidget::RequestSwapInventorySlots(int32 FromIndex, int32 ToIndex)
{
	if (!OwnerUIManager) return;
	OwnerUIManager->RequestSwapInventorySlots(InventoryName, FromIndex, ToIndex);
}

void UInventoryUIWidget::RequestDropInventorySlot(int32 SlotIndex)
{
	if (!OwnerUIManager) return;
	OwnerUIManager->RequestDropInventorySlot(InventoryName, SlotIndex);
}

void UInventoryUIWidget::RequestUseInventorySlot(int32 SlotIndex)
{
	UE_LOG(LogTemp, Error, TEXT("void UInventoryUIWidget::RequestUseInventorySlot(int32 SlotIndex)"));
	if (!OwnerUIManager) return;
	OwnerUIManager->RequestUseInventorySlot(InventoryName, SlotIndex);
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

