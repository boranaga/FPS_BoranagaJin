


#include "Characters/Player/FPSPlayerController.h"

#include "Characters/Player/CharacterPlayer.h"

#include "UI/PauseMenuWidget.h"
#include "UI/GameOverWidget.h"
#include "UI/HealthWidget.h"
#include "UI/ThrowableWeaponInventoryWidget.h"
#include "UI/QuickSlotWidget.h"

#include "PlayerUISubsystem.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "InputMappingContext.h"


#include "FPS_BoranagaJinCameraManager.h" //TODO: ???
#include "FPS_BoranagaJin.h" //TODO: ???

AFPSPlayerController::AFPSPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = AFPS_BoranagaJinCameraManager::StaticClass();
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

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer)) { return; }
	UPlayerUISubsystem* UISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(UISubsystem)) { return; }
	UISubsystem->InitPauseMenuUI(PauseMenuWidgetClass);
	UISubsystem->InitGameOverUI(GameOverWidgetClass);

	if (IsValid(GetPawn()) && GetPawn()->HasActorBegunPlay())
	{
		InitPlayerUISubsystem();
	}

}

void AFPSPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);
	if (IsValid(GetPawn()) && GetPawn()->HasActorBegunPlay())
	{
		InitPlayerUISubsystem();
	}
}

void AFPSPlayerController::OnUnPossess()
{
	ReleasePlayerUISubsystem(Cast<ACharacterPlayer>(GetPawn()));
	Super::OnUnPossess();
}

void AFPSPlayerController::OnRep_Pawn()
{
	ACharacterPlayer* Previous = InitializedUICharacter.Get();
	Super::OnRep_Pawn();
	if (Previous && Previous != GetPawn()) { ReleasePlayerUISubsystem(Previous); }
	if (IsValid(GetPawn()) && GetPawn()->HasActorBegunPlay()) { InitPlayerUISubsystem(); }
}

void AFPSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleasePlayerUISubsystem(InitializedUICharacter.Get());
	Super::EndPlay(EndPlayReason);
}

void AFPSPlayerController::ReleasePlayerUISubsystem(const ACharacterPlayer* ExpectedCharacter)
{
	if (!ExpectedCharacter) { return; }
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UPlayerUISubsystem* UI = LocalPlayer->GetSubsystem<UPlayerUISubsystem>())
		{
			UI->ClearControlledCharacter(ExpectedCharacter);
		}
	}
	if (InitializedUICharacter.Get() == ExpectedCharacter)
	{
		InitializedUICharacter.Reset();
		bIsInventoryOpened = false;
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

void AFPSPlayerController::InitPlayerUISubsystem()
{
	if (!IsLocalPlayerController()) { return; }
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer)) { return; }
	UPlayerUISubsystem* PlayerUISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(PlayerUISubsystem)) { return; }

	ACharacterPlayer* ControlledCharacter = Cast<ACharacterPlayer>(GetPawn());
	if (!IsValid(ControlledCharacter)) { return; }

	if (InitializedUICharacter.Get() != ControlledCharacter)
	{
		bIsInventoryOpened = false;
	}
	InitializedUICharacter = ControlledCharacter;
	PlayerUISubsystem->SetControlledCharacter(ControlledCharacter);

	PlayerUISubsystem->InitHealthBarUI(HealthBarWidgetClass);
	PlayerUISubsystem->InitStaminaUI(StaminaWidgetClass);

	PlayerUISubsystem->InitInteractionWidget(InteractionWidgetClass);
	PlayerUISubsystem->InitInventoryUI(PlayerDisplayWidgetClass);

	PlayerUISubsystem->InitThrowableWeaponInventoryUI(ThrowableWeaponInventoryWidgetClass);

	PlayerUISubsystem->InitQuickSlotUI(QuickSlotWidgetClass);

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
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer)) { return; }
	UPlayerUISubsystem* PlayerUISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();
	if (!IsValid(PlayerUISubsystem)) { return; }

	bIsInventoryOpened = true;
	PlayerUISubsystem->OpenInventory();
}

void AFPSPlayerController::CloseInventory()
{
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