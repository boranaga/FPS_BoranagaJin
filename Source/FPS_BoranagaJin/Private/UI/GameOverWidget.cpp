#include "UI/GameOverWidget.h"

//#include "GameFlow/GameFlowSubsystem.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//if (Button_Retry)
	//{
	//	Button_Retry->OnClicked.RemoveDynamic(this, &UGameOverWidget::HandleRetryClicked);
	//	Button_Retry->OnClicked.AddDynamic(this, &UGameOverWidget::HandleRetryClicked);
	//}

	if (Button_MainMenu)
	{
		Button_MainMenu->OnClicked.RemoveDynamic(this, &UGameOverWidget::HandleMainMenuClicked);
		Button_MainMenu->OnClicked.AddDynamic(this, &UGameOverWidget::HandleMainMenuClicked);
	}

	if (Button_Exit)
	{
		Button_Exit->OnClicked.RemoveDynamic(this, &UGameOverWidget::HandleExitClicked);
		Button_Exit->OnClicked.AddDynamic(this, &UGameOverWidget::HandleExitClicked);
	}
}

void UGameOverWidget::NativeDestruct()
{
	//if (Button_Retry)
	//{
	//	Button_Retry->OnClicked.RemoveDynamic(this, &UGameOverWidget::HandleRetryClicked);
	//}

	if (Button_MainMenu)
	{
		Button_MainMenu->OnClicked.RemoveDynamic(this, &UGameOverWidget::HandleMainMenuClicked);
	}

	if (Button_Exit)
	{
		Button_Exit->OnClicked.RemoveDynamic(this, &UGameOverWidget::HandleExitClicked);
	}

	Super::NativeDestruct();
}

//void UGameOverWidget::SetGameEndReason(EGameEndReason EndReason)
//{
//	CurrentEndReason = EndReason;
//
//	UpdateGameOverText();
//}

void UGameOverWidget::UpdateGameOverText()
{
	if (!Text_Result || !Text_Description)
	{
		return;
	}

	//switch (CurrentEndReason)
	//{
	//case EGameEndReason::PlayerDead:
	//	Text_Result->SetText(FText::FromString(TEXT("GAME OVER")));
	//	Text_Description->SetText(FText::FromString(TEXT("You Died")));
	//	break;

	//case EGameEndReason::BossDefeated:
	//	Text_Result->SetText(FText::FromString(TEXT("MISSION COMPLETE")));
	//	Text_Description->SetText(FText::FromString(TEXT("Main Boss Defeated")));
	//	break;

	//case EGameEndReason::Escaped:
	//	Text_Result->SetText(FText::FromString(TEXT("ESCAPED")));
	//	Text_Description->SetText(FText::FromString(TEXT("You escaped from the area")));
	//	break;

	//default:
	//	Text_Result->SetText(FText::GetEmpty());
	//	Text_Description->SetText(FText::GetEmpty());
	//	break;
	//}
}

//void UGameOverWidget::HandleRetryClicked()
//{
//	UGameInstance* GameInstance = GetGameInstance();
//
//	if (!IsValid(GameInstance))
//	{
//		return;
//	}
//
//	if (UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>())
//	{
//		GameFlowSubsystem->RestartFromLastSave();
//	}
//}

void UGameOverWidget::HandleMainMenuClicked()
{
	OnMainMenuRequested.Broadcast();

	////----------------------------------

	//UGameInstance* GameInstance = GetGameInstance();

	//if (!IsValid(GameInstance))
	//{
	//	return;
	//}

	//if (UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>())
	//{
	//	GameFlowSubsystem->ReturnToMainMenu();
	//}
}

void UGameOverWidget::HandleExitClicked()
{
	OnExitRequested.Broadcast();

	//----------------------------------

	//UGameInstance* GameInstance = GetGameInstance();

	//if (!IsValid(GameInstance))
	//{
	//	return;
	//}

	//if (UGameFlowSubsystem* GameFlowSubsystem = GameInstance->GetSubsystem<UGameFlowSubsystem>())
	//{
	//	GameFlowSubsystem->ExitGame();
	//}
}
