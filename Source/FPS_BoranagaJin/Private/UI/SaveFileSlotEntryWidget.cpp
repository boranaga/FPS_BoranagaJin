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
}

void USaveFileSlotEntryWidget::HandleLoadButtonClicked()
{
	OnSaveSlotClicked.Broadcast(SlotInfo.SlotName);
}