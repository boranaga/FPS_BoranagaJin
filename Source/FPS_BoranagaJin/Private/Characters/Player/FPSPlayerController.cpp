


#include "Characters/Player/FPSPlayerController.h"
#include "UI/UIManagerComponent.h"
#include "UI/PauseMenuWidget.h"

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


	// TODO: SubSystem으로 변경해야함
	UIManagerComponent = CreateDefaultSubobject<UUIManagerComponent>(TEXT("UIManagerComponent"));
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
}

void AFPSPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	if (IsLocalPlayerController())
	{
		//if (AHorrorCharacter* HorrorCharacter = Cast<AHorrorCharacter>(aPawn))
		//{

		//}
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

}

void AFPSPlayerController::InitUIManager()
{
	if (UIManagerComponent) UIManagerComponent->InitUIManagerComponent();
}

void AFPSPlayerController::HandlePauseInput()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();

	if (!IsValid(LocalPlayer)) { return; }

	UPlayerUISubsystem* PlayerUISubsystem = LocalPlayer->GetSubsystem<UPlayerUISubsystem>();

	if (!IsValid(PlayerUISubsystem)) { return; }

	PlayerUISubsystem->TogglePauseMenu();
}
