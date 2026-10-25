#include "UI/GameResultEntryWidget.h"

#include "Components/TextBlock.h"

void UGameResultEntryWidget::SetGameResult(const FGameResultSaveData& ResultData)
{
	if (Text_Result)
	{
		FText ResultText;

		switch (ResultData.EndReason)
		{
		case EGameEndReason::BossDefeated:
			ResultText = FText::FromString(TEXT("Boss Defeated"));
			break;

		case EGameEndReason::Escaped:
			ResultText = FText::FromString(TEXT("Escaped"));
			break;

		case EGameEndReason::PlayerDead:
			ResultText = FText::FromString(TEXT("Game Over"));
			break;

		default:
			ResultText = FText::FromString(TEXT("Unknown"));
			break;
		}

		Text_Result->SetText(ResultText);
	}

	if (Text_Level)
	{
		Text_Level->SetText(FText::FromName(ResultData.FinalLevelName));
	}

	if (Text_PlayTime)
	{
		Text_PlayTime->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("Play Time : %s"),
					*ResultData.GetFormattedPlayTime()
				)
			)
		);
	}

	if (Text_CompletedAt)
	{
		Text_CompletedAt->SetText(
			FText::FromString(
				ResultData.CompletedAt.ToString(TEXT("%Y-%m-%d %H:%M"))
			)
		);
	}
}