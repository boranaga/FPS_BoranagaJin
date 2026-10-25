#include "UI/GameResultLogWidget.h"

#include "UI/GameResultEntryWidget.h"
#include "SaveSystem/SaveGameSubsystem.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"

void UGameResultLogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Back)
	{
		Button_Back->OnClicked.RemoveDynamic(this, &UGameResultLogWidget::HandleBackClicked);
		Button_Back->OnClicked.AddDynamic(this, &UGameResultLogWidget::HandleBackClicked);
	}

	RefreshGameResults();
}

void UGameResultLogWidget::RefreshGameResults()
{
	if (!IsValid(ScrollBox_GameResults))
	{
		return;
	}

	if (!GameResultEntryWidgetClass)
	{
		return;
	}

	ScrollBox_GameResults->ClearChildren();

	UGameInstance* GameInstance = GetGameInstance();

	if (!IsValid(GameInstance))
	{
		return;
	}

	USaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<USaveGameSubsystem>();

	if (!IsValid(SaveSubsystem))
	{
		return;
	}

	const TArray<FGameResultSaveData>& Results = SaveSubsystem->GetGameResultLogs();

	for (const FGameResultSaveData& Result : Results)
	{
		UGameResultEntryWidget* EntryWidget = CreateWidget<UGameResultEntryWidget>(
			this,
			GameResultEntryWidgetClass
		);

		if (!IsValid(EntryWidget))
		{
			continue;
		}

		EntryWidget->SetGameResult(Result);

		ScrollBox_GameResults->AddChild(EntryWidget);
	}
}

void UGameResultLogWidget::HandleBackClicked()
{
	// 현재 사용하는 PlayerUISubsystem 구조에 맞춰
	// GameResultLog UI를 숨기고 MainMenu UI를 다시 표시
}