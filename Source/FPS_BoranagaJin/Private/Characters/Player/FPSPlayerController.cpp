


#include "Characters/Player/FPSPlayerController.h"

#include "Characters/Player/CharacterPlayer.h"

#include "UI/UIManagerComponent.h"
#include "UI/PauseMenuWidget.h"
#include "UI/GameOverWidget.h"
#include "UI/HealthWidget.h"
#include "UI/ThrowableWeaponInventoryWidget.h"

#include "PlayerUISubsystem.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"


#include "FPS_BoranagaJinCameraManager.h" //TODO: ???
#include "FPS_BoranagaJin.h" //TODO: ???

AFPSPlayerController::AFPSPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AFPS_BoranagaJinCameraManager::StaticClass();

	// TODO: SubSystem으로 통합해야함
	//UIManagerComponent = CreateDefaultSubobject<UUIManagerComponent>(TEXT("UIManagerComponent"));
}

void AFPSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	//-------------------------
	if (!IsLocalPlayerController())
	{
		return;
	}

	SetShowMouseCursor(false);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	/*
	 * 에디터에서는 뷰포트 포커스가 유지되지 않을 때가 있으므로
	 * 게임 뷰포트에 포커스를 되돌립니다.
	 */
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UGameViewportClient* ViewportClient = LocalPlayer->ViewportClient)
		{
			ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
		}
	}

	//---------------------------------

	if (!IsLocalPlayerController()) { return; }

	UPlayerUISubsystem* UISubsystem = GetLocalPlayer()->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(UISubsystem)) { return; }
	UISubsystem->InitPauseMenuUI(PauseMenuWidgetClass);
	UISubsystem->InitGameOverUI(GameOverWidgetClass);

}

void AFPSPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	if (IsLocalPlayerController())
	{

	}
}

void AFPSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();


	//// <Old Version>
	//if (IsLocalPlayerController())
	//{
	//	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	//	{
	//		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
	//		{
	//			Subsystem->AddMappingContext(CurrentContext, 0);
	//		}
	//	}
	//}
	//---------------------------------
	// <New Version>

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

	if (!IsValid(EnhancedInputComponent)) { return; }
	if (!IsValid(IA_Pause)) { return; }

	EnhancedInputComponent->BindAction(IA_Pause, ETriggerEvent::Started, this, &AFPSPlayerController::HandlePauseInput);
	EnhancedInputComponent->BindAction(IA_Tab, ETriggerEvent::Started, this, &AFPSPlayerController::OnTabToggled);
	EnhancedInputComponent->BindAction(IA_V, ETriggerEvent::Started, this, &AFPSPlayerController::OpenThrowableWeaponInventory);
	EnhancedInputComponent->BindAction(IA_V, ETriggerEvent::Completed, this, &AFPSPlayerController::CloseThrowableWeaponInventory);

}

//void AFPSPlayerController::InitUIManager()
//{
//
//	//if (UIManagerComponent) UIManagerComponent->InitUIManagerComponent();
//
//	//------------------------------
//	////TODO: <임시>
//
//	//ULocalPlayer* LocalPlayer = GetLocalPlayer();
//	//if (!IsValid(LocalPlayer)) { return; }
//	//UPlayerUISubsystem* PlayerUISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();
//	//if (!IsValid(PlayerUISubsystem)) { return; }
//
//	//PlayerUISubsystem->InitThrowableWeaponInventoryUI(ThrowableWeaponInventoryWidgetClass);
//
//	//CharacterPlayer->OnThrowableWeaponInventoryCreatedDelegate.AddDynamic(ThrowableWeaponInventoryWidget, &UThrowableWeaponInventoryWidget::CreateInventorySlots);
//    //CharacterPlayer->OnThrowableWeaponInventoryUpdatedDelegate.AddDynamic(ThrowableWeaponInventoryWidget, &UThrowableWeaponInventoryWidget::UpdateInventorySlots);
//}

void AFPSPlayerController::InitPlayerUISubsystem()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer)) { return; }
	UPlayerUISubsystem* PlayerUISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(PlayerUISubsystem)) { return; }

	ACharacterPlayer* ControlledCharacter = Cast<ACharacterPlayer>(GetPawn());
	if (!ControlledCharacter) { return; }

	PlayerUISubsystem->SetControlledCharacter(ControlledCharacter);

	PlayerUISubsystem->InitHealthBarUI(HealthBarWidgetClass);
	PlayerUISubsystem->InitStaminaUI(StaminaWidgetClass);

	PlayerUISubsystem->InitInteractionWidget(InteractionWidgetClass);
	PlayerUISubsystem->InitInventoryUI(PlayerDisplayWidgetClass);

	PlayerUISubsystem->InitThrowableWeaponInventoryUI(ThrowableWeaponInventoryWidgetClass);
	PlayerUISubsystem->Init_BeginPlay();
}

void AFPSPlayerController::HandlePauseInput()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer)) { return; }
	UPlayerUISubsystem* PlayerUISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(PlayerUISubsystem)) { return; }

	PlayerUISubsystem->TogglePauseMenu();
}

void AFPSPlayerController::OpenInventory()
{
	//if (!PlayerDisplayWidget) return;
	//PlayerDisplayWidget->SetVisibility(ESlateVisibility::Visible);
	//PlayerDisplayWidget->OpenInventory();
	//bIsInventoryOpened = true;


	//--------------------

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer)) { return; }
	UPlayerUISubsystem* PlayerUISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(PlayerUISubsystem)) { return; }

	bIsInventoryOpened = true;
	PlayerUISubsystem->OpenInventory();
}

void AFPSPlayerController::CloseInventory()
{
	//if (!PlayerDisplayWidget) return;
	//PlayerDisplayWidget->SetVisibility(ESlateVisibility::Hidden);
	//PlayerDisplayWidget->CloseInventory();
	//bIsInventoryOpened = false;

	//---------------------------

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer)) { return; }
	UPlayerUISubsystem* PlayerUISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(PlayerUISubsystem)) { return; }

	bIsInventoryOpened = false;
	PlayerUISubsystem->CloseInventory();
}

void AFPSPlayerController::OnTabToggled()
{
	bIsInventoryOpened ? CloseInventory() : OpenInventory();
}

void AFPSPlayerController::OpenThrowableWeaponInventory()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer)) { return; }
	UPlayerUISubsystem* PlayerUISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(PlayerUISubsystem)) { return; }

	PlayerUISubsystem->OpenThrowableWeaponInventory();
}

void AFPSPlayerController::CloseThrowableWeaponInventory()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer)) { return; }
	UPlayerUISubsystem* PlayerUISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(PlayerUISubsystem)) { return; }

	PlayerUISubsystem->CloseThrowableWeaponInventory();
}