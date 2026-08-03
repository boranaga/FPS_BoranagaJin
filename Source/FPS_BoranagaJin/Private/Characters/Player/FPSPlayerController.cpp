


#include "Characters/Player/FPSPlayerController.h"
#include "UI/UIManagerComponent.h"

#include "EnhancedInputSubsystems.h"
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

void AFPSPlayerController::InitUIManager()
{
	if (UIManagerComponent) UIManagerComponent->InitUIManagerComponent();
}
