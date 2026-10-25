#include "UI/SaveFileSlotEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void USaveFileSlotEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button_Load))
	{
		Button_Load->OnClicked.RemoveDynamic(this, &USaveFileSlotEntryWidget::HandleLoadButtonClicked);
		Button_Load->OnClicked.AddDynamic(this, &USaveFileSlotEntryWidget::HandleLoadButtonClicked);
	}
}

void USaveFileSlotEntryWidget::NativeDestruct()
{
	if (IsValid(Button_Load))
	{
		Button_Load->OnClicked.RemoveDynamic(this, &USaveFileSlotEntryWidget::HandleLoadButtonClicked);
	}
	Super::NativeDestruct();
}

void USaveFileSlotEntryWidget::InitializeSlot(const FSaveSlotInfo& InSlotInfo)
{
	SlotInfo = InSlotInfo;

	if (IsValid(Text_LevelName))
	{
		Text_LevelName->SetText(FText::FromName(SlotInfo.SavedLevelName));
	}

	if (IsValid(Text_SavedAt))
	{
		Text_SavedAt->SetText(FText::FromString(SlotInfo.SavedAt.ToString()));
	}

	if (IsValid(Text_PlayTime))
	{
		Text_PlayTime->SetText(FText::FromString(GetFormattedPlayTime(SlotInfo.AccumulatedPlayTimeSeconds)));
	}
}

void USaveFileSlotEntryWidget::HandleLoadButtonClicked()
{
	OnSaveSlotClicked.Broadcast(SlotInfo.SlotName);
}

FString  USaveFileSlotEntryWidget::GetFormattedPlayTime(double AccumulatedPlayTime) const
{
	const int64 TotalSeconds = FMath::Max<int64>(0, static_cast<int64>(AccumulatedPlayTime));

	const int64 Hours = TotalSeconds / 3600;
	const int64 Minutes = (TotalSeconds % 3600) / 60;
	const int64 Seconds = TotalSeconds % 60;

	return FString::Printf(TEXT("%02lld:%02lld:%02lld"), Hours, Minutes, Seconds);
}