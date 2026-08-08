
#include "UI/MainMenuWidget.h"
#include "Components/Button.h"

void UMainMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button_PlayNewGame))
	{
		Button_PlayNewGame->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandlePlayButtonClicked);
		Button_PlayNewGame->OnClicked.AddDynamic(this, &UMainMenuWidget::HandlePlayButtonClicked);
	}

	if (IsValid(Button_Continue))
	{
		Button_Continue->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleContinueButtonClicked);
		Button_Continue->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleContinueButtonClicked);
	}

	if (IsValid(Button_Option))
	{
		Button_Option->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleOptionButtonClicked);
		Button_Option->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleOptionButtonClicked);
	}

	if (IsValid(Button_Exit))
	{
		Button_Exit->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleExitButtonClicked);
		Button_Exit->OnClicked.AddDynamic(this,&UMainMenuWidget::HandleExitButtonClicked);
	}
}

void UMainMenuWidget::NativeDestruct()
{
	if (IsValid(Button_PlayNewGame))
	{
		Button_PlayNewGame->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandlePlayButtonClicked);
	}

	if (IsValid(Button_Exit))
	{
		Button_Exit->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleExitButtonClicked);
	}

	Super::NativeDestruct();
}

void UMainMenuWidget::HandlePlayButtonClicked()
{
	OnPlayNewGameRequested.Broadcast();
}

void UMainMenuWidget::HandleContinueButtonClicked()
{
	OnContinueRequested.Broadcast();
}

void UMainMenuWidget::HandleOptionButtonClicked()
{
	OnOptionRequested.Broadcast();
}

void UMainMenuWidget::HandleExitButtonClicked()
{
	OnExitRequested.Broadcast();
}
