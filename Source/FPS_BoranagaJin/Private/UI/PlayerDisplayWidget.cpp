// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PlayerDisplayWidget.h"
#include "UI/InventoryUIWidget.h"

#include "Items/InventorySlot.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UPlayerDisplayWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetUIType(EUIType::UIType_Inventory);

	//InventoryUIWidget = CreateWidget<UInventoryUIWidget>(GetWorld(), InventoryUIWidgetClass);
	//InventoryUIWidget->AddToViewport();
}

void UPlayerDisplayWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPlayerDisplayWidget::OpenInventory()
{
	ShowMouseCursor();
}

void UPlayerDisplayWidget::CloseInventory()
{
	HideMouseCursor();
}

void UPlayerDisplayWidget::ShowMouseCursor()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(true);
		FInputModeGameAndUI NewInputMode;
		PlayerController->SetInputMode(NewInputMode);
	}
}

void UPlayerDisplayWidget::HideMouseCursor()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->SetShowMouseCursor(false);
		FInputModeGameOnly NewInputMode;
		PlayerController->SetInputMode(NewInputMode);
	}
}

void UPlayerDisplayWidget::DisplayInventory()
{
}


void UPlayerDisplayWidget::CreateItemInventorySlots(int32 InventorySlotCount)
{
	if (ItemInventoryUIWidget)
	{
		ItemInventoryUIWidget->SetOwnerUIManager(OwnerUIManager);
		ItemInventoryUIWidget->CreateInventorySlots(InventorySlotCount);
	}
}

void UPlayerDisplayWidget::UpdateItemInventorySlots(const TArray<FInventorySlot>& Inventory)
{
	if (ItemInventoryUIWidget)
	{
		ItemInventoryUIWidget->UpdateInventorySlots(Inventory);
	}
}

void UPlayerDisplayWidget::CreateWeaponInventorySlots(int32 InventorySlotCount)
{
	if (WeaponInventoryUIWidget)
	{
		WeaponInventoryUIWidget->SetOwnerUIManager(OwnerUIManager);
		WeaponInventoryUIWidget->CreateInventorySlots(InventorySlotCount);
	}
}

void UPlayerDisplayWidget::UpdateWeaponInventorySlots(const TArray<FInventorySlot>& Inventory)
{
	if (WeaponInventoryUIWidget)
	{
		WeaponInventoryUIWidget->UpdateInventorySlots(Inventory);
	}
}
