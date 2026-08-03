
#include "UI/MainMenuWidget.h"
#include "Components/Button.h"

void UMainMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button_Play))
	{
		Button_Play->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandlePlayButtonClicked);
		Button_Play->OnClicked.AddDynamic(this, &UMainMenuWidget::HandlePlayButtonClicked);
	}

	if (IsValid(Button_Exit))
	{
		Button_Exit->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleExitButtonClicked);
		Button_Exit->OnClicked.AddDynamic(this,&UMainMenuWidget::HandleExitButtonClicked);
	}
}

void UMainMenuWidget::NativeDestruct()
{
	if (IsValid(Button_Play))
	{
		Button_Play->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandlePlayButtonClicked);
	}

	if (IsValid(Button_Exit))
	{
		Button_Exit->OnClicked.RemoveDynamic(this, &UMainMenuWidget::HandleExitButtonClicked);
	}

	Super::NativeDestruct();
}

void UMainMenuWidget::HandlePlayButtonClicked()
{
	OnPlayRequested.Broadcast();
}

void UMainMenuWidget::HandleExitButtonClicked()
{
	OnExitRequested.Broadcast();
}
