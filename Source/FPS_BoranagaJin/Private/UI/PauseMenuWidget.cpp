#include "UI/PauseMenuWidget.h"
#include "Components/Button.h"

void UPauseMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button_Play))
	{
		UE_LOG(LogTemp, Error, TEXT("void UPauseMenuWidget::NativeConstruct(): IsValid(Button_Play)"));
		Button_Play->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandlePlayButtonClicked);
		Button_Play->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandlePlayButtonClicked);
	}

	if (IsValid(Button_Option))
	{
		UE_LOG(LogTemp, Error, TEXT("void UPauseMenuWidget::NativeConstruct(): IsValid(Button_Option)"));
		Button_Option->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleOptionButtonClicked);
		Button_Option->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleOptionButtonClicked);
	}

	if (IsValid(Button_SaveAndExit))
	{
		UE_LOG(LogTemp, Error, TEXT("void UPauseMenuWidget::NativeConstruct(): IsValid(Button_SaveAndExist)"));
		Button_SaveAndExit->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleSaveAndExitButtonClicked);
		Button_SaveAndExit->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleSaveAndExitButtonClicked);
	}
}

void UPauseMenuWidget::NativeDestruct()
{
	if (IsValid(Button_Play))
	{
		Button_Play->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandlePlayButtonClicked);
	}

	if (IsValid(Button_Option))
	{
		Button_Option->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleOptionButtonClicked);
	}

	if (IsValid(Button_SaveAndExit))
	{
		Button_SaveAndExit->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleSaveAndExitButtonClicked);
	}

	Super::NativeDestruct();
}

void UPauseMenuWidget::HandlePlayButtonClicked()
{
	UE_LOG(LogTemp, Error, TEXT("void UPauseMenuWidget::HandlePlayButtonClicked()"));
	OnPlayRequested.Broadcast();
}

void UPauseMenuWidget::HandleOptionButtonClicked()
{
	UE_LOG(LogTemp, Error, TEXT("void UPauseMenuWidget::HandleOptionButtonClicked()"));
	OnOptionRequested.Broadcast();
}

void UPauseMenuWidget::HandleSaveAndExitButtonClicked()
{
	UE_LOG(LogTemp, Error, TEXT("void UPauseMenuWidget::HandleSaveAndExitButtonClicked()"));

	OnSaveAndExitRequested.Broadcast();
}