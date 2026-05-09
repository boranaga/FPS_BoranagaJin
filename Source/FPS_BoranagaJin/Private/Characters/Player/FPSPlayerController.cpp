


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

	UIManagerComponent = CreateDefaultSubobject<UUIManagerComponent>(TEXT("UIManagerComponent"));
}

void AFPSPlayerController::BeginPlay()
{
	Super::BeginPlay();
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
