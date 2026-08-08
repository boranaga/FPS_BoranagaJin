
#include "UI/ThrowableWeaponInventoryWidget.h"
#include "UI/InventorySlotWidget.h"
#include "Items/InventorySlot.h"

#include "Components/WrapBox.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UThrowableWeaponInventoryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetUIType(EUIType::ThrowableWeaponInventory);
}

void UThrowableWeaponInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UThrowableWeaponInventoryWidget::OpenUI()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UThrowableWeaponInventoryWidget::CloseUI()
{
	SetVisibility(ESlateVisibility::Hidden);
}

void UThrowableWeaponInventoryWidget::ShowMouseCursor()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(true);
		FInputModeGameAndUI NewInputMode;
		PlayerController->SetInputMode(NewInputMode);
	}
}

void UThrowableWeaponInventoryWidget::HideMouseCursor()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);
		FInputModeGameOnly NewInputMode;
		PlayerController->SetInputMode(NewInputMode);
	}
}

void UThrowableWeaponInventoryWidget::CreateInventorySlots(int32 InventorySlotCount)
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
}

void UThrowableWeaponInventoryWidget::UpdateInventorySlots(const TArray<FInventorySlot>& Inventory)
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (!Inventory[i].IsEmpty())
		{
			if (InventorySlotWidgets.IsValidIndex(i) && InventorySlotWidgets[i])
			{
				InventorySlotWidgets[i]->SetItemSlotData(Inventory[i].ItemID, Inventory[i].Num());

				//UE_LOG(LogTemp, Error, TEXT("UInventoryUIWidget::UpdateInventorySlots(const TArray<FInventorySlot>& Inventory)"));
			}
		}
	}
}
