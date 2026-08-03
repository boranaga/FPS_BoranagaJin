// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuPlayerController.h"
#include "PlayerUISubsystem.h"
#include "UI/MainMenuWidget.h"

#include "EnhancedInputSubsystems.h"

#include "FPS_BoranagaJinCameraManager.h"

AMainMenuPlayerController::AMainMenuPlayerController()
{
    PlayerCameraManagerClass = AFPS_BoranagaJinCameraManager::StaticClass();
}

void AMainMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalPlayerController()) { return; }

    UPlayerUISubsystem* UISubsystem = GetLocalPlayer()->GetSubsystem<UPlayerUISubsystem>();

    if (UISubsystem)
    {
        if (MainMenuWidgetClass)
        {
            UISubsystem->InitMainMenuUI(MainMenuWidgetClass);
        }
    }

    SetShowMouseCursor(true);

    FInputModeUIOnly InputMode;
    SetInputMode(InputMode);
}

void AMainMenuPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (IsLocalPlayerController())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
        {
            for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
            {
                Subsystem->AddMappingContext(CurrentContext, 0);
            }
        }
    }
}
