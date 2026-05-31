// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PlayerDisplayWidget.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

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
