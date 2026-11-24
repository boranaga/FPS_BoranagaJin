#include "UI/ExitDoorWidget.h"
#include "PlayerUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "Input/Reply.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

namespace
{
	void SetTextStyle(UTextBlock* TextBlock, int32 FontSize)
	{
		if (!TextBlock) { return; }

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		TextBlock->SetJustification(ETextJustify::Center);
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		// Prevent text from intercepting button clicks.
		TextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UExitSlotButton::InitSlot(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	// Avoid taking keyboard focus away from player movement.
	InitIsFocusable(false);
	OnClicked.AddUniqueDynamic(this, &UExitSlotButton::HandleClicked);
}

void UExitSlotButton::HandleClicked()
{
	OnSlotButtonClicked.Broadcast(SlotIndex);
}

void UExitDoorWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true); // Needed for Escape while using GameAndUI input mode.
}

FReply UExitDoorWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		UPlayerUISubsystem* UISubsystem = IsValid(LocalPlayer)
			? LocalPlayer->GetSubsystem<UPlayerUISubsystem>() : nullptr;
		if (IsValid(UISubsystem) && UISubsystem->IsExitDoorUIOpen())
		{
			UISubsystem->CloseExitDoorUI();
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

TSharedRef<SWidget> UExitDoorWidget::RebuildWidget()
{
	// Build a fallback widget tree when no WBP provides one.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultTree();
	}

	return Super::RebuildWidget();
}

void UExitDoorWidget::BuildDefaultTree()
{
	// Full-screen root with a centered panel; clicks pass through outside the panel.
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(BackgroundColor);
	Background->SetPadding(FMargin(18.f));
	if (UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background))
	{
		BackgroundSlot->SetHorizontalAlignment(HAlign_Center);
		BackgroundSlot->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Layout"));
	Background->SetContent(Layout);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	SetTextStyle(TitleText, 22);
	if (UVerticalBoxSlot* TitleSlot = Layout->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UTextBlock* RequiredLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RequiredLabel"));
	SetTextStyle(RequiredLabel, 14);
	RequiredLabel->SetText(RequiredLabelText);
	if (UVerticalBoxSlot* LabelSlot = Layout->AddChildToVerticalBox(RequiredLabel))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 10.f));
	}

	SlotBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotBox"));
	if (UVerticalBoxSlot* SlotBoxSlot = Layout->AddChildToVerticalBox(SlotBox))
	{
		SlotBoxSlot->SetHorizontalAlignment(HAlign_Center);
	}

	HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
	SetTextStyle(HintText, 14);
	if (UVerticalBoxSlot* HintSlot = Layout->AddChildToVerticalBox(HintText))
	{
		HintSlot->SetHorizontalAlignment(HAlign_Center);
		HintSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}
}

void UExitDoorWidget::RebuildSlotEntries(int32 SlotCount)
{
	SlotButtons.Reset();
	SlotIcons.Reset();
	SlotNameTexts.Reset();
	SlotCountTexts.Reset();

	if (!SlotBox || !WidgetTree) { return; }

	SlotBox->ClearChildren();

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SizeBox->SetWidthOverride(SlotSize.X);
		SizeBox->SetHeightOverride(SlotSize.Y);

		UExitSlotButton* Button = WidgetTree->ConstructWidget<UExitSlotButton>(UExitSlotButton::StaticClass());
		Button->InitSlot(Index);
		Button->OnSlotButtonClicked.AddUObject(this, &UExitDoorWidget::HandleSlotButtonClicked);
		SizeBox->SetContent(Button);

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Content->SetVisibility(ESlateVisibility::HitTestInvisible);
		Button->SetContent(Content);

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Icon->SetDesiredSizeOverride(IconSize);
		if (UVerticalBoxSlot* IconSlot = Content->AddChildToVerticalBox(Icon))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		SetTextStyle(NameText, 18);
		if (UVerticalBoxSlot* NameSlot = Content->AddChildToVerticalBox(NameText))
		{
			NameSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		SetTextStyle(CountText, 16);
		if (UVerticalBoxSlot* CountSlot = Content->AddChildToVerticalBox(CountText))
		{
			CountSlot->SetHorizontalAlignment(HAlign_Center);
			CountSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		}

		if (UHorizontalBoxSlot* EntrySlot = SlotBox->AddChildToHorizontalBox(SizeBox))
		{
			EntrySlot->SetPadding(FMargin(5.f, 0.f));
		}

		SlotButtons.Add(Button);
		SlotIcons.Add(Icon);
		SlotNameTexts.Add(NameText);
		SlotCountTexts.Add(CountText);
	}
}

void UExitDoorWidget::SetExitData(const FText& Title, const TArray<FExitSlotViewData>& Slots, const FText& Hint, bool bUnlocked)
{
	if (TitleText) { TitleText->SetText(Title); }
	if (HintText) { HintText->SetText(Hint); }

	if (SlotButtons.Num() != Slots.Num())
	{
		RebuildSlotEntries(Slots.Num());
	}

	for (int32 Index = 0; Index < Slots.Num() && Index < SlotButtons.Num(); ++Index)
	{
		const FExitSlotViewData& Data = Slots[Index];
		const bool bInserted = Data.State == EExitSlotState::Inserted;
		const bool bOwned = Data.State == EExitSlotState::Owned;

		// Only owned items are clickable.
		SlotButtons[Index]->SetIsEnabled(bOwned);
		SlotButtons[Index]->SetBackgroundColor(GetStateColor(Data.State));

		SlotNameTexts[Index]->SetText(Data.DisplayName);
		SlotNameTexts[Index]->SetColorAndOpacity(FSlateColor(bOwned ? FLinearColor::Black : FLinearColor::White));

		// Inserted count / required count
		SlotCountTexts[Index]->SetText(FText::FromString(bInserted ? TEXT("1/1") : TEXT("0/1")));
		SlotCountTexts[Index]->SetColorAndOpacity(FSlateColor(bOwned ? FLinearColor::Black : FLinearColor::White));

		if (Data.Icon)
		{
			SlotIcons[Index]->SetBrushFromTexture(Data.Icon);
			SlotIcons[Index]->SetDesiredSizeOverride(IconSize);
			SlotIcons[Index]->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			SlotIcons[Index]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	OnExitDataChanged(Slots, bUnlocked);
}

void UExitDoorWidget::HandleSlotButtonClicked(int32 SlotIndex)
{
	OnSlotClicked.Broadcast(SlotIndex);
}

FLinearColor UExitDoorWidget::GetStateColor(EExitSlotState State) const
{
	switch (State)
	{
	case EExitSlotState::Owned: return OwnedColor;
	case EExitSlotState::Inserted: return InsertedColor;
	default: return MissingColor;
	}
}
