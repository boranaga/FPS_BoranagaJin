#include "UI/SaveFileSlotMenuWidget.h"
#include "UI/SaveFileSlotEntryWidget.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"


void USaveFileSlotMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void USaveFileSlotMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//if (IsValid(Button_Slot_1))
	//{
	//	Button_Slot_1->OnClicked.RemoveDynamic(this, &USaveFileSlotMenuWidget::HandleSaveFile_1ButtonClicked);
	//	Button_Slot_1->OnClicked.AddDynamic(this, &USaveFileSlotMenuWidget::HandleSaveFile_1ButtonClicked);
	//}

	//if (IsValid(Button_Slot_2))
	//{
	//	Button_Slot_2->OnClicked.RemoveDynamic(this, &USaveFileSlotMenuWidget::HandleSaveFile_2ButtonClicked);
	//	Button_Slot_2->OnClicked.AddDynamic(this, &USaveFileSlotMenuWidget::HandleSaveFile_2ButtonClicked);
	//}

	//if (IsValid(Button_Slot_3))
	//{
	//	Button_Slot_3->OnClicked.RemoveDynamic(
	//		this,
	//		&USaveFileSlotMenuWidget::HandleSaveFile_3ButtonClicked
	//	);

	//	Button_Slot_3->OnClicked.AddDynamic(
	//		this,
	//		&USaveFileSlotMenuWidget::HandleSaveFile_3ButtonClicked
	//	);
	//}

	//if (IsValid(Button_Back))
	//{
	//	Button_Back->OnClicked.RemoveDynamic(
	//		this,
	//		&USaveFileSlotMenuWidget::HandleBackButtonClicked
	//	);

	//	Button_Back->OnClicked.AddDynamic(
	//		this,
	//		&USaveFileSlotMenuWidget::HandleBackButtonClicked
	//	);
	//}

	if (IsValid(Button_Back))
	{
		Button_Back->OnClicked.RemoveDynamic(this, &USaveFileSlotMenuWidget::HandleBackButtonClicked);
		Button_Back->OnClicked.AddDynamic(this, &USaveFileSlotMenuWidget::HandleBackButtonClicked);
	}
}

void USaveFileSlotMenuWidget::NativeDestruct()
{
	//if (IsValid(Button_Slot_1))
	//{
	//	Button_Slot_1->OnClicked.RemoveDynamic(
	//		this,
	//		&USaveFileSlotMenuWidget::HandleSaveFile_1ButtonClicked
	//	);
	//}

	//if (IsValid(Button_Slot_2))
	//{
	//	Button_Slot_2->OnClicked.RemoveDynamic(
	//		this,
	//		&USaveFileSlotMenuWidget::HandleSaveFile_2ButtonClicked
	//	);
	//}

	//if (IsValid(Button_Slot_3))
	//{
	//	Button_Slot_3->OnClicked.RemoveDynamic(
	//		this,
	//		&USaveFileSlotMenuWidget::HandleSaveFile_3ButtonClicked
	//	);
	//}

	//if (IsValid(Button_Back))
	//{
	//	Button_Back->OnClicked.RemoveDynamic(
	//		this,
	//		&USaveFileSlotMenuWidget::HandleBackButtonClicked
	//	);
	//}

	if (IsValid(Button_Back))
	{
		Button_Back->OnClicked.RemoveDynamic(this, &USaveFileSlotMenuWidget::HandleBackButtonClicked);
	}

	Super::NativeDestruct();
}

//void USaveFileSlotMenuWidget::HandleSaveFile_1ButtonClicked()
//{
//	OnSaveFileSlotSelected.Broadcast(1);
//}
//
//void USaveFileSlotMenuWidget::HandleSaveFile_2ButtonClicked()
//{
//	OnSaveFileSlotSelected.Broadcast(2);
//}
//
//void USaveFileSlotMenuWidget::HandleSaveFile_3ButtonClicked()
//{
//	OnSaveFileSlotSelected.Broadcast(3);
//}

void USaveFileSlotMenuWidget::HandleSaveSlotClicked(FString SlotName)
{
	OnSaveFileSlotSelected.Broadcast(SlotName);
}

void USaveFileSlotMenuWidget::HandleBackButtonClicked()
{
	OnBackRequested.Broadcast();
}

void USaveFileSlotMenuWidget::RefreshSaveSlots(const TArray<FSaveSlotInfo>& SaveSlots)
{
	if (!IsValid(SaveSlotContainer)) { return; }
	if (!SaveSlotEntryWidgetClass) { return; }

	SaveSlotContainer->ClearChildren();

	for (const FSaveSlotInfo& SlotInfo : SaveSlots)
	{
		USaveFileSlotEntryWidget* EntryWidget = CreateWidget<USaveFileSlotEntryWidget>(GetOwningPlayer(), SaveSlotEntryWidgetClass);

		if (!IsValid(EntryWidget)) { continue; }

		EntryWidget->InitializeSlot(SlotInfo);
		EntryWidget->OnSaveSlotClicked.AddUObject(this, &USaveFileSlotMenuWidget::HandleSaveSlotClicked);

		SaveSlotContainer->AddChild(EntryWidget);
	}
}
